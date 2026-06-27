import sys
from pathlib import Path
from rembg import remove
from PIL import Image

if len(sys.argv) < 4:
    print("Usage: remove_background.py input output mode", file=sys.stderr)
    sys.exit(1)

input_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
mode = sys.argv[3].strip().lower()

img = Image.open(input_path).convert("RGBA")
cut = remove(img).convert("RGBA")

output_path.parent.mkdir(parents=True, exist_ok=True)

if mode == "white":
    bg = Image.new("RGBA", cut.size, (255, 255, 255, 255))
    bg.alpha_composite(cut)
    bg.convert("RGB").save(output_path, format="PNG")
else:
    cut.save(output_path, format="PNG")