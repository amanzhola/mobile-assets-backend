# feature/upscale-realesrgan-runner

| Branch                              | Previous                | Next                                 | Main Goal                                                     | Problem Before                                                                   | Main Change                                                       | Added Runner    | Added Script       | Added Tool                     | Updated Core                                                   | Build File       | Result                                               | Back                                                                                  |
| ----------------------------------- | ----------------------- | ------------------------------------ | ------------------------------------------------------------- | -------------------------------------------------------------------------------- | ----------------------------------------------------------------- | --------------- | ------------------ | ------------------------------ | -------------------------------------------------------------- | ---------------- | ---------------------------------------------------- | ------------------------------------------------------------------------------------- |
| `feature/upscale-realesrgan-runner` | `feature/prompt-runner` | `feature/remove-objects-mask-runner` | вынести Upscale Image в отдельный локальный RealESRGAN runner | upscale мог идти через общий SDXL/tool flow, что было медленно и не всегда нужно | добавлен локальный `UpscaleRunner` через `realesrgan-ncnn-vulkan` | `UpscaleRunner` | `upscale_image.py` | `tools/realesrgan-ncnn-vulkan` | `generation_service.h`, `generation_service.cpp`, `.gitignore` | `CMakeLists.txt` | Upscale Image получил быстрый отдельный local runner | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Architecture

| Layer               | До этой ветки                                             | После этой ветки                           | Что стало лучше                        | Почему это важно                       |
| ------------------- | --------------------------------------------------------- | ------------------------------------------ | -------------------------------------- | -------------------------------------- |
| `GenerationService` | мог направлять upscale через общий Comfy/SDXL action flow | делегирует upscale в `UpscaleRunner`       | меньше action-specific кода            | сервис остаётся orchestration layer    |
| Upscale Image       | был частью generic tool logic                             | стал отдельным local runner                | быстрее и стабильнее                   | upscale не обязан использовать SDXL    |
| Local tools         | уже начали выделяться                                     | появился специализированный upscale runner | архитектура стала шире                 | каждый tool получает свой backend      |
| RealESRGAN          | не был подключён как runner                               | добавлен ncnn-vulkan binary/tool           | можно делать реальный upscale локально | не зависит от Kaggle/ComfyUI           |
| `.gitignore`        | не учитывал heavy/runtime artifacts                       | обновлён                                   | меньше мусора в Git                    | бинарники/storage проще контролировать |

---

## Files

| Type    | File                                 | Action   | Purpose                                | Notes                  |
| ------- | ------------------------------------ | -------- | -------------------------------------- | ---------------------- |
| new     | `scripts/upscale_image.py`           | created  | wrapper script for local upscale       | calls RealESRGAN tool  |
| new     | `src/local_tools/upscale_runner.h`   | created  | declaration for Upscale runner         | dedicated local runner |
| new     | `src/local_tools/upscale_runner.cpp` | created  | implementation for Upscale runner      | runs upscale pipeline  |
| new     | `tools/realesrgan-ncnn-vulkan/`      | added    | local RealESRGAN executable and models | binary tool directory  |
| updated | `src/generation_service.h`           | modified | connect `UpscaleRunner` to service     | dependency added       |
| updated | `src/generation_service.cpp`         | modified | delegate upscale flow to runner        | service cleanup        |
| updated | `CMakeLists.txt`                     | modified | add upscale runner source              | required for build     |
| updated | `.gitignore`                         | modified | ignore runtime/heavy generated files   | project hygiene        |

---

## RealESRGAN Tool Structure

| Path                                                                | Purpose                 |
| ------------------------------------------------------------------- | ----------------------- |
| `tools/realesrgan-ncnn-vulkan/realesrgan-ncnn-vulkan`               | executable binary       |
| `tools/realesrgan-ncnn-vulkan/models/realesrgan-x4plus.bin`         | RealESRGAN model data   |
| `tools/realesrgan-ncnn-vulkan/models/realesrgan-x4plus.param`       | RealESRGAN model config |
| `tools/realesrgan-ncnn-vulkan/models/realesrgan-x4plus-anime.bin`   | anime model data        |
| `tools/realesrgan-ncnn-vulkan/models/realesrgan-x4plus-anime.param` | anime model config      |
| `tools/realesrgan-ncnn-vulkan/models/realesr-animevideov3-x2.*`     | anime video x2 model    |
| `tools/realesrgan-ncnn-vulkan/models/realesr-animevideov3-x3.*`     | anime video x3 model    |
| `tools/realesrgan-ncnn-vulkan/models/realesr-animevideov3-x4.*`     | anime video x4 model    |

---

## Flow Comparison

| Flow          | Before                                                                   | After                                                                                    | Output                               |
| ------------- | ------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------- | ------------------------------------ |
| Upscale Image | `GenerationService → ToolActionRunner / SDXL img2img → ComfyUI → output` | `GenerationService → UpscaleRunner → upscale_image.py → realesrgan-ncnn-vulkan → output` | `/outputs/pixo_upscale_image_...png` |
| Prompt        | already extracted                                                        | remains in `PromptRunner`                                                                | `/outputs/pixo_prompt_...png`        |
| Tool actions  | already extracted                                                        | remain in `ToolActionRunner`                                                             | `/outputs/pixo_<action>_...png`      |
| AI Enhancer   | already extracted                                                        | remains in `AiEnhancerRunner`                                                            | `/outputs/pixo_ai_enhancer_...png`   |
| Template      | already extracted                                                        | remains in `TemplateRunner`                                                              | `/outputs/pixo_template_...png`      |

---

## UpscaleRunner Responsibility

