import sys
from pathlib import Path

from PIL import Image, ImageOps


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


if len(sys.argv) < 4:
    fail("Usage: create_prompt_collage.py output_image prompt_text input1 [input2 input3 input4]")

output_path = Path(sys.argv[1])
# prompt_text пока принимаем для совместимости, но НЕ рисуем на изображении.
_ = sys.argv[2]

input_paths = [Path(p) for p in sys.argv[3:7]]

if not input_paths:
    fail("No input images")

if len(input_paths) > 4:
    fail("Prompt supports max 4 images")

for path in input_paths:
    if not path.exists():
        fail(f"Input not found: {path}")

canvas_w = 1024
canvas_h = 768

canvas = Image.new("RGB", (canvas_w, canvas_h), (245, 245, 245))

count = len(input_paths)

if count == 1:
    boxes = [
        (0, 0, canvas_w, canvas_h),
    ]
elif count == 2:
    boxes = [
        (0, 0, canvas_w // 2, canvas_h),
        (canvas_w // 2, 0, canvas_w, canvas_h),
    ]
elif count == 3:
    boxes = [
        (0, 0, canvas_w // 2, canvas_h),
        (canvas_w // 2, 0, canvas_w, canvas_h // 2),
        (canvas_w // 2, canvas_h // 2, canvas_w, canvas_h),
    ]
else:
    boxes = [
        (0, 0, canvas_w // 2, canvas_h // 2),
        (canvas_w // 2, 0, canvas_w, canvas_h // 2),
        (0, canvas_h // 2, canvas_w // 2, canvas_h),
        (canvas_w // 2, canvas_h // 2, canvas_w, canvas_h),
    ]

for path, box in zip(input_paths, boxes):
    x1, y1, x2, y2 = box
    w = x2 - x1
    h = y2 - y1

    image = Image.open(path).convert("RGB")
    image = ImageOps.fit(
        image,
        (w, h),
        method=Image.Resampling.LANCZOS,
        centering=(0.5, 0.5),
    )
    canvas.paste(image, (x1, y1))

output_path.parent.mkdir(parents=True, exist_ok=True)
canvas.save(output_path, quality=95)
print(str(output_path))