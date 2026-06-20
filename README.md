# 🧽 feature/remove-objects-auto-mask

| Branch                             | Parent                      | Goal                                            | Main Result                                                 | Local Tool        | Mask             | Inpaint          | ComfyUI  | Android            | Back                                                                                  |
| ---------------------------------- | --------------------------- | ----------------------------------------------- | ----------------------------------------------------------- | ----------------- | ---------------- | ---------------- | -------- | ------------------ | ------------------------------------------------------------------------------------- |
| `feature/remove-objects-auto-mask` | `feature/local-tool-runner` | сделать честный Remove Objects без ручной маски | `prompt → CLIPSeg mask → OpenCV CPU inpaint → /outputs/...` | `LocalToolRunner` | auto text-guided | CPU OpenCV TELEA | not used | No Android changes | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## 🌳 Tree

```text
mobile-assets-backend/
├── scripts/
│   ├── remove_background.py
│   └── remove_objects.py
├── src/
│   ├── local_tools/
│   │   ├── local_tool_runner.h
│   │   └── local_tool_runner.cpp
│   ├── generation_service.cpp
│   └── generation_service.h
├── storage/
│   ├── input/
│   └── output/
└── README.md
```

---

## ✅ What was done

| #  | Area            | Before                              | After                                 | Result                         |
| -- | --------------- | ----------------------------------- | ------------------------------------- | ------------------------------ |
| 1  | Remove Objects  | fake SDXL/tool prompt behavior risk | local text-guided object removal      | honest implementation          |
| 2  | Mask            | Android user did not draw mask      | backend builds mask automatically     | no Android mask UI needed      |
| 3  | Segmentation    | no object mask                      | CLIPSeg `text + image → mask`         | prompt-guided mask             |
| 4  | Inpaint         | planned LaMa/FLUX idea              | CPU OpenCV `cv2.inpaint`              | stable WSL execution           |
| 5  | Russian prompt  | `зонтик` failed                     | RU→EN normalization                   | common Russian words work      |
| 6  | CUDA issue      | `SimpleLama()` tried CUDA           | removed LaMa dependency path          | works without NVIDIA           |
| 7  | LocalToolRunner | only `RunRemoveBackground()`        | added `RunRemoveObjects()`            | local tools architecture grows |
| 8  | Output          | no local remove object result       | `/outputs/pixo_remove_objects_...png` | Android Result works           |
| 9  | Debug           | no mask artifact                    | saves debug mask as `*_mask.png`      | easier diagnosis               |
| 10 | ComfyUI         | not needed for this tool            | bypassed                              | faster and simpler             |

---

## 🔁 Remove Objects pipeline

| Step | Component         | Action                                         | Result                                |
| ---- | ----------------- | ---------------------------------------------- | ------------------------------------- |
| 1    | Android           | sends `serverAction=remove_objects` + `prompt` | backend receives request              |
| 2    | GenerationService | extracts first uploaded file                   | input filename                        |
| 3    | GenerationService | calls `local_tool_runner_.RunRemoveObjects()`  | local tool path                       |
| 4    | LocalToolRunner   | reads `prompt`, `objectText`, or `removeText`  | object text                           |
| 5    | Python script     | CLIPSeg builds object mask                     | grayscale mask                        |
| 6    | Python script     | expands/blurs/thresholds mask                  | usable inpaint mask                   |
| 7    | Python script     | OpenCV TELEA inpaint                           | object removed                        |
| 8    | OutputService     | builds public URL                              | `/outputs/pixo_remove_objects_...png` |
| 9    | Android           | polls task                                     | completed result                      |

---

## 🧠 Why not FLUX Fill yet

| Point                      | Explanation                                                                       |
| -------------------------- | --------------------------------------------------------------------------------- |
| FLUX Fill needs mask       | inpainting/outpainting still needs an edited region                               |
| Android does not draw mask | backend must generate mask automatically first                                    |
| First working approach     | CLIPSeg text mask + CPU inpaint                                                   |
| Future upgrade             | replace CPU OpenCV with FLUX Fill / LaMa / ComfyUI inpaint after mask is reliable |

---

## 🧩 LocalToolRunner API

| Method                  | Purpose                                     |
| ----------------------- | ------------------------------------------- |
| `RunRemoveBackground()` | local `rembg` background removal            |
| `RunRemoveObjects()`    | local CLIPSeg + OpenCV object removal       |
| `ReadOptionString()`    | read values from `options`                  |
| `ReadStringOrEmpty()`   | read top-level request string like `prompt` |

---

## 🗣 Prompt sources

| Priority | Field                | Example               |
| -------- | -------------------- | --------------------- |
| 1        | `prompt`             | `umbrella`            |
| 2        | `options.objectText` | `person in red shirt` |
| 3        | `options.removeText` | `logo`                |

---

## 🌍 Russian → English normalization

| Russian        | English     |
| -------------- | ----------- |
| `зонтик`       | `umbrella`  |
| `зонт`         | `umbrella`  |
| `человек`      | `person`    |
| `люди`         | `people`    |
| `машина`       | `car`       |
| `авто`         | `car`       |
| `водяной знак` | `watermark` |
| `логотип`      | `logo`      |
| `текст`        | `text`      |
| `провод`       | `wire`      |
| `провода`      | `wires`     |
| `мусор`        | `trash`     |
| `сумка`        | `bag`       |
| `стул`         | `chair`     |
| `стол`         | `table`     |

