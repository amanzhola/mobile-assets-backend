import sys
from pathlib import Path

import numpy as np
import torch
from PIL import Image, ImageFilter
from transformers import CLIPSegProcessor, CLIPSegForImageSegmentation


RU_TO_EN = {
    "зонтик": "umbrella",
    "зонт": "umbrella",
    "человек": "person",
    "люди": "people",
    "машина": "car",
    "авто": "car",
    "водяной знак": "watermark",
    "логотип": "logo",
    "текст": "text",
    "провод": "wire",
    "провода": "wires",
    "мусор": "trash",
    "сумка": "bag",
    "стул": "chair",
    "стол": "table",
}


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


def normalize_prompt(text: str) -> str:
    value = text.strip().lower()
    return RU_TO_EN.get(value, value)


if len(sys.argv) < 4:
    fail("Usage: create_object_mask.py input mask_output object_text [threshold]")

input_path = Path(sys.argv[1])
mask_output_path = Path(sys.argv[2])
object_text = normalize_prompt(sys.argv[3])
threshold = float(sys.argv[4]) if len(sys.argv) >= 5 else 0.45

if not input_path.exists():
    fail(f"Input not found: {input_path}")

if not object_text:
    fail("Object text is empty")

image = Image.open(input_path).convert("RGB")

processor = CLIPSegProcessor.from_pretrained("CIDAS/clipseg-rd64-refined")
model = CLIPSegForImageSegmentation.from_pretrained("CIDAS/clipseg-rd64-refined")
model.eval()

inputs = processor(
    text=[object_text],
    images=[image],
    padding=True,
    return_tensors="pt",
)

with torch.no_grad():
    outputs = model(**inputs)

raw = outputs.logits[0].cpu().numpy()
raw = 1.0 / (1.0 + np.exp(-raw))

mn = float(raw.min())
mx = float(raw.max())

if mx > mn:
    raw = (raw - mn) / (mx - mn)

mask = Image.fromarray((raw * 255).astype(np.uint8))
mask = mask.resize(image.size, Image.Resampling.BICUBIC)

mask_np = np.array(mask)
mask_np = (mask_np > int(threshold * 255)).astype(np.uint8) * 255

mask_img = Image.fromarray(mask_np, mode="L")
mask_img = mask_img.filter(ImageFilter.MaxFilter(13))
mask_img = mask_img.filter(ImageFilter.GaussianBlur(radius=4))
mask_img = mask_img.point(lambda p: 255 if p > 20 else 0)

mask_np = np.array(mask_img)

if mask_np.max() == 0:
    fail(f"Auto mask is empty for object text: {object_text}")

# IMPORTANT for ComfyUI LoadImage second output:
# transparent alpha area becomes mask area.
alpha = np.where(mask_np > 0, 0, 255).astype(np.uint8)

rgba = np.zeros((image.height, image.width, 4), dtype=np.uint8)
rgba[:, :, 0] = 0
rgba[:, :, 1] = 0
rgba[:, :, 2] = 0
rgba[:, :, 3] = alpha

mask_output_path.parent.mkdir(parents=True, exist_ok=True)
Image.fromarray(rgba, mode="RGBA").save(mask_output_path)

debug_path = mask_output_path.with_name(mask_output_path.stem + "_debug_white_object.png")
Image.fromarray(mask_np, mode="L").save(debug_path)

print(str(mask_output_path))
