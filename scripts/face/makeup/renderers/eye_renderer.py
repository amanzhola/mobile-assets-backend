from .common import blend_tint, lower


def detect_eye_color(text: str):
    t = lower(text)

    if "gold" in t or "golden" in t:
        return (230, 170, 50)
    if "blue" in t:
        return (45, 85, 200)
    if "green" in t:
        return (55, 150, 90)
    if "pink" in t or "rose" in t:
        return (220, 90, 150)
    if "brown" in t or "bronze" in t:
        return (155, 85, 50)
    if "black" in t or "smokey" in t or "smoky" in t:
        return (40, 35, 40)
    if "purple" in t or "plum" in t:
        return (120, 60, 145)

    return (130, 85, 120)


def detect_strength(text: str) -> float:
    t = lower(text)

    strength = 0.62

    if "soft" in t or "natural" in t or "subtle" in t or "light" in t:
        strength *= 0.82

    if "smokey" in t or "smoky" in t or "bold" in t or "dramatic" in t or "evening" in t:
        strength *= 1.18

    return max(0.38, min(0.80, strength))


def render(image, mask, details: str):
    color = detect_eye_color(details)
    strength = detect_strength(details)

    return blend_tint(
        image,
        mask,
        color=color,
        strength=strength,
        tint_amount=0.52,
        blur_radius=2.5,
    ), color, strength