# feature/skin-improve-runner

| Branch                        | Previous                            | Next                        | Status                             | Commit                               | Main Goal                                          | Problem Before                                                                               | Main Change                          | Added Runner        | Updated Router           | Updated Core                                     | Build File       | Result                                    | Back                                                                                  |
| ----------------------------- | ----------------------------------- | --------------------------- | ---------------------------------- | ------------------------------------ | -------------------------------------------------- | -------------------------------------------------------------------------------------------- | ------------------------------------ | ------------------- | ------------------------ | ------------------------------------------------ | ---------------- | ----------------------------------------- | ------------------------------------------------------------------------------------- |
| `feature/skin-improve-runner` | `feature/template-workflow-cleanup` | `feature/smile-edit-runner` | ✅ Finished / ✅ Compiles / ✅ Pushed | `Extract skin improve action runner` | вынести Skin Improve из общего `GenerationService` | `GenerationService` знал детали Skin Improve workflow, upload, queue, wait, download, output | создан отдельный `SkinImproveRunner` | `SkinImproveRunner` | `GenerationActionRouter` | `generation_service.h`, `generation_service.cpp` | `CMakeLists.txt` | Skin Improve стал отдельным Action Runner | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Goal

| До                                                           | После                                                            | Главная идея                                                                 |
| ------------------------------------------------------------ | ---------------------------------------------------------------- | ---------------------------------------------------------------------------- |
| `GenerationService → RunGenerationViaComfy() → skin_improve` | `GenerationService → GenerationActionRouter → SkinImproveRunner` | `GenerationService` больше не знает, как технически выполняется Skin Improve |

---

## Files

| Type    | File                                          | Action   | Purpose                                   | Notes                      |
| ------- | --------------------------------------------- | -------- | ----------------------------------------- | -------------------------- |
| new     | `src/action_runners/skin_improve_runner.h`    | created  | declaration for Skin Improve runner       | dedicated action runner    |
| new     | `src/action_runners/skin_improve_runner.cpp`  | created  | implementation for Skin Improve runner    | full Skin Improve pipeline |
| updated | `src/generation/generation_action_router.h`   | modified | add `SkinImproveRunner` dependency        | router integration         |
| updated | `src/generation/generation_action_router.cpp` | modified | route `serverAction=skin_improve`         | action dispatch            |
| updated | `src/generation_service.h`                    | modified | add runner field / constructor dependency | service integration        |
| updated | `src/generation_service.cpp`                  | modified | construct/pass runner through router      | service cleanup            |
| updated | `CMakeLists.txt`                              | modified | add runner source                         | required for build         |

---

## Architecture

| Layer                    | До этой ветки                           | После этой ветки                   | Что стало лучше             | Почему это важно                  |
| ------------------------ | --------------------------------------- | ---------------------------------- | --------------------------- | --------------------------------- |
| `GenerationService`      | содержал детали Skin Improve execution  | только вызывает router             | меньше action-specific кода | service становится координатором  |
| `GenerationActionRouter` | не знал про Skin Improve runner         | маршрутизирует `skin_improve`      | routing централизован       | новый action подключён правильно  |
| Skin Improve             | был внутри общего Comfy/generation flow | получил отдельный runner           | проще тестировать и менять  | изоляция action logic             |
| Workflow execution       | была смешана с service                  | внутри runner                      | меньше связности            | легче отлаживать                  |
| Progress                 | был частью общего flow                  | runner обновляет progress callback | понятные стадии             | Android видит нормальный прогресс |

---

## SkinImproveRunner Responsibility

| Responsibility          | Description                        |
| ----------------------- | ---------------------------------- |
| read request            | получает `json::object` запроса    |
| read uploaded image     | находит входное изображение        |
| choose enhancement mode | выбирает режим улучшения           |
| build workflow          | собирает workflow для Skin Improve |
| upload image            | отправляет input в ComfyUI         |
| queue prompt            | запускает ComfyUI prompt           |
| wait output             | ждёт результат                     |
| download output         | скачивает output                   |
| save output             | сохраняет в `storage/output`       |
| return public URL       | возвращает `/outputs/...`          |

