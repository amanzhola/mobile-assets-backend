import sys
from pathlib import Path

import numpy as np
import torch
from PIL import Image, ImageFilter
from transformers import AutoProcessor, AutoModelForZeroShotObjectDetection
from segment_anything import sam_model_registry, SamPredictor


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
    fail("Usage: create_object_mask_sam.py input mask_output object_text [box_threshold] [text_threshold]")

input_path = Path(sys.argv[1])
mask_output_path = Path(sys.argv[2])
object_text = normalize_prompt(sys.argv[3])
box_threshold = float(sys.argv[4]) if len(sys.argv) >= 5 else 0.32
text_threshold = float(sys.argv[5]) if len(sys.argv) >= 6 else 0.25

project_root = Path(__file__).resolve().parents[1]
sam_checkpoint = project_root / "models/local/sam_vit_b_01ec64.pth"

if not input_path.exists():
    fail(f"Input not found: {input_path}")

if not sam_checkpoint.exists():
    fail(f"SAM checkpoint not found: {sam_checkpoint}")

device = "cuda" if torch.cuda.is_available() else "cpu"

image = Image.open(input_path).convert("RGB")
image_np = np.array(image)

model_id = "IDEA-Research/grounding-dino-tiny"

processor = AutoProcessor.from_pretrained(model_id)
model = AutoModelForZeroShotObjectDetection.from_pretrained(model_id).to(device)
model.eval()

text_prompt = object_text if object_text.endswith(".") else object_text + "."

inputs = processor(
    images=image,
    text=text_prompt,
    return_tensors="pt"
).to(device)

with torch.no_grad():
    outputs = model(**inputs)

# Compatibility for different transformers versions
try:
    result = processor.post_process_grounded_object_detection(
        outputs,
        inputs.input_ids,
        box_threshold=box_threshold,
        text_threshold=text_threshold,
        target_sizes=[image.size[::-1]],
    )[0]
except TypeError:
    result = processor.post_process_grounded_object_detection(
        outputs,
        inputs.input_ids,
        target_sizes=[image.size[::-1]],
    )[0]

boxes = result["boxes"].detach().cpu().numpy()

if "scores" in result:
    scores = result["scores"].detach().cpu().numpy()
else:
    scores = np.ones((len(boxes),), dtype=np.float32)

if len(boxes) == 0:
    fail(f"No object detected for: {object_text}")

# Manual score filter for old/new API compatibility
keep = scores >= box_threshold
if keep.any():
    boxes = boxes[keep]
    scores = scores[keep]

if len(boxes) == 0:
    fail(f"No high-confidence object detected for: {object_text}")

best_index = int(np.argmax(scores))
best_box = boxes[best_index]

sam = sam_model_registry["vit_b"](checkpoint=str(sam_checkpoint))
sam.to(device=device)

predictor = SamPredictor(sam)
predictor.set_image(image_np)

masks, mask_scores, _ = predictor.predict(
    box=best_box,
    multimask_output=True,
)

best_mask_index = int(np.argmax(mask_scores))
mask_np = masks[best_mask_index].astype(np.uint8) * 255

image_area = image.width * image.height
mask_area = int((mask_np > 0).sum())

if mask_area == 0:
    fail(f"SAM mask is empty for: {object_text}")

if mask_area > image_area * 0.35:
    fail(f"SAM mask too large for: {object_text}, area={mask_area}, image_area={image_area}")

mask_img = Image.fromarray(mask_np, mode="L")
mask_img = mask_img.filter(ImageFilter.MaxFilter(5))
mask_img = mask_img.filter(ImageFilter.GaussianBlur(radius=2))
mask_img = mask_img.point(lambda p: 255 if p > 32 else 0)

mask_np = np.array(mask_img)

alpha = np.where(mask_np > 0, 0, 255).astype(np.uint8)

rgba = np.zeros((image.height, image.width, 4), dtype=np.uint8)
rgba[:, :, 3] = alpha

mask_output_path.parent.mkdir(parents=True, exist_ok=True)
Image.fromarray(rgba, mode="RGBA").save(mask_output_path)

debug_path = mask_output_path.with_name(mask_output_path.stem + "_debug_white_object.png")
Image.fromarray(mask_np, mode="L").save(debug_path)

print(str(mask_output_path))