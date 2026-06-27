import sys
from pathlib import Path

from renderers.common import fail, load_mask, load_rgb, lower
from renderers.lip_renderer import render as render_lips
from renderers.cheek_renderer import render as render_cheeks
from renderers.eye_renderer import render as render_eyes
from renderers.brow_renderer import render as render_brows


if len(sys.argv) < 6:
    fail("Usage: apply_face_makeup.py input_image mask_rgba output_image region english_details")

input_path = Path(sys.argv[1])
mask_path = Path(sys.argv[2])
output_path = Path(sys.argv[3])
region = lower(sys.argv[4])
english_details = sys.argv[5]

if not input_path.exists():
    fail(f"Input not found: {input_path}")

if not mask_path.exists():
    fail(f"Mask not found: {mask_path}")

image = load_rgb(input_path)
mask = load_mask(mask_path, image.size)

if region == "lips":
    result, color, strength = render_lips(image, mask, english_details)
elif region == "cheeks":
    result, color, strength = render_cheeks(image, mask, english_details)
elif region == "eyelids" or region == "eyes":
    result, color, strength = render_eyes(image, mask, english_details)
elif region == "eyebrows":
    result, color, strength = render_brows(image, mask, english_details)
else:
    fail(f"Unsupported local makeup region: {region}")

output_path.parent.mkdir(parents=True, exist_ok=True)
result.save(output_path)

print(str(output_path))
print(f"region={region}")
print(f"color={color}")
print(f"strength={strength}")
