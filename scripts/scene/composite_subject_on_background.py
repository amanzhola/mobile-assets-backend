import sys
from pathlib import Path

from PIL import Image, ImageOps, ImageFilter


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


if len(sys.argv) < 4:
    fail("Usage: composite_subject_on_background.py subject_rgba background output")

subject_path = Path(sys.argv[1])
background_path = Path(sys.argv[2])
output_path = Path(sys.argv[3])

if not subject_path.exists():
    fail(f"Subject not found: {subject_path}")

if not background_path.exists():
    fail(f"Background not found: {background_path}")

subject = Image.open(subject_path)
subject = ImageOps.exif_transpose(subject).convert("RGBA")

background = Image.open(background_path)
background = ImageOps.exif_transpose(background).convert("RGB").resize(
    subject.size,
    Image.Resampling.LANCZOS
).convert("RGBA")

alpha = subject.getchannel("A")

# Чуть смягчаем волосы/край, но не создаём старую обводку.
alpha = alpha.filter(ImageFilter.GaussianBlur(radius=0.6))
subject.putalpha(alpha)

background.alpha_composite(subject)

output_path.parent.mkdir(parents=True, exist_ok=True)
background.convert("RGB").save(output_path)

print(str(output_path))