---

## 📂 Main files

| File                                    | Purpose                                             |
| --------------------------------------- | --------------------------------------------------- |
| `scripts/remove_objects.py`             | CLIPSeg mask + OpenCV CPU inpaint                   |
| `src/local_tools/local_tool_runner.h`   | adds `RunRemoveObjects()` declaration               |
| `src/local_tools/local_tool_runner.cpp` | runs remove objects script and returns output URL   |
| `src/generation_service.cpp`            | adds `server_action == "remove_objects"` local path |
| `.venv-tools/`                          | Python environment with ML dependencies             |

---

## 🐍 Python dependencies

| Package                  | Purpose                                                |
| ------------------------ | ------------------------------------------------------ |
| `transformers`           | CLIPSeg model loading                                  |
| `torch`                  | model inference                                        |
| `torchvision`            | PyTorch vision support                                 |
| `pillow`                 | image loading/saving/filtering                         |
| `numpy`                  | mask array processing                                  |
| `opencv-python`          | CPU inpainting                                         |
| `simple-lama-inpainting` | attempted earlier, but CPU-safe final path uses OpenCV |

---

## 🧪 Direct script test

```bash
source ~/mobile-assets-backend/.venv-tools/bin/activate

python3 scripts/remove_objects.py \
  storage/input/img_9504081960506_360b977a04f31f9a.jpg \
  /tmp/remove_objects_test.png \
  "зонтик" \
  0.20

file /tmp/remove_objects_test.png
file /tmp/remove_objects_test_mask.png
```

---

## 🧪 Backend test

```bash
RESPONSE=$(curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "serverAction":"remove_objects",
  "toolType":"REMOVE_OBJECTS",
  "sourceImageUrl":"http://192.168.0.177:8080/uploads/img_9504081960506_360b977a04f31f9a.jpg",
  "prompt":"зонтик",
  "outputCount":1
}')

echo "$RESPONSE" | jq

TASK_ID=$(echo "$RESPONSE" | jq -r '.taskId')

watch -n 3 "curl -s http://localhost:8080/generations/$TASK_ID | jq"
```

---

## ✅ Expected response

| Field                | Expected                                                        |
| -------------------- | --------------------------------------------------------------- |
| `status`             | `completed`                                                     |
| `progressPercent`    | `100`                                                           |
| `resultImageUrls[0]` | `http://192.168.0.177:8080/outputs/pixo_remove_objects_....png` |

---

## 🧨 Problems fixed

| Problem                 | Cause                                             | Fix                                  |
| ----------------------- | ------------------------------------------------- | ------------------------------------ |
| `зонтик` failed         | CLIPSeg prompt needed English                     | RU→EN dictionary                     |
| `SimpleLama()` failed   | attempted CUDA on WSL without NVIDIA              | replaced with OpenCV CPU inpaint     |
| empty mask              | CLIPSeg score too weak                            | normalized logits + lower threshold  |
| weak mask edges         | raw mask too small                                | MaxFilter + GaussianBlur + threshold |
| hard diagnosis          | no mask saved                                     | output debug `*_mask.png`            |
| fake remove object risk | SDXL prompt alone does not remove target reliably | mask-driven inpaint path             |

---

## ⚠️ Known limitations

| Limitation                    | Meaning                             | Future fix                                |
| ----------------------------- | ----------------------------------- | ----------------------------------------- |
| CLIPSeg mask may be imperfect | object text may select wrong region | better segmentation / SAM                 |
| OpenCV inpaint is basic       | texture may be rough                | LaMa / FLUX Fill / ComfyUI inpaint        |
| no manual mask                | user cannot refine selection        | add mask UI later                         |
| no multi-object UI yet        | prompt can describe one/few objects | add better prompt parser                  |
| no FLUX Fill yet              | inpaint model not connected         | use auto mask as input to FLUX Fill later |

---

## 🔜 Next stage

| Branch idea                         | Goal                                                |
| ----------------------------------- | --------------------------------------------------- |
| `feature/remove-objects-flux-fill`  | use auto mask with FLUX Fill / ComfyUI inpainting   |
| `feature/remove-objects-sam-mask`   | replace CLIPSeg mask with SAM/grounded segmentation |
| `feature/local-tool-runner-cleanup` | split local tools into separate runners             |

---

## 🧾 Git

| Step          | Command                                                                                                                                  |
| ------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| create branch | `git checkout -b feature/remove-objects-auto-mask`                                                                                       |
| add           | `git add scripts/remove_objects.py src/local_tools/local_tool_runner.cpp src/local_tools/local_tool_runner.h src/generation_service.cpp` |
| commit        | `git commit -m "Add CPU text-guided remove objects runner"`                                                                              |
| push          | `git push -u origin feature/remove-objects-auto-mask`                                                                                    |

---

## 🏁 Final result

| Capability                                        | Status |
| ------------------------------------------------- | ------ |
| Remove Objects works without Android mask drawing | ✅      |
| Backend creates auto mask from text               | ✅      |
| Russian common prompts normalized                 | ✅      |
| CPU inpaint works in WSL                          | ✅      |
| ComfyUI not required for this tool                | ✅      |
| Output saved to `/outputs/...`                    | ✅      |
| Android receives completed task                   | ✅      |
| Honest non-fake remove objects path               | ✅      |

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
