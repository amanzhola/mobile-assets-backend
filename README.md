# feature/tool-action-runner

| Branch                       | Previous                  | Next                    | Main Goal                                               | Problem Before                                                                                              | Main Change                                                             | Added Runner       | Removed From Service                                      | Updated Core                                     | Build File       | Result                                       | Back                                                                                  |
| ---------------------------- | ------------------------- | ----------------------- | ------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------- | ------------------ | --------------------------------------------------------- | ------------------------------------------------ | ---------------- | -------------------------------------------- | ------------------------------------------------------------------------------------- |
| `feature/tool-action-runner` | `feature/template-runner` | `feature/prompt-runner` | вынести общий SDXL tool actions flow в отдельный runner | `GenerationService` всё ещё содержал generic `RunSingleImageViaComfy()` и `FindNewestComfyOutputByPrefix()` | создан `ToolActionRunner`, а single-image Comfy logic убрана из service | `ToolActionRunner` | `RunSingleImageViaComfy`, `FindNewestComfyOutputByPrefix` | `generation_service.h`, `generation_service.cpp` | `CMakeLists.txt` | tool actions получили отдельный общий runner | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Architecture

| Layer                  | До этой ветки                                | После этой ветки                              | Что стало лучше                 | Почему это важно                 |
| ---------------------- | -------------------------------------------- | --------------------------------------------- | ------------------------------- | -------------------------------- |
| `GenerationService`    | содержал общий single-image Comfy execution  | больше не содержит `RunSingleImageViaComfy()` | service стал меньше             | меньше god-object                |
| Tool actions           | обрабатывались generic веткой внутри service | переданы в `ToolActionRunner`                 | общий flow изолирован           | легче развивать 10 tools         |
| Output prefix fallback | был внутри service                           | перенесён в runner / action layer             | output safety ближе к execution | меньше cross-task ошибок         |
| Workflow choice        | смешивался с routing                         | runner отвечает за tool workflow              | логика ближе к action           | проще менять `tool_img2img.json` |
| CMake                  | не знал про runner                           | добавлен new cpp                              | сборка включает runner          | ветка собирается штатно          |

---

## Files

| Type    | File                                     | Action   | Purpose                                             | Notes                                                           |
| ------- | ---------------------------------------- | -------- | --------------------------------------------------- | --------------------------------------------------------------- |
| new     | `src/local_tools/tool_action_runner.h`   | created  | declaration for common SDXL tool action runner      | shared tools runner                                             |
| new     | `src/local_tools/tool_action_runner.cpp` | created  | implementation for tool action execution            | ComfyUI img2img flow                                            |
| updated | `src/generation_service.h`               | modified | removed old declarations                            | no `RunSingleImageViaComfy`, no `FindNewestComfyOutputByPrefix` |
| updated | `src/generation_service.cpp`             | modified | removed old implementations and delegates to runner | service cleanup                                                 |
| updated | `CMakeLists.txt`                         | modified | add tool action runner source                       | required for build                                              |

---

## Removed From GenerationService

| Removed Item                                                    | Why Removed                                             | New Owner                                |
| --------------------------------------------------------------- | ------------------------------------------------------- | ---------------------------------------- |
| `std::optional<std::string> RunSingleImageViaComfy(...)`        | too generic and action-specific at the same time        | `ToolActionRunner` and dedicated runners |
| `std::optional<std::string> FindNewestComfyOutputByPrefix(...)` | output recovery belongs to Comfy/action execution layer | runner layer                             |
| generic tool Comfy branch                                       | made service too large                                  | `ToolActionRunner`                       |
| workflow execution details                                      | service should route, not execute                       | action runners                           |

---

## ToolActionRunner Responsibility

