# feature/template-runner

| Branch                    | Previous                     | Next                         | Main Goal                                           | Problem Before                                                                                  | Main Change                                                                                    | Added Runner     | Updated Core                                                            | Build File       | Result                                         | Back                                                                                  |
| ------------------------- | ---------------------------- | ---------------------------- | --------------------------------------------------- | ----------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------- | ---------------- | ----------------------------------------------------------------------- | ---------------- | ---------------------------------------------- | ------------------------------------------------------------------------------------- |
| `feature/template-runner` | `feature/ai-enhancer-runner` | `feature/tool-action-runner` | вынести Template generation flow в отдельный runner | template-логика всё ещё жила внутри `GenerationService` и смешивалась с single-image Comfy flow | создан отдельный `TemplateRunner`, а `LocalToolRunner` оставлен временным helper для auto-mask | `TemplateRunner` | `generation_service.h`, `generation_service.cpp`, `local_tool_runner.*` | `CMakeLists.txt` | Template pipeline стал отдельным action runner | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Architecture

| Layer               | До этой ветки                                                 | После этой ветки                              | Что стало лучше               | Почему это важно                                     |
| ------------------- | ------------------------------------------------------------- | --------------------------------------------- | ----------------------------- | ---------------------------------------------------- |
| `GenerationService` | сам обрабатывал template action внутри общего generation flow | делегирует template action в `TemplateRunner` | меньше template-specific кода | сервис остаётся orchestration layer                  |
| Template generation | смешивался с generic Comfy flow                               | получил отдельный runner                      | проще менять template logic   | templates требуют отдельной логики с template assets |
| `LocalToolRunner`   | постепенно становился свалкой helpers                         | временно оставлен только для auto-mask helper | меньше ответственности        | позже можно переименовать в `ObjectMaskRunner`       |
| Template assets     | использовались в общем flow                                   | template runner отвечает за template flow     | логика ближе к action         | проще развивать template metadata                    |
| CMake               | не знал про new runner                                        | добавлен `template_runner.cpp`                | сборка включает runner        | ветка собирается штатно                              |

---

## Files

| Type    | File                                    | Action   | Purpose                                                                   | Notes                               |
| ------- | --------------------------------------- | -------- | ------------------------------------------------------------------------- | ----------------------------------- |
| updated | `src/local_tools/local_tool_runner.h`   | modified | оставить временный helper для auto-mask                                   | больше не должен быть общей свалкой |
| updated | `src/local_tools/local_tool_runner.cpp` | modified | сократить роль LocalToolRunner                                            | later rename candidate              |
| new     | `src/local_tools/template_runner.h`     | created  | declaration for Template runner                                           | dedicated template action           |
| new     | `src/local_tools/template_runner.cpp`   | created  | implementation for Template runner                                        | template generation pipeline        |
| updated | `src/generation_service.h`              | modified | connect `TemplateRunner` to service                                       | dependency added                    |
| updated | `src/generation_service.cpp`            | modified | delegate template flow from `RunSingleImageViaComfy` / `CreateGeneration` | service cleanup                     |
| updated | `CMakeLists.txt`                        | modified | add template runner source                                                | required for build                  |

---

## Flow Comparison

| Flow                   | Before                                                                          | After                                                                           | Output                                              |
| ---------------------- | ------------------------------------------------------------------------------- | ------------------------------------------------------------------------------- | --------------------------------------------------- |
| Template               | `GenerationService → template logic → BuildTemplateWorkflow → ComfyUI → output` | `GenerationService → TemplateRunner → BuildTemplateWorkflow → ComfyUI → output` | `/outputs/pixo_template_...png`                     |
| AI Enhancer            | already extracted                                                               | remains in `AiEnhancerRunner`                                                   | `/outputs/pixo_ai_enhancer_...png`                  |
| Remove Objects         | already extracted                                                               | remains in `RemoveObjectsRunner`                                                | `/outputs/final_pixo_remove_objects_...png`         |
| Remove Background      | already extracted                                                               | remains in background runner                                                    | `/outputs/pixo_remove_background_...png`            |
| Remove Objects Cleanup | already extracted                                                               | remains in cleanup runner                                                       | `/outputs/final_pixo_remove_objects_cleanup_...png` |

---

## Template Runner Responsibility

| Responsibility            | Description                                           |
| ------------------------- | ----------------------------------------------------- |
| read template request     | handles `serverAction=template`                       |
| resolve `templateId`      | reads selected template id from request               |
| load template prompt      | uses backend template prompt mapping                  |
| resolve template denoise  | keeps template-specific denoise logic outside service |
| prepare template workflow | builds template ComfyUI workflow                      |
| use template asset        | connects selected template image/background           |
| queue ComfyUI             | sends workflow to ComfyUI                             |
| save output               | saves generated result into `storage/output`          |
| return public URL         | returns `/outputs/...` URL                            |

