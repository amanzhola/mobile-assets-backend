from .common import blend_tint, lower


def detect_blush_color(text: str):
    t = lower(text)

    if "soft pink" in t or "light pink" in t:
        return (255, 120, 155)
    if "red" in t:
        return (230, 50, 65)
    if "pink" in t or "rose" in t or "rosy" in t:
        return (245, 80, 125)
    if "peach" in t:
        return (245, 120, 80)
    if "nude" in t or "natural" in t:
        return (225, 105, 95)
    if "bronze" in t or "bronzer" in t:
        return (185, 95, 55)

    return (245, 90, 130)


def detect_strength(text: str) -> float:
    t = lower(text)

    strength = 0.58

    if "soft" in t or "gentle" in t or "natural" in t or "subtle" in t or "light" in t:
        strength *= 0.82

    if "red" in t or "bright" in t or "strong" in t or "bold" in t:
        strength *= 1.18

    return max(0.38, min(0.75, strength))


def render(image, mask, details: str):
    color = detect_blush_color(details)
    strength = detect_strength(details)

    return blend_tint(
        image,
        mask,
        color=color,
        strength=strength,
        tint_amount=0.46,
        blur_radius=16,
    ), color, strength