| Responsibility               | Description                                      |
| ---------------------------- | ------------------------------------------------ |
| detect supported tool action | handles shared SDXL tool actions                 |
| prepare input image          | uses uploaded image file                         |
| build positive prompt        | uses tool prompt builder logic                   |
| resolve denoise              | applies per-tool denoise values                  |
| build workflow               | uses `tool_img2img.json`                         |
| queue ComfyUI                | sends workflow to ComfyUI                        |
| protect output assignment    | validates output prefix / newest matching output |
| save final result            | stores image in `storage/output`                 |
| return public URL            | returns `/outputs/...` URL                       |

---

## Supported Tool Actions

| Action              | Route                      | Workflow                                | Runner                   |
| ------------------- | -------------------------- | --------------------------------------- | ------------------------ |
| `ghibli`            | SDXL img2img               | `workflows/tool_img2img.json`           | `ToolActionRunner`       |
| `ghostface`         | SDXL img2img               | `workflows/tool_img2img.json`           | `ToolActionRunner`       |
| `glam_makeup`       | SDXL img2img at this stage | `workflows/tool_img2img.json`           | `ToolActionRunner`       |
| `remove_objects`    | already separate runner    | `workflows/remove_objects_inpaint.json` | `RemoveObjectsRunner`    |
| `remove_background` | local runner               | local script                            | `RemoveBackgroundRunner` |
| `skin_improve`      | SDXL/img2img at this stage | `workflows/tool_img2img.json`           | `ToolActionRunner`       |
| `upscale_image`     | tool flow at this stage    | `workflows/tool_img2img.json`           | `ToolActionRunner`       |
| `change_scene`      | SDXL img2img               | `workflows/tool_img2img.json`           | `ToolActionRunner`       |
| `hair_studio`       | SDXL img2img               | `workflows/tool_img2img.json`           | `ToolActionRunner`       |
| `smile_edit`        | SDXL/img2img at this stage | `workflows/tool_img2img.json`           | `ToolActionRunner`       |

---

## Flow Comparison

| Flow                | Before                                                                              | After                                                                         | Output                                      |
| ------------------- | ----------------------------------------------------------------------------------- | ----------------------------------------------------------------------------- | ------------------------------------------- |
| Generic tool action | `GenerationService → RunSingleImageViaComfy → BuildToolWorkflow → ComfyUI → output` | `GenerationService → ToolActionRunner → BuildToolWorkflow → ComfyUI → output` | `/outputs/pixo_<action>_...png`             |
| AI Enhancer         | already extracted                                                                   | remains in `AiEnhancerRunner`                                                 | `/outputs/pixo_ai_enhancer_...png`          |
| Template            | already extracted                                                                   | remains in `TemplateRunner`                                                   | `/outputs/pixo_template_...png`             |
| Remove Objects      | already extracted                                                                   | remains in `RemoveObjectsRunner`                                              | `/outputs/final_pixo_remove_objects_...png` |
| Prompt              | not yet extracted                                                                   | next branch                                                                   | later `PromptRunner`                        |

---

## Pipeline

| Step | Component                     | Meaning                       |
| ---- | ----------------------------- | ----------------------------- |
| 1    | uploaded source image         | user input image              |
| 2    | `serverAction`                | selected tool action          |
| 3    | `ToolActionRunner`            | controls shared tool action   |
| 4    | prompt builder                | creates positive prompt       |
| 5    | denoise resolver              | selects tool-specific denoise |
| 6    | `BuildToolWorkflow()`         | builds ComfyUI workflow       |
| 7    | `workflows/tool_img2img.json` | shared SDXL workflow          |
| 8    | ComfyUI                       | generates edited image        |
| 9    | output prefix validation      | prevents wrong task output    |
| 10   | `storage/output`              | stores final image            |
| 11   | API result                    | returns `/outputs/...` URL    |

```text
source image
+
serverAction
↓
ToolActionRunner
↓
prompt + denoise + workflow
↓
ComfyUI
↓
output prefix validation
↓
storage/output
↓
resultImageUrls
```

---

## Responsibility Split

