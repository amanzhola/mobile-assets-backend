import sys
from pathlib import Path

import cv2
import mediapipe as mp
import numpy as np
from PIL import Image, ImageFilter, ImageOps

from mediapipe.tasks import python
from mediapipe.tasks.python import vision


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


if len(sys.argv) < 4:
    fail("Usage: create_face_region_mask.py input_image output_mask region1 [region2 ...]")


input_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
regions = [value.strip().lower() for value in sys.argv[3:] if value.strip()]

if not input_path.exists():
    fail(f"Input not found: {input_path}")

if not regions:
    fail("No regions provided")


project_root = Path(__file__).resolve().parents[3]
model_path = project_root / "models" / "mediapipe" / "face_landmarker.task"

if not model_path.exists():
    fail(f"FaceLandmarker model not found: {model_path}")


image = Image.open(input_path)
image = ImageOps.exif_transpose(image).convert("RGB")
rgb = np.array(image)
height, width = rgb.shape[:2]


LIPS = [
    61, 185, 40, 39, 37, 0, 267, 269, 270, 409, 291,
    375, 321, 405, 314, 17, 84, 181, 91, 146
]

LEFT_EYE = [
    33, 7, 163, 144, 145, 153, 154, 155, 133,
    173, 157, 158, 159, 160, 161, 246
]

RIGHT_EYE = [
    263, 249, 390, 373, 374, 380, 381, 382, 362,
    398, 384, 385, 386, 387, 388, 466
]

LEFT_EYEBROW = [
    70, 63, 105, 66, 107,
    55, 65, 52, 53, 46
]

RIGHT_EYEBROW = [
    336, 296, 334, 293, 300,
    276, 283, 282, 295, 285
]

FACE_OVAL = [
    10, 338, 297, 332, 284, 251, 389, 356,
    454, 323, 361, 288, 397, 365, 379, 378,
    400, 377, 152, 148, 176, 149, 150, 136,
    172, 58, 132, 93, 234, 127, 162, 21,
    54, 103, 67, 109
]


def points_from_indices(landmarks, indices):
    result = []

    for index in indices:
        lm = landmarks[index]
        x = int(round(lm.x * width))
        y = int(round(lm.y * height))
        result.append([x, y])

    return np.array(result, dtype=np.int32)


def fill_polygon(mask, points, value=255):
    if len(points) >= 3:
        cv2.fillConvexPoly(mask, points, value)


