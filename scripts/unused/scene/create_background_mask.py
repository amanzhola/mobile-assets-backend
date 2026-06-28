import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter, ImageOps
from rembg import remove, new_session


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


if len(sys.argv) < 3:
    fail("Usage: create_background_mask.py input_image output_mask")

input_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])

if not input_path.exists():
    fail(f"Input not found: {input_path}")

image = Image.open(input_path)
image = ImageOps.exif_transpose(image).convert("RGB")

session = new_session("u2net_human_seg")

cutout = remove(
    image,
    session=session,
    alpha_matting=True,
    alpha_matting_foreground_threshold=240,
    alpha_matting_background_threshold=10,
    alpha_matting_erode_size=8,
).convert("RGBA")

alpha = np.array(cutout)[:, :, 3]

subject = np.where(alpha > 32, 255, 0).astype(np.uint8)
subject_img = Image.fromarray(subject, mode="L")

# Важно: для inpaint держим только core человека.
# Край вокруг человека отдаём Comfy, чтобы он не оставлял старый фон.
subject_core = subject_img.filter(ImageFilter.MinFilter(9))
subject_core = subject_core.filter(ImageFilter.GaussianBlur(radius=2.0))
subject_core_np = np.array(subject_core)

# Convention для текущего pipeline:
# alpha=0 -> edit area
# alpha=255 -> keep area
mask_alpha = np.where(subject_core_np > 80, 255, 0).astype(np.uint8)

rgba = np.zeros((image.height, image.width, 4), dtype=np.uint8)
rgba[:, :, 3] = mask_alpha

output_path.parent.mkdir(parents=True, exist_ok=True)
Image.fromarray(rgba, mode="RGBA").save(output_path)

debug_path = output_path.with_name(
    output_path.stem + "_debug_white_keep_subject_core.png"
)
Image.fromarray(mask_alpha, mode="L").save(debug_path)

print(str(output_path))
print("debug_mask=" + str(debug_path))