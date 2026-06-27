from .common import blend_tint, lower


def detect_lip_color(text: str):
    t = lower(text)

    if "green" in t:
        return (35, 170, 85)
    if "blue" in t:
        return (45, 85, 230)
    if "black" in t:
        return (35, 30, 35)
    if "red" in t:
        return (210, 30, 45)
    if "pink" in t or "rose" in t:
        return (230, 70, 115)
    if "nude" in t:
        return (180, 100, 90)
    if "berry" in t or "plum" in t:
        return (120, 30, 90)
    if "brown" in t:
        return (130, 65, 45)
    if "peach" in t:
        return (230, 115, 80)

    return (180, 45, 75)


def detect_strength(text: str) -> float:
    t = lower(text)

    strength = 0.72

    if "soft" in t or "natural" in t or "subtle" in t or "light" in t:
        strength *= 0.8

    if "bold" in t or "strong" in t or "bright" in t or "dramatic" in t:
        strength *= 1.15

    return max(0.45, min(0.90, strength))


def render(image, mask, details: str):
    color = detect_lip_color(details)
    strength = detect_strength(details)

    return blend_tint(
        image,
        mask,
        color=color,
        strength=strength,
        tint_amount=0.62,
        blur_radius=0.8,
    ), color, strength