import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter, ImageOps
from rembg import remove, new_session


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


if len(sys.argv) < 4:
    fail("Usage: apply_change_scene_composite.py original_image generated_image output_image")

original_path = Path(sys.argv[1])
generated_path = Path(sys.argv[2])
output_path = Path(sys.argv[3])

if not original_path.exists():
    fail(f"Original not found: {original_path}")

if not generated_path.exists():
    fail(f"Generated not found: {generated_path}")

original = Image.open(original_path)
original = ImageOps.exif_transpose(original).convert("RGB")

generated = Image.open(generated_path)
generated = ImageOps.exif_transpose(generated).convert("RGB").resize(
    original.size,
    Image.Resampling.LANCZOS
)

session = new_session("u2net_human_seg")

cutout = remove(
    original,
    session=session,
    alpha_matting=True,
    alpha_matting_foreground_threshold=240,
    alpha_matting_background_threshold=10,
    alpha_matting_erode_size=6,
).convert("RGBA")

alpha = cutout.getchannel("A")

# Убираем старую обводку фона вокруг человека.
alpha = alpha.filter(ImageFilter.MinFilter(3))
alpha = alpha.filter(ImageFilter.GaussianBlur(radius=1.2))

original_np = np.array(original).astype(np.float32)
generated_np = np.array(generated).astype(np.float32)
alpha_np = np.array(alpha).astype(np.float32) / 255.0
alpha_np = alpha_np[..., None]

result = original_np * alpha_np + generated_np * (1.0 - alpha_np)
result = np.clip(result, 0, 255).astype(np.uint8)

output_path.parent.mkdir(parents=True, exist_ok=True)
Image.fromarray(result, mode="RGB").save(output_path)

debug_path = output_path.with_name(output_path.stem + "_debug_subject_alpha.png")
alpha.save(debug_path)

print(str(output_path))
print("debug_alpha=" + str(debug_path))
