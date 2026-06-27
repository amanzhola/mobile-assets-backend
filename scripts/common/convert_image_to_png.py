import sys
from pathlib import Path

from PIL import Image, ImageOps


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


if len(sys.argv) != 3:
    fail("Usage: convert_image_to_png.py input_image output_png")

input_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])

if not input_path.exists():
    fail(f"Input not found: {input_path}")

image = Image.open(input_path)
image = ImageOps.exif_transpose(image)

if image.mode not in ("RGB", "RGBA"):
    image = image.convert("RGBA")

output_path.parent.mkdir(parents=True, exist_ok=True)
image.save(output_path, "PNG")

if not output_path.exists() or output_path.stat().st_size == 0:
    fail(f"Output was not created: {output_path}")

print(str(output_path))