| Responsibility         | Description                                   |
| ---------------------- | --------------------------------------------- |
| resolve input image    | finds uploaded image in backend input storage |
| choose output name     | creates `pixo_upscale_image_<task_id>.png`    |
| run local script       | calls `scripts/upscale_image.py`              |
| call RealESRGAN binary | script invokes `realesrgan-ncnn-vulkan`       |
| validate output        | checks file exists and is not empty           |
| return public URL      | returns `/outputs/...` URL                    |
| avoid Comfy dependency | upscale can work without remote GPU           |

---

## Pipeline

| Step | Component                  | Meaning                    |
| ---- | -------------------------- | -------------------------- |
| 1    | uploaded image             | source image from Android  |
| 2    | `UpscaleRunner`            | controls upscale action    |
| 3    | `scripts/upscale_image.py` | Python wrapper             |
| 4    | `realesrgan-ncnn-vulkan`   | local upscaler             |
| 5    | `storage/output`           | stores final image         |
| 6    | API result                 | returns `/outputs/...` URL |

```text
source image
↓
UpscaleRunner
↓
scripts/upscale_image.py
↓
realesrgan-ncnn-vulkan
↓
storage/output
↓
resultImageUrls
```

---

## Responsibility Split

| Component                      | Responsibility                                    | Should NOT do                      |
| ------------------------------ | ------------------------------------------------- | ---------------------------------- |
| `GenerationService`            | route request, update task lifecycle, call runner | contain upscale implementation     |
| `UpscaleRunner`                | execute local upscale flow                        | handle SDXL tools/templates/prompt |
| `scripts/upscale_image.py`     | call RealESRGAN binary and prepare output         | backend task routing               |
| `tools/realesrgan-ncnn-vulkan` | perform image upscale                             | HTTP/API logic                     |
| `OutputService`                | save/serve output                                 | know upscale model details         |
| `ToolActionRunner`             | generic SDXL tools                                | upscale when local runner exists   |

---

## What This Branch Solves

| Problem                                                              | Fix                              | Result                        |
| -------------------------------------------------------------------- | -------------------------------- | ----------------------------- |
| upscale through SDXL was unnecessary for pure resolution improvement | added local RealESRGAN runner    | faster direct upscale         |
| `GenerationService` risked growing again                             | delegated to `UpscaleRunner`     | cleaner service               |
| Comfy/Kaggle dependency was not needed for upscale                   | local ncnn-vulkan tool added     | offline/local capable upscale |
| output handling needed a dedicated path                              | runner saves to `storage/output` | standard result URL           |
| heavy/runtime files needed hygiene                                   | `.gitignore` updated             | cleaner Git state             |

---

## Validation

| Check                        | Expected                                                       |
| ---------------------------- | -------------------------------------------------------------- |
| build                        | `cmake --build .` passes                                       |
| `serverAction=upscale_image` | routes to `UpscaleRunner`                                      |
| script                       | `scripts/upscale_image.py` runs                                |
| binary                       | `tools/realesrgan-ncnn-vulkan/realesrgan-ncnn-vulkan` executes |
| output                       | image appears in `storage/output`                              |
| API result                   | returns `/outputs/pixo_upscale_image_...png`                   |
| ComfyUI                      | not required for local upscale                                 |
| other actions                | unchanged                                                      |

---

## Test Request

| Field            | Value              |
| ---------------- | ------------------ |
| `serverAction`   | `upscale_image`    |
| `toolType`       | `UPSCALE_IMAGE`    |
| `sourceImageUrl` | uploaded image URL |
| `outputCount`    | `1`                |

```bash
RESPONSE=$(curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "serverAction": "upscale_image",
  "toolType": "UPSCALE_IMAGE",
  "sourceImageUrl": "http://192.168.0.177:8080/uploads/YOUR_TEST_IMAGE.jpg",
  "outputCount": 1
}')

echo "$RESPONSE" | jq

TASK_ID=$(echo "$RESPONSE" | jq -r ".taskId")

watch -n 3 "curl -s http://localhost:8080/generations/$TASK_ID | jq"
```

---

## Local Tool Install Notes

| Step             | Command / Path                                        |
| ---------------- | ----------------------------------------------------- |
| binary directory | `tools/realesrgan-ncnn-vulkan/`                       |
| executable       | `tools/realesrgan-ncnn-vulkan/realesrgan-ncnn-vulkan` |
| models           | `tools/realesrgan-ncnn-vulkan/models/`                |
| script           | `scripts/upscale_image.py`                            |
| output directory | `storage/output/`                                     |

---

## Commands

| Step     | Command                                                                                                                         |
| -------- | ------------------------------------------------------------------------------------------------------------------------------- |
| checkout | `git checkout feature/upscale-realesrgan-runner`                                                                                |
| build    | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                           |
| run      | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend` |
| commit   | `git add . && git commit -m "Add RealESRGAN upscale runner"`                                                                    |
| push     | `git push -u origin feature/upscale-realesrgan-runner`                                                                          |

---

## Final State

| Item                           | Status                               |
| ------------------------------ | ------------------------------------ |
| `UpscaleRunner`                | added                                |
| `scripts/upscale_image.py`     | added                                |
| `tools/realesrgan-ncnn-vulkan` | added                                |
| `upscale_image` flow           | extracted                            |
| `GenerationService`            | cleaner                              |
| ComfyUI dependency for upscale | removed                              |
| output URL format              | preserved                            |
| Prompt runner                  | preserved                            |
| ToolAction runner              | preserved                            |
| next branch                    | `feature/remove-objects-mask-runner` |

---

## Back

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
