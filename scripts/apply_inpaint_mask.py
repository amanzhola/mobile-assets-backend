import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


if len(sys.argv) < 5:
    fail("Usage: apply_inpaint_mask.py original inpainted mask_rgba output")

original_path = Path(sys.argv[1])
inpainted_path = Path(sys.argv[2])
mask_path = Path(sys.argv[3])
output_path = Path(sys.argv[4])

for path in [original_path, inpainted_path, mask_path]:
    if not path.exists():
        fail(f"File not found: {path}")

original = Image.open(original_path).convert("RGB")
inpainted = Image.open(inpainted_path).convert("RGB").resize(original.size, Image.Resampling.LANCZOS)
mask_rgba = Image.open(mask_path).convert("RGBA").resize(original.size, Image.Resampling.NEAREST)

alpha = np.array(mask_rgba)[:, :, 3]

# In our Comfy mask file: object area is transparent alpha=0.
mask = np.where(alpha < 128, 255, 0).astype(np.uint8)

mask_img = Image.fromarray(mask, mode="L")
mask_img = mask_img.filter(ImageFilter.MaxFilter(5))
mask_img = mask_img.filter(ImageFilter.GaussianBlur(radius=3))

result = Image.composite(inpainted, original, mask_img)

output_path.parent.mkdir(parents=True, exist_ok=True)
result.save(output_path)
print(str(output_path))
