import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


if len(sys.argv) < 3:
    fail("Usage: prepare_manual_cleanup_mask.py input_mask output_rgba_mask")

input_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])

if not input_path.exists():
    fail(f"Mask not found: {input_path}")

mask = Image.open(input_path).convert("L")
mask_np = np.array(mask)

remove_np = np.where(mask_np > 8, 255, 0).astype(np.uint8)

remove_img = Image.fromarray(remove_np, mode="L")
remove_img = remove_img.filter(ImageFilter.MaxFilter(9))
remove_img = remove_img.filter(ImageFilter.GaussianBlur(radius=2))
remove_np = np.array(remove_img)

alpha = np.where(remove_np > 20, 0, 255).astype(np.uint8)

rgba = np.zeros((mask.height, mask.width, 4), dtype=np.uint8)
rgba[:, :, 3] = alpha

output_path.parent.mkdir(parents=True, exist_ok=True)
Image.fromarray(rgba, mode="RGBA").save(output_path)

debug_path = output_path.with_name(output_path.stem + "_debug_white_cleanup.png")
Image.fromarray(remove_np, mode="L").save(debug_path)

print(str(output_path))