---

## Pipeline

| Step | Component                | Meaning                          |
| ---- | ------------------------ | -------------------------------- |
| 1    | Android request          | `serverAction=skin_improve`      |
| 2    | `GenerationService`      | создаёт task и передаёт в router |
| 3    | `GenerationActionRouter` | выбирает `SkinImproveRunner`     |
| 4    | `SkinImproveRunner`      | выполняет весь Skin Improve flow |
| 5    | `WorkflowBuilder`        | собирает workflow                |
| 6    | `ComfyClient`            | queue / wait / download          |
| 7    | `OutputService`          | сохраняет результат              |
| 8    | API response             | task получает completed result   |

```text
GenerationService
↓
GenerationActionRouter
↓
SkinImproveRunner
↓
Upload image
↓
Build workflow
↓
Queue ComfyUI
↓
Wait output
↓
Download
↓
Save output
↓
Return public URL
```

---

## Progress

| Stage           | Progress |
| --------------- | -------: |
| start           |      `1` |
| input resolved  |     `10` |
| upload image    |     `20` |
| workflow built  |     `30` |
| prompt queued   |     `40` |
| output received |     `85` |
| downloaded      |     `92` |
| saved           |     `95` |
| completed       |    `100` |

---

## Logs

| Log                           | Meaning                      |
| ----------------------------- | ---------------------------- |
| `[SKIN_IMPROVE_RUNNER_START]` | runner started               |
| `[SKIN_IMPROVE_UPLOAD]`       | input image upload stage     |
| `[SKIN_IMPROVE_WORKFLOW]`     | workflow build stage         |
| `[SKIN_IMPROVE_QUEUE]`        | ComfyUI queue stage          |
| `[SKIN_IMPROVE_DOWNLOAD]`     | output download stage        |
| `[SKIN_IMPROVE_SUCCESS]`      | action completed             |
| `[SKIN_IMPROVE_EXCEPTION]`    | action failed with exception |

---

## What Was Removed From GenerationService

| Removed Detail                   | New Owner           |
| -------------------------------- | ------------------- |
| Skin Improve workflow details    | `SkinImproveRunner` |
| Skin Improve upload logic        | `SkinImproveRunner` |
| Skin Improve queue logic         | `SkinImproveRunner` |
| Skin Improve wait/download logic | `SkinImproveRunner` |
| Skin Improve output handling     | `SkinImproveRunner` |
| Skin Improve progress steps      | `SkinImproveRunner` |

---

## Router Integration

| Item                  | Added                                                |
| --------------------- | ---------------------------------------------------- |
| include               | `#include "../action_runners/skin_improve_runner.h"` |
| field                 | `SkinImproveRunner& skin_improve_runner_`            |
| constructor parameter | `SkinImproveRunner& skin_improve_runner`             |
| initializer           | `skin_improve_runner_(skin_improve_runner)`          |
| route branch          | `if (server_action == "skin_improve")`               |
| call                  | `skin_improve_runner_.Run(...)`                      |

---

## GenerationService Integration

| Item              | Added / Updated                                                |
| ----------------- | -------------------------------------------------------------- |
| field             | `SkinImproveRunner skin_improve_runner_`                       |
| constructor init  | `skin_improve_runner_(...)`                                    |
| router injection  | pass `skin_improve_runner_` into `GenerationActionRouter`      |
| service role      | task lifecycle only                                            |
| removed knowledge | workflow / prompt / queue / download / upload / output details |

---

## API Compatibility

| Item             | Status                  |
| ---------------- | ----------------------- |
| request format   | unchanged               |
| Android frontend | unchanged               |
| `serverAction`   | still `skin_improve`    |
| response format  | unchanged               |
| task status      | standard flow           |
| output URL       | standard `/outputs/...` |

Example request remains:

```json
{
  "serverAction": "skin_improve"
}
```

---

## Responsibility Split

