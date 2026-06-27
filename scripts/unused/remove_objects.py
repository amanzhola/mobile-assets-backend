import sys
from pathlib import Path

import cv2
import numpy as np
import torch
from PIL import Image, ImageFilter
from transformers import CLIPSegProcessor, CLIPSegForImageSegmentation


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


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


def normalize_prompt(text: str) -> str:
    value = text.strip().lower()
    return RU_TO_EN.get(value, value)


if len(sys.argv) < 4:
    fail("Usage: remove_objects.py input output object_text [threshold]")

input_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
object_text = normalize_prompt(sys.argv[3])
threshold = float(sys.argv[4]) if len(sys.argv) >= 5 else 0.25

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
    return_tensors="pt"
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
mask_img = mask_img.filter(ImageFilter.MaxFilter(21))
mask_img = mask_img.filter(ImageFilter.GaussianBlur(radius=9))
mask_img = mask_img.point(lambda p: 255 if p > 20 else 0)

mask_np = np.array(mask_img)

if mask_np.max() == 0:
    fail(f"Auto mask is empty for object text: {object_text}")

output_path.parent.mkdir(parents=True, exist_ok=True)

cv_image = cv2.cvtColor(np.array(image), cv2.COLOR_RGB2BGR)
cv_mask = mask_np.astype(np.uint8)

result = cv2.inpaint(
    cv_image,
    cv_mask,
    7,
    cv2.INPAINT_TELEA
)

result_rgb = cv2.cvtColor(result, cv2.COLOR_BGR2RGB)
Image.fromarray(result_rgb).save(output_path)

debug_mask = output_path.with_name(output_path.stem + "_mask.png")
Image.fromarray(cv_mask).save(debug_mask)

print(str(output_path))