---

## Pipeline

| Step | Component                 | Meaning                          |
| ---- | ------------------------- | -------------------------------- |
| 1    | uploaded user image       | source image from Android        |
| 2    | `templateId`              | selected template                |
| 3    | `TemplateRunner`          | controls template action         |
| 4    | template asset cache      | resolves template image          |
| 5    | `BuildTemplateWorkflow()` | builds ComfyUI JSON              |
| 6    | ComfyUI                   | blends/generates template result |
| 7    | `storage/output`          | stores output image              |
| 8    | API result                | returns `/outputs/...` URL       |

```text
user image
+
templateId
↓
TemplateRunner
↓
template asset / prompt / workflow
↓
ComfyUI
↓
storage/output
↓
resultImageUrls
```

---

## LocalToolRunner Cleanup

| Before                                                     | After                         | Reason                             |
| ---------------------------------------------------------- | ----------------------------- | ---------------------------------- |
| `LocalToolRunner` risked becoming a generic dumping ground | kept as temporary helper only | avoid another god-object           |
| template logic could be added into LocalToolRunner         | moved into `TemplateRunner`   | template is its own action         |
| auto-mask helper still needed                              | left temporarily              | later rename to `ObjectMaskRunner` |
| many unrelated tools in one helper                         | separated into runners        | consistent architecture            |

---

## Responsibility Split

| Component              | Responsibility                                | Should NOT do                                |
| ---------------------- | --------------------------------------------- | -------------------------------------------- |
| `GenerationService`    | route request, update task state, call runner | contain template implementation              |
| `TemplateRunner`       | execute template generation pipeline          | handle AI Enhancer / Remove Objects / Prompt |
| `LocalToolRunner`      | temporary helper for auto-mask only           | become a general tool container              |
| `TemplateAssetService` | provide template asset/cache                  | run ComfyUI                                  |
| `WorkflowBuilder`      | build workflow JSON                           | download template assets                     |
| `ComfyClient`          | queue/wait/download ComfyUI result            | decide business action                       |
| `OutputService`        | save and serve final output                   | know template semantics                      |

---

## What This Branch Solves

| Problem                                                 | Fix                                             | Result                          |
| ------------------------------------------------------- | ----------------------------------------------- | ------------------------------- |
| template flow was inside `GenerationService`            | moved to `TemplateRunner`                       | cleaner service                 |
| `LocalToolRunner` was growing too broad                 | reduced role and avoided putting template there | better separation               |
| template action needs own asset/prompt/denoise behavior | dedicated runner owns it                        | easier future template metadata |
| future 24 templates would complicate service            | template logic isolated                         | safer scaling                   |
| CMake did not include runner                            | added new source                                | normal build                    |

---

## Validation

| Check                   | Expected                                |
| ----------------------- | --------------------------------------- |
| build                   | `cmake --build .` passes                |
| `serverAction=template` | routes to `TemplateRunner`              |
| `templateId`            | read correctly                          |
| template workflow       | built correctly                         |
| ComfyUI                 | receives valid workflow                 |
| result                  | saved in `storage/output`               |
| API result              | returns `/outputs/pixo_template_...png` |
| other actions           | unchanged                               |

---

## Test Request

| Field            | Value                                            |
| ---------------- | ------------------------------------------------ |
| `serverAction`   | `template`                                       |
| `toolType`       | `TEMPLATE`                                       |
| `templateId`     | existing template id, for example `travel_style` |
| `sourceImageUrl` | uploaded image URL                               |
| `outputCount`    | `1`                                              |

```bash
RESPONSE=$(curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "serverAction": "template",
  "toolType": "TEMPLATE",
  "templateId": "travel_style",
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
| checkout | `git checkout feature/template-runner`                                                                                          |
| build    | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                           |
| run      | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend` |
| commit   | `git add . && git commit -m "Extract template runner"`                                                                          |
| push     | `git push -u origin feature/template-runner`                                                                                    |

---

## Final State

| Item                 | Status                       |
| -------------------- | ---------------------------- |
| `TemplateRunner`     | added                        |
| template flow        | extracted                    |
| `GenerationService`  | cleaner                      |
| `LocalToolRunner`    | reduced role                 |
| template asset logic | preserved                    |
| template workflow    | preserved                    |
| ComfyUI execution    | preserved                    |
| output URL format    | preserved                    |
| next branch          | `feature/tool-action-runner` |

---

## Back

| Link        | URL                                                                                                                                              |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------ |
| Main README | [https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |
