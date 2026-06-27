from .common import blend_tint, lower


def detect_brow_color(text: str):
    t = lower(text)

    if "green" in t:
        return (35, 130, 70)
    if "blue" in t:
        return (45, 80, 190)
    if "black" in t:
        return (55, 45, 40)
    if "brown" in t:
        return (95, 55, 35)
    if "dark" in t:
        return (45, 40, 40)
    if "red" in t:
        return (150, 45, 45)
    if "blonde" in t or "gold" in t or "golden" in t:
        return (170, 125, 55)
    if "pink" in t or "rose" in t:
        return (170, 70, 110)

    return (70, 50, 45)


def detect_strength(text: str) -> float:
    t = lower(text)

    strength = 0.72

    if "soft" in t or "natural" in t or "subtle" in t or "light" in t:
        strength *= 0.75

    if "bold" in t or "strong" in t or "bright" in t or "dark" in t:
        strength *= 1.15

    return max(0.40, min(0.88, strength))


def render(image, mask, details: str):
    color = detect_brow_color(details)
    strength = detect_strength(details)

    return blend_tint(
        image,
        mask,
        color=color,
        strength=strength,
        tint_amount=0.48,
        blur_radius=0.8,
    ), color, strength
