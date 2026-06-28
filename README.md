# feature/prompt-runner

| Branch                  | Previous                     | Next                                | Main Goal                                         | Problem Before                                                           | Main Change                                                | Added Runner   | Added Script               | Updated Core                                     | Build File       | Result                              | Back                                                                                  |
| ----------------------- | ---------------------------- | ----------------------------------- | ------------------------------------------------- | ------------------------------------------------------------------------ | ---------------------------------------------------------- | -------------- | -------------------------- | ------------------------------------------------ | ---------------- | ----------------------------------- | ------------------------------------------------------------------------------------- |
| `feature/prompt-runner` | `feature/tool-action-runner` | `feature/upscale-realesrgan-runner` | вынести Prompt generation flow в отдельный runner | prompt flow ещё не был отделён и мог снова раздувать `GenerationService` | создан `PromptRunner` и добавлен script для prompt collage | `PromptRunner` | `create_prompt_collage.py` | `generation_service.h`, `generation_service.cpp` | `CMakeLists.txt` | Prompt стал отдельным action runner | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Architecture

| Layer               | До этой ветки                                               | После этой ветки                          | Что стало лучше                  | Почему это важно                     |
| ------------------- | ----------------------------------------------------------- | ----------------------------------------- | -------------------------------- | ------------------------------------ |
| `GenerationService` | мог обрабатывать prompt logic внутри общего generation flow | делегирует prompt action в `PromptRunner` | меньше prompt-specific кода      | service остаётся orchestration layer |
| Prompt generation   | не имел отдельного runner                                   | появился `PromptRunner`                   | prompt flow изолирован           | проще развивать 1–5 images           |
| Multi-image input   | не имел отдельной подготовки                                | добавлен `create_prompt_collage.py`       | можно собрать входную композицию | подготовка к real multi-image SDXL   |
| Tool actions        | уже вынесены в `ToolActionRunner`                           | не затронуты                              | стабильность сохраняется         | меньше риска регрессий               |
| CMake               | не знал про prompt runner                                   | добавлен new cpp                          | сборка включает runner           | ветка собирается штатно              |

---

## Files

| Type    | File                                | Action   | Purpose                               | Notes                            |
| ------- | ----------------------------------- | -------- | ------------------------------------- | -------------------------------- |
| new     | `scripts/create_prompt_collage.py`  | created  | create image collage for prompt input | prepares multi-image prompt base |
| new     | `src/local_tools/prompt_runner.h`   | created  | declaration for Prompt runner         | dedicated prompt action          |
| new     | `src/local_tools/prompt_runner.cpp` | created  | implementation for Prompt runner      | prompt generation flow           |
| updated | `src/generation_service.h`          | modified | connect `PromptRunner` to service     | dependency added                 |
| updated | `src/generation_service.cpp`        | modified | delegate prompt flow to runner        | service cleanup                  |
| updated | `CMakeLists.txt`                    | modified | add prompt runner source              | required for build               |

---

## Flow Comparison

| Flow                        | Before                                                 | After                                                         | Output                             |
| --------------------------- | ------------------------------------------------------ | ------------------------------------------------------------- | ---------------------------------- |
| Prompt with one image       | `GenerationService → generic prompt handling → output` | `GenerationService → PromptRunner → prompt workflow → output` | `/outputs/pixo_prompt_...png`      |
| Prompt with multiple images | no clean runner / no clear composition layer           | `PromptRunner → create_prompt_collage.py → workflow → output` | `/outputs/pixo_prompt_...png`      |
| Tool actions                | already extracted                                      | still handled by `ToolActionRunner`                           | `/outputs/pixo_<action>_...png`    |
| AI Enhancer                 | already extracted                                      | still handled by `AiEnhancerRunner`                           | `/outputs/pixo_ai_enhancer_...png` |
| Template                    | already extracted                                      | still handled by `TemplateRunner`                             | `/outputs/pixo_template_...png`    |

---

## PromptRunner Responsibility

| Responsibility        | Description                                          |
| --------------------- | ---------------------------------------------------- |
| read prompt request   | handles prompt generation requests                   |
| collect source images | supports one or multiple uploaded images             |
| prepare input image   | uses original image or creates collage               |
| build positive prompt | uses user prompt as generation instruction           |
| call ComfyUI          | runs prompt workflow through existing Comfy pipeline |
| save output           | stores generated image in `storage/output`           |
| return public URL     | returns `/outputs/...` URL                           |
| preserve API contract | Android request/response format remains unchanged    |

---

## Prompt Collage Script

| Script                             | Purpose                                             | Input               | Output              | Why Needed                                                                               |
| ---------------------------------- | --------------------------------------------------- | ------------------- | ------------------- | ---------------------------------------------------------------------------------------- |
| `scripts/create_prompt_collage.py` | combine several prompt images into one input canvas | 1–5 uploaded images | composed image file | SDXL img2img receives one image input, so multiple Android images need composition first |