def fill_region(mask, landmarks, region):
    region = region.lower()

    if region == "lips":
        pts = points_from_indices(landmarks, LIPS)
        fill_polygon(mask, cv2.convexHull(pts), 255)
        return

    if region == "eyelids":
        left_eye = points_from_indices(landmarks, LEFT_EYE)
        right_eye = points_from_indices(landmarks, RIGHT_EYE)
        left_brow = points_from_indices(landmarks, LEFT_EYEBROW)
        right_brow = points_from_indices(landmarks, RIGHT_EYEBROW)

        left = np.concatenate([left_eye, left_brow], axis=0)
        right = np.concatenate([right_eye, right_brow], axis=0)

        fill_polygon(mask, cv2.convexHull(left), 255)
        fill_polygon(mask, cv2.convexHull(right), 255)
        return

    if region == "eyebrows":
    	left_brow = points_from_indices(landmarks, LEFT_EYEBROW)
    	right_brow = points_from_indices(landmarks, RIGHT_EYEBROW)

    	brow_zone = np.zeros_like(mask)

    	fill_polygon(brow_zone, cv2.convexHull(left_brow), 255)
    	fill_polygon(brow_zone, cv2.convexHull(right_brow), 255)

    	brow_zone = cv2.dilate(
        	brow_zone,
        	np.ones((5, 5), dtype=np.uint8),
        	iterations=1
    	)

    	gray = cv2.cvtColor(rgb, cv2.COLOR_RGB2GRAY)

    	dark_pixels = np.where(gray < 125, 255, 0).astype(np.uint8)

    	mask[(brow_zone > 0) & (dark_pixels > 0)] = 255
    	return

    if region == "eyes":
        left_eye = points_from_indices(landmarks, LEFT_EYE)
        right_eye = points_from_indices(landmarks, RIGHT_EYE)

        fill_polygon(mask, cv2.convexHull(left_eye), 255)
        fill_polygon(mask, cv2.convexHull(right_eye), 255)
        return

    if region == "cheeks":
        face_pts = points_from_indices(landmarks, FACE_OVAL)
        x, y, w, h = cv2.boundingRect(face_pts)

        left_center = (
            int(x + w * 0.30),
            int(y + h * 0.58)
        )

        right_center = (
            int(x + w * 0.70),
            int(y + h * 0.58)
        )

        axes = (
            max(14, int(w * 0.18)),
            max(14, int(h * 0.12))
        )

        cv2.ellipse(mask, left_center, axes, 0, 0, 360, 255, -1)
        cv2.ellipse(mask, right_center, axes, 0, 0, 360, 255, -1)
        return

    if region == "skin" or region == "face":
        face_pts = points_from_indices(landmarks, FACE_OVAL)
        fill_polygon(mask, cv2.convexHull(face_pts), 255)

        remove = np.zeros_like(mask)
        fill_region(remove, landmarks, "lips")
        fill_region(remove, landmarks, "eyes")

        mask[remove > 0] = 0
        return

    if region == "face contour":
        face_pts = points_from_indices(landmarks, FACE_OVAL)

        contour = np.zeros_like(mask)
        fill_polygon(contour, cv2.convexHull(face_pts), 255)

        kernel_size = max(3, width // 80)

        eroded = cv2.erode(
            contour,
            np.ones((kernel_size, kernel_size), dtype=np.uint8),
            iterations=2
        )

        mask[(contour > 0) & (eroded == 0)] = 255
        return


base_options = python.BaseOptions(
    model_asset_path=str(model_path)
)

options = vision.FaceLandmarkerOptions(
    base_options=base_options,
    running_mode=vision.RunningMode.IMAGE,
    num_faces=1,
    min_face_detection_confidence=0.5,
    min_face_presence_confidence=0.5,
    min_tracking_confidence=0.5,
)

mp_image = mp.Image(
    image_format=mp.ImageFormat.SRGB,
    data=rgb
)

with vision.FaceLandmarker.create_from_options(options) as landmarker:
    result = landmarker.detect(mp_image)


if not result.face_landmarks:
    fail("No face detected")


landmarks = result.face_landmarks[0]
mask = np.zeros((height, width), dtype=np.uint8)

for region in regions:
    fill_region(mask, landmarks, region)


if int((mask > 0).sum()) == 0:
    fail(f"Mask is empty for regions: {regions}")


mask_img = Image.fromarray(mask, mode="L")

if "lips" in regions:
    mask_img = mask_img.filter(ImageFilter.MaxFilter(3))
    mask_img = mask_img.filter(ImageFilter.GaussianBlur(radius=0.8))
    mask_img = mask_img.point(lambda p: 255 if p > 18 else 0)

elif "cheeks" in regions:
    mask_img = mask_img.filter(ImageFilter.GaussianBlur(radius=10))
    mask_img = mask_img.point(lambda p: 255 if p > 16 else 0)

elif "eyelids" in regions or "eyes" in regions:
    mask_img = mask_img.filter(ImageFilter.GaussianBlur(radius=2.5))
    mask_img = mask_img.point(lambda p: 255 if p > 32 else 0)

elif "eyebrows" in regions:
    mask_img = mask_img.filter(ImageFilter.MaxFilter(3))
    mask_img = mask_img.filter(ImageFilter.GaussianBlur(radius=0.7))
    mask_img = mask_img.point(lambda p: 255 if p > 20 else 0)

else:
    mask_img = mask_img.filter(ImageFilter.GaussianBlur(radius=4))
    mask_img = mask_img.point(lambda p: 255 if p > 32 else 0)


mask_np = np.array(mask_img)

alpha = np.where(mask_np > 0, 0, 255).astype(np.uint8)

rgba = np.zeros((height, width, 4), dtype=np.uint8)
rgba[:, :, 3] = alpha

output_path.parent.mkdir(parents=True, exist_ok=True)

Image.fromarray(rgba, mode="RGBA").save(output_path)

debug_path = output_path.with_name(
    output_path.stem + "_debug_white_region.png"
)

Image.fromarray(mask_np, mode="L").save(debug_path)

print(str(output_path))
print("debug_mask=" + str(debug_path))