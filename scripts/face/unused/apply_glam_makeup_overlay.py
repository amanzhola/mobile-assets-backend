import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter, ImageOps


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


def lower(text: str) -> str:
    return (text or "").strip().lower()


def detect_color(text: str, region: str):
    t = lower(text)

    if "green" in t:
        return (45, 170, 95)
    if "blue" in t:
        return (45, 90, 230)
    if "gold" in t or "golden" in t:
        return (230, 175, 55)
    if "soft pink" in t or "light pink" in t or "baby pink" in t:
        return (255, 145, 175)
    if "pink" in t or "rose" in t or "rosy" in t:
        return (245, 105, 145)
    if "nude" in t:
        return (190, 110, 95)
    if "red" in t:
        return (220, 35, 55)
    if "plum" in t or "berry" in t:
        return (125, 35, 105)
    if "brown" in t:
        return (135, 75, 45)
    if "peach" in t:
        return (250, 135, 95)
    if "black" in t or "smokey" in t or "smoky" in t:
        return (45, 40, 45)

    if region == "cheeks":
        return (245, 115, 145)

    if region == "lips":
        return (190, 55, 85)

    if region == "eyelids" or region == "eyes":
        return (95, 70, 90)

    return (220, 100, 130)


def detect_strength(text: str, region: str) -> float:
    t = lower(text)

    if region == "cheeks":
        strength = 0.48
    elif region == "lips":
        strength = 0.68
    elif region == "eyelids" or region == "eyes":
        strength = 0.58
    else:
        strength = 0.38

    if "soft" in t or "gentle" in t or "natural" in t or "subtle" in t or "light" in t:
        strength *= 0.9

    if "rich" in t or "strong" in t or "bold" in t or "evening" in t or "dramatic" in t:
        strength *= 1.25

    if "blue" in t or "green" in t or "red" in t:
        strength *= 1.18

    return max(0.25, min(0.85, strength))


def apply_overlay(image_rgb: Image.Image, mask_rgba: Image.Image, color, strength: float, region: str):
    image = np.array(image_rgb).astype(np.float32)
    mask_alpha = np.array(mask_rgba.convert("RGBA"))[:, :, 3]

    # create_face_region_mask: alpha=0 means edit area.
    edit = np.where(mask_alpha < 128, 255, 0).astype(np.uint8)

    blur_radius = 10
    if region == "lips":
        blur_radius = 4
    elif region == "eyelids" or region == "eyes":
        blur_radius = 5
    elif region == "cheeks":
        blur_radius = 14

    mask = Image.fromarray(edit, mode="L")
    mask = mask.filter(ImageFilter.GaussianBlur(radius=blur_radius))

    mask_np = np.array(mask).astype(np.float32) / 255.0
    mask_np = np.clip(mask_np * strength, 0.0, 1.0)[..., None]

    overlay = np.zeros_like(image)
    overlay[:, :, 0] = color[0]
    overlay[:, :, 1] = color[1]
    overlay[:, :, 2] = color[2]

    base = image / 255.0
    blend = overlay / 255.0

    # More visible and still natural:
    # lips = stronger normal tint
    # cheeks = tint + soft light
    # eyes = darker cosmetic tint
    if region == "lips":
        mixed = image * (1.0 - mask_np) + overlay * mask_np
    elif region == "eyelids" or region == "eyes":
        cosmetic = image * 0.55 + overlay * 0.45
        mixed = image * (1.0 - mask_np) + cosmetic * mask_np
    else:
        soft = np.where(
            base < 0.5,
            2.0 * base * blend,
            1.0 - 2.0 * (1.0 - base) * (1.0 - blend)
        ) * 255.0

        tint = image * 0.65 + overlay * 0.35
        cosmetic = soft * 0.45 + tint * 0.55
        mixed = image * (1.0 - mask_np) + cosmetic * mask_np

    mixed = np.clip(mixed, 0, 255).astype(np.uint8)
    return Image.fromarray(mixed, mode="RGB")


if len(sys.argv) < 6:
    fail("Usage: apply_glam_makeup_overlay.py input_image mask_rgba output_image region english_details")

input_path = Path(sys.argv[1])
mask_path = Path(sys.argv[2])
output_path = Path(sys.argv[3])
region = lower(sys.argv[4])
english_details = sys.argv[5]

if not input_path.exists():
    fail(f"Input not found: {input_path}")

if not mask_path.exists():
    fail(f"Mask not found: {mask_path}")

image = Image.open(input_path)
image = ImageOps.exif_transpose(image).convert("RGB")

mask = Image.open(mask_path).convert("RGBA").resize(
    image.size,
    Image.Resampling.NEAREST
)

color = detect_color(english_details, region)
strength = detect_strength(english_details, region)

result = apply_overlay(image, mask, color, strength, region)

output_path.parent.mkdir(parents=True, exist_ok=True)
result.save(output_path)

print(str(output_path))
print(f"region={region}")
print(f"color={color}")
print(f"strength={strength}")