---

## Pipeline

| Step | Component                  | Meaning                                       |
| ---- | -------------------------- | --------------------------------------------- |
| 1    | Android Prompt Screen      | user sends prompt + one/more images           |
| 2    | backend request            | `serverAction=prompt`                         |
| 3    | `PromptRunner`             | controls prompt action                        |
| 4    | `create_prompt_collage.py` | creates composition when several images exist |
| 5    | workflow builder           | builds Comfy workflow                         |
| 6    | ComfyUI                    | generates prompt result                       |
| 7    | `storage/output`           | stores final image                            |
| 8    | API result                 | returns `/outputs/...` URL                    |

```text
prompt text
+
1–5 images
↓
PromptRunner
↓
optional collage
↓
ComfyUI
↓
storage/output
↓
resultImageUrls
```

---

## Responsibility Split

| Component                  | Responsibility                                    | Should NOT do                         |
| -------------------------- | ------------------------------------------------- | ------------------------------------- |
| `GenerationService`        | route request, update task lifecycle, call runner | contain prompt implementation         |
| `PromptRunner`             | execute prompt generation flow                    | handle tools/templates/remove objects |
| `create_prompt_collage.py` | prepare multi-image canvas                        | backend routing                       |
| `ToolActionRunner`         | generic tools only                                | prompt composition                    |
| `TemplateRunner`           | templates only                                    | prompt action                         |
| `WorkflowBuilder`          | build workflow JSON                               | parse Android navigation              |
| `ComfyClient`              | send workflow / wait output                       | build prompt text                     |

---

## What This Branch Solves

| Problem                                                | Fix                       | Result                              |
| ------------------------------------------------------ | ------------------------- | ----------------------------------- |
| Prompt flow could make `GenerationService` large again | added `PromptRunner`      | service remains clean               |
| multiple prompt images needed a preparation layer      | added collage script      | prompt can accept multi-image input |
| prompt action was not symmetrical with other actions   | runner added              | architecture becomes consistent     |
| future prompt composition would be hard in service     | isolated in runner/script | easier next stages                  |
| CMake did not include runner                           | added source              | normal build                        |

---

## Validation

| Check                 | Expected                              |
| --------------------- | ------------------------------------- |
| build                 | `cmake --build .` passes              |
| `serverAction=prompt` | routes to `PromptRunner`              |
| one image prompt      | generates standard output             |
| multiple image prompt | creates/uses collage input            |
| result                | saved in `storage/output`             |
| API result            | returns `/outputs/pixo_prompt_...png` |
| other actions         | unchanged                             |
| Android API           | unchanged                             |

---

## Test Request

| Field             | Value                                    |
| ----------------- | ---------------------------------------- |
| `serverAction`    | `prompt`                                 |
| `toolType`        | `PROMPT` or matching Android prompt type |
| `prompt`          | user text prompt                         |
| `sourceImageUrl`  | first uploaded image URL                 |
| `sourceImageUrls` | optional array for multiple images       |
| `outputCount`     | `1`                                      |

```bash
RESPONSE=$(curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "serverAction": "prompt",
  "toolType": "PROMPT",
  "prompt": "Create a cinematic edited image with natural lighting",
  "sourceImageUrl": "http://192.168.0.177:8080/uploads/YOUR_TEST_IMAGE.jpg",
  "outputCount": 1
}')

echo "$RESPONSE" | jq

TASK_ID=$(echo "$RESPONSE" | jq -r ".taskId")

watch -n 3 "curl -s http://localhost:8080/generations/$TASK_ID | jq"
```

---

## Multi Image Concept

| Input Count | Behavior                                        |
| ----------- | ----------------------------------------------- |
| 1 image     | use image directly                              |
| 2 images    | create two-image collage                        |
| 3 images    | create compact composition                      |
| 4 images    | create grid/contact sheet                       |
| 5 images    | create multi-image prompt canvas                |
| 0 images    | invalid / fallback depending request validation |

---

## Commands

| Step     | Command                                                                                                                         |
| -------- | ------------------------------------------------------------------------------------------------------------------------------- |
| checkout | `git checkout feature/prompt-runner`                                                                                            |
| build    | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                           |
| run      | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend` |
| commit   | `git add . && git commit -m "Extract prompt runner"`                                                                            |
| push     | `git push -u origin feature/prompt-runner`                                                                                      |

---

## Final State

| Item                           | Status                              |
| ------------------------------ | ----------------------------------- |
| `PromptRunner`                 | added                               |
| `create_prompt_collage.py`     | added                               |
| prompt flow                    | extracted                           |
| `GenerationService`            | cleaner                             |
| multi-image prompt preparation | started                             |
| AI Enhancer runner             | preserved                           |
| ToolAction runner              | preserved                           |
| Template runner                | preserved                           |
| Remove Objects runners         | preserved                           |
| next branch                    | `feature/upscale-realesrgan-runner` |

---

## Back

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