| Component                | Responsibility                           | Should NOT do                                        |
| ------------------------ | ---------------------------------------- | ---------------------------------------------------- |
| `GenerationService`      | route request, update tasks, call runner | contain generic Comfy single-image implementation    |
| `ToolActionRunner`       | execute shared SDXL tool actions         | handle AI Enhancer / Template / Prompt special flows |
| `AiEnhancerRunner`       | AI Enhancer only                         | generic tools                                        |
| `TemplateRunner`         | Template only                            | generic tools                                        |
| `RemoveObjectsRunner`    | auto object removal                      | generic SDXL tools                                   |
| `RemoveBackgroundRunner` | local background removal                 | Comfy img2img                                        |
| `WorkflowBuilder`        | build workflow JSON                      | run task lifecycle                                   |
| `ComfyClient`            | HTTP communication with ComfyUI          | decide action semantics                              |

---

## What This Branch Solves

| Problem                                                       | Fix                                | Result                            |
| ------------------------------------------------------------- | ---------------------------------- | --------------------------------- |
| `GenerationService` still had large single-image Comfy method | removed `RunSingleImageViaComfy()` | smaller service                   |
| output prefix recovery lived in wrong layer                   | removed from service               | runner owns output safety         |
| 10 tool actions needed common execution                       | added `ToolActionRunner`           | shared tool path                  |
| each new tool risked adding more conditions to service        | runner absorbs tool execution      | scalable architecture             |
| future prompt runner needed service cleanup first             | generic tool path extracted        | ready for `feature/prompt-runner` |

---

## Validation

| Check               | Expected                              |
| ------------------- | ------------------------------------- |
| build               | `cmake --build .` passes              |
| tool actions        | route through `ToolActionRunner`      |
| workflow            | uses `workflows/tool_img2img.json`    |
| output prefix       | validated / recovered by runner layer |
| AI Enhancer         | still works through its own runner    |
| Template            | still works through its own runner    |
| Remove Objects      | still works through its own runner    |
| Remove Background   | still works through local runner      |
| API response format | unchanged                             |

---

## Test Request

| Field            | Value                       |
| ---------------- | --------------------------- |
| `serverAction`   | for example `ghibli`        |
| `toolType`       | matching Android tool type  |
| `sourceImageUrl` | uploaded image URL          |
| `prompt`         | optional/user action prompt |
| `outputCount`    | `1`                         |

```bash
RESPONSE=$(curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "serverAction": "ghibli",
  "toolType": "GHIBLI",
  "sourceImageUrl": "http://192.168.0.177:8080/uploads/YOUR_TEST_IMAGE.jpg",
  "prompt": "Studio Ghibli anime style, beautiful colors",
  "outputCount": 1
}')

echo "$RESPONSE" | jq

TASK_ID=$(echo "$RESPONSE" | jq -r ".taskId")

watch -n 3 "curl -s http://localhost:8080/generations/$TASK_ID | jq"
```

---

## Commands

| Step     | Command                                                                                                                         |
| -------- | ------------------------------------------------------------------------------------------------------------------------------- |
| checkout | `git checkout feature/tool-action-runner`                                                                                       |
| build    | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                           |
| run      | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend` |
| commit   | `git add . && git commit -m "Extract tool action runner"`                                                                       |
| push     | `git push -u origin feature/tool-action-runner`                                                                                 |

---

## Final State

| Item                              | Status                           |
| --------------------------------- | -------------------------------- |
| `ToolActionRunner`                | added                            |
| generic SDXL tool action flow     | extracted                        |
| `RunSingleImageViaComfy()`        | removed from `GenerationService` |
| `FindNewestComfyOutputByPrefix()` | removed from `GenerationService` |
| `GenerationService`               | cleaner                          |
| AI Enhancer runner                | preserved                        |
| Template runner                   | preserved                        |
| Remove Objects runner             | preserved                        |
| Remove Background runner          | preserved                        |
| next branch                       | `feature/prompt-runner`          |

---

## Back

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
