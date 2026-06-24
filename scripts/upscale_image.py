import os
import subprocess
import sys
from pathlib import Path

from PIL import Image, ImageFilter, ImageOps


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


def find_realesrgan_binary(project_root: Path) -> Path | None:
    env_path = os.environ.get("REAL_ESRGAN_NCNN")
    if env_path:
        candidate = Path(env_path)
        if candidate.exists():
            return candidate

    candidate = project_root / "tools/realesrgan-ncnn-vulkan/realesrgan-ncnn-vulkan"
    if candidate.exists():
        return candidate

    return None


def run_realesrgan(binary: Path, input_path: Path, output_path: Path, scale: int) -> bool:
    output_path.parent.mkdir(parents=True, exist_ok=True)

    command = [
        str(binary),
        "-i", str(input_path),
        "-o", str(output_path),
        "-n", "realesrgan-x4plus",
        "-s", str(scale),
        "-f", "png",
    ]

    print("[UPSCALE_REALESRGAN_START]")
    print("command=" + " ".join(command))

    result = subprocess.run(command)

    return (
        result.returncode == 0
        and output_path.exists()
        and output_path.stat().st_size > 0
    )


def run_lanczos_fallback(input_path: Path, output_path: Path, scale: int) -> None:
    print("[UPSCALE_LANCZOS_FALLBACK_START]")
    print(f"input={input_path}")
    print(f"output={output_path}")
    print(f"scale={scale}")

    image = Image.open(input_path)
    image = ImageOps.exif_transpose(image).convert("RGB")

    width, height = image.size
    new_width = width * scale
    new_height = height * scale

    max_side = 4096
    if max(new_width, new_height) > max_side:
        ratio = max_side / max(new_width, new_height)
        new_width = max(1, int(new_width * ratio))
        new_height = max(1, int(new_height * ratio))

    result = image.resize(
        (new_width, new_height),
        Image.Resampling.LANCZOS,
    )

    result = result.filter(
        ImageFilter.UnsharpMask(
            radius=1.2,
            percent=135,
            threshold=3,
        )
    )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    result.save(output_path, "PNG")


if len(sys.argv) < 3:
    fail("Usage: upscale_image.py input output [scale]")

input_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
scale = int(sys.argv[3]) if len(sys.argv) >= 4 else 4

if scale not in (2, 3, 4):
    scale = 4

if not input_path.exists():
    fail(f"Input not found: {input_path}")

project_root = Path(__file__).resolve().parents[1]
binary = find_realesrgan_binary(project_root)

if binary and run_realesrgan(binary, input_path, output_path, scale):
    print("[UPSCALE_REALESRGAN_OK]")
    print(str(output_path))
    sys.exit(0)

run_lanczos_fallback(input_path, output_path, scale)

if not output_path.exists() or output_path.stat().st_size == 0:
    fail("Upscale failed")

print("[UPSCALE_OK]")
print(str(output_path))
