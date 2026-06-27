import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter, ImageOps


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


def lower(text: str) -> str:
    return (text or "").strip().lower()


def load_rgb(path: Path) -> Image.Image:
    return ImageOps.exif_transpose(Image.open(path)).convert("RGB")


def load_mask(path: Path, size) -> Image.Image:
    return Image.open(path).convert("RGBA").resize(
        size,
        Image.Resampling.NEAREST
    )


def edit_mask(mask_rgba: Image.Image, blur_radius: float) -> np.ndarray:
    mask_alpha = np.array(mask_rgba.convert("RGBA"))[:, :, 3]
    edit = np.where(mask_alpha < 128, 255, 0).astype(np.uint8)

    mask = Image.fromarray(edit, mode="L")
    mask = mask.filter(ImageFilter.GaussianBlur(radius=blur_radius))

    return np.array(mask).astype(np.float32) / 255.0


def blend_tint(
    image_rgb: Image.Image,
    mask_rgba: Image.Image,
    color,
    strength: float,
    tint_amount: float,
    blur_radius: float,
) -> Image.Image:
    image = np.array(image_rgb).astype(np.float32)

    mask_np = edit_mask(mask_rgba, blur_radius)
    mask_np = np.clip(mask_np * strength, 0.0, 1.0)[..., None]

    overlay = np.zeros_like(image)
    overlay[:, :, 0] = color[0]
    overlay[:, :, 1] = color[1]
    overlay[:, :, 2] = color[2]

    cosmetic = image * (1.0 - tint_amount) + overlay * tint_amount
    mixed = image * (1.0 - mask_np) + cosmetic * mask_np
    mixed = np.clip(mixed, 0, 255).astype(np.uint8)

    return Image.fromarray(mixed, mode="RGB")