| Component                | Responsibility                                | Should NOT do                 |
| ------------------------ | --------------------------------------------- | ----------------------------- |
| `GenerationService`      | task lifecycle, persistence, progress wrapper | know Skin Improve internals   |
| `GenerationActionRouter` | route `skin_improve` to runner                | execute workflow              |
| `SkinImproveRunner`      | execute Skin Improve pipeline                 | handle HTTP/API/task storage  |
| `ComfyClient`            | communicate with ComfyUI                      | decide action routing         |
| `WorkflowBuilder`        | build workflow JSON                           | queue prompt                  |
| `OutputService`          | save/serve output                             | know enhancement mode details |

---

## What This Branch Solves

| Problem                                                 | Fix                            | Result                        |
| ------------------------------------------------------- | ------------------------------ | ----------------------------- |
| Skin Improve lived in `GenerationService`               | moved to `SkinImproveRunner`   | cleaner service               |
| workflow/upload/download logic was mixed with task flow | runner owns execution          | better separation             |
| progress stages were not action-local                   | runner owns progress callbacks | clearer logs                  |
| future Skin Improve changes would touch service         | now touch runner only          | safer iteration               |
| architecture needed consistency with other runners      | added action runner            | same pattern as other actions |

---

## Validation

| Check                       | Expected                                |
| --------------------------- | --------------------------------------- |
| build                       | `cmake --build .` passes                |
| branch                      | pushed to GitHub                        |
| `serverAction=skin_improve` | routes through `GenerationActionRouter` |
| runner                      | `SkinImproveRunner::Run(...)` is called |
| progress                    | reaches `100`                           |
| result                      | saved in `storage/output`               |
| API result                  | returns normal completed task           |
| frontend                    | no changes required                     |

---

## Test Request

| Field            | Value              |
| ---------------- | ------------------ |
| `serverAction`   | `skin_improve`     |
| `toolType`       | `SKIN_IMPROVE`     |
| `sourceImageUrl` | uploaded image URL |
| `outputCount`    | `1`                |

```bash
RESPONSE=$(curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "serverAction": "skin_improve",
  "toolType": "SKIN_IMPROVE",
  "sourceImageUrl": "http://192.168.0.177:8080/uploads/YOUR_TEST_IMAGE.jpg",
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
| checkout | `git checkout feature/skin-improve-runner`                                                                                      |
| build    | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                           |
| run      | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend` |
| commit   | `git add . && git commit -m "Extract skin improve action runner"`                                                               |
| push     | `git push -u origin feature/skin-improve-runner`                                                                                |

---

## Final State

| Item                     | Status                      |
| ------------------------ | --------------------------- |
| `SkinImproveRunner`      | added                       |
| Skin Improve flow        | extracted                   |
| `GenerationService`      | cleaner                     |
| `GenerationActionRouter` | updated                     |
| progress callbacks       | implemented                 |
| logs                     | added                       |
| API                      | unchanged                   |
| frontend                 | unchanged                   |
| build                    | passes                      |
| pushed                   | yes                         |
| next branch              | `feature/smile-edit-runner` |

---

## Architecture After This Branch

| Layer                    | Runners                                                                                                                                                                                       |
| ------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `GenerationService`      | coordinates task lifecycle                                                                                                                                                                    |
| `GenerationActionRouter` | routes actions                                                                                                                                                                                |
| `src/action_runners/`    | `RemoveBackgroundRunner`, `RemoveObjectsRunner`, `RemoveObjectsCleanupRunner`, `TemplateRunner`, `PromptRunner`, `AiEnhancerRunner`, `SkinImproveRunner`, `UpscaleRunner`, `ToolActionRunner` |

```text
GenerationService
↓
GenerationActionRouter
├── RemoveBackgroundRunner
├── RemoveObjectsRunner
├── RemoveObjectsCleanupRunner
├── TemplateRunner
├── PromptRunner
├── AiEnhancerRunner
├── SkinImproveRunner
├── UpscaleRunner
└── ToolActionRunner
```

---

## Back

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
