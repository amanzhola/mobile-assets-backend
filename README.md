# feature/ai-enhancer-runner

| Branch                       | Previous                        | Next                      | Main Goal                                   | Problem Before                                                                                    | Main Change                         | Added Runner       | Updated Core                                     | Build File       | Result                                                               | Back                                                                                  |
| ---------------------------- | ------------------------------- | ------------------------- | ------------------------------------------- | ------------------------------------------------------------------------------------------------- | ----------------------------------- | ------------------ | ------------------------------------------------ | ---------------- | -------------------------------------------------------------------- | ------------------------------------------------------------------------------------- |
| `feature/ai-enhancer-runner` | `feature/remove-objects-runner` | `feature/template-runner` | вынести AI Enhancer flow в отдельный runner | `GenerationService` всё ещё содержал отдельную логику AI Enhancer внутри `RunSingleImageViaComfy` | создан отдельный `AiEnhancerRunner` | `AiEnhancerRunner` | `generation_service.h`, `generation_service.cpp` | `CMakeLists.txt` | AI Enhancer стал самостоятельным runner, а generation flow стал чище | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Architecture

| Layer               | До этой ветки                                       | После этой ветки                            | Что стало лучше             | Почему это важно                    |
| ------------------- | --------------------------------------------------- | ------------------------------------------- | --------------------------- | ----------------------------------- |
| `GenerationService` | сам собирал AI Enhancer workflow и запускал ComfyUI | делегирует AI Enhancer в `AiEnhancerRunner` | меньше action-specific кода | проще сопровождать разные tools     |
| AI Enhancer         | был частью общего `RunSingleImageViaComfy`          | стал отдельным runner                       | workflow изолирован         | можно менять enhancer отдельно      |
| ComfyUI workflow    | выбирался внутри общего метода                      | выбирается внутри runner                    | логика ближе к action       | меньше условий в сервисе            |
| Output handling     | смешивался с общим flow                             | переносится в runner-level flow             | проще отлаживать output     | меньше риска сломать другие actions |
| CMake               | не знал про runner                                  | добавлен new cpp                            | сборка включает runner      | ветка собирается штатно             |

---

## Files

| Type    | File                                     | Action   | Purpose                                        | Notes                   |
| ------- | ---------------------------------------- | -------- | ---------------------------------------------- | ----------------------- |
| new     | `src/local_tools/ai_enhancer_runner.h`   | created  | declaration for AI Enhancer runner             | dedicated action runner |
| new     | `src/local_tools/ai_enhancer_runner.cpp` | created  | implementation for AI Enhancer runner          | ComfyUI enhancer flow   |
| updated | `src/generation_service.h`               | modified | connect new runner to service                  | dependency added        |
| updated | `src/generation_service.cpp`             | modified | `RunSingleImageViaComfy` delegates AI Enhancer | service cleanup         |
| updated | `CMakeLists.txt`                         | modified | add new runner source                          | required for build      |

---

## Flow Comparison

| Flow                   | Before                                                           | After                                                                               | Output                                              |
| ---------------------- | ---------------------------------------------------------------- | ----------------------------------------------------------------------------------- | --------------------------------------------------- |
| AI Enhancer            | `GenerationService → BuildAiEnhancerWorkflow → ComfyUI → output` | `GenerationService → AiEnhancerRunner → BuildAiEnhancerWorkflow → ComfyUI → output` | `/outputs/pixo_ai_enhancer_...png`                  |
| Remove Objects         | already extracted in previous branch                             | remains in `RemoveObjectsRunner`                                                    | `/outputs/final_pixo_remove_objects_...png`         |
| Remove Background      | already extracted                                                | remains in background runner                                                        | `/outputs/pixo_remove_background_...png`            |
| Remove Objects Cleanup | already extracted                                                | remains in cleanup runner                                                           | `/outputs/final_pixo_remove_objects_cleanup_...png` |
| Other tool actions     | still in generation flow                                         | unchanged in this branch                                                            | unchanged                                           |

---

## AI Enhancer Runner Responsibility

| Responsibility               | Description                                          |
| ---------------------------- | ---------------------------------------------------- |
| resolve input image          | uses uploaded source image                           |
| copy/upload image to ComfyUI | prepares file for ComfyUI input                      |
| build enhancer workflow      | uses `BuildAiEnhancerWorkflow()`                     |
| queue ComfyUI prompt         | sends workflow to ComfyUI                            |
| wait for output              | waits for first matching ComfyUI output              |
| save output                  | copies/downloads image into backend `storage/output` |
| return public URL            | returns `/outputs/...` URL for task result           |

---

## Pipeline

| Step | Tool / File                  | Meaning                        |
| ---- | ---------------------------- | ------------------------------ |
| 1    | source image                 | user uploaded image            |
| 2    | `AiEnhancerRunner`           | controls enhancer action       |
| 3    | `BuildAiEnhancerWorkflow()`  | builds ComfyUI workflow        |
| 4    | `workflows/ai_enhancer.json` | AI Enhancer workflow           |
| 5    | ComfyUI                      | runs enhancer/upscale pipeline |
| 6    | `storage/output`             | stores generated result        |
| 7    | API result                   | returns `/outputs/...` URL     |

```text
source image
↓
AiEnhancerRunner
↓
BuildAiEnhancerWorkflow()
↓
workflows/ai_enhancer.json
↓
ComfyUI
↓
storage/output
↓
resultImageUrls
```

---

## Responsibility Split

| Component           | Responsibility                          | Should NOT do                                   |
| ------------------- | --------------------------------------- | ----------------------------------------------- |
| `GenerationService` | route request and update task lifecycle | contain AI Enhancer implementation              |
| `AiEnhancerRunner`  | execute AI Enhancer pipeline            | handle remove objects/background/template logic |
| `WorkflowBuilder`   | build enhancer workflow JSON            | run HTTP requests                               |
| `ComfyClient`       | communicate with ComfyUI                | decide business action routing                  |
| `OutputService`     | save/serve output image                 | know about AI Enhancer prompt logic             |
| `CMakeLists.txt`    | compile new source                      | business logic                                  |

---

## What This Branch Solves

| Problem                                                            | Fix                                       | Result                          |
| ------------------------------------------------------------------ | ----------------------------------------- | ------------------------------- |
| AI Enhancer logic was still inside `GenerationService`             | moved to `AiEnhancerRunner`               | service becomes cleaner         |
| `RunSingleImageViaComfy` was growing with action-specific branches | AI Enhancer branch extracted              | fewer conditions                |
| enhancer workflow was harder to change independently               | runner owns action-specific flow          | safer future tuning             |
| output handling was mixed with generic flow                        | runner controls enhancer output           | easier debugging                |
| future runners needed a pattern                                    | AI Enhancer follows same extraction style | architecture becomes consistent |

---

## Validation

| Check                      | Expected                                   |
| -------------------------- | ------------------------------------------ |
| build                      | `cmake --build .` passes                   |
| `serverAction=ai_enhancer` | routes to `AiEnhancerRunner`               |
| workflow                   | uses `workflows/ai_enhancer.json`          |
| ComfyUI                    | receives valid prompt                      |
| result                     | saved into `storage/output`                |
| API result                 | returns `/outputs/pixo_ai_enhancer_...png` |
| other actions              | unchanged                                  |

---

## Test Request

| Field            | Value                    |
| ---------------- | ------------------------ |
| `serverAction`   | `ai_enhancer`            |
| `toolType`       | `AI_ENHANCER`            |
| `sourceImageUrl` | uploaded image URL       |
| `prompt`         | optional enhancer prompt |
| `outputCount`    | `1` or more              |

```bash
RESPONSE=$(curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "serverAction": "ai_enhancer",
  "toolType": "AI_ENHANCER",
  "sourceImageUrl": "http://192.168.0.177:8080/uploads/YOUR_TEST_IMAGE.jpg",
  "prompt": "HD Enhance",
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
| checkout | `git checkout feature/ai-enhancer-runner`                                                                                       |
| build    | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                           |
| run      | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend` |
| commit   | `git add . && git commit -m "Extract AI enhancer runner"`                                                                       |
| push     | `git push -u origin feature/ai-enhancer-runner`                                                                                 |

---

## Final State

| Item                         | Status                         |
| ---------------------------- | ------------------------------ |
| `AiEnhancerRunner`           | added                          |
| `ai_enhancer` flow           | extracted                      |
| `GenerationService`          | cleaner                        |
| `BuildAiEnhancerWorkflow()`  | preserved                      |
| `workflows/ai_enhancer.json` | preserved                      |
| ComfyUI execution            | preserved                      |
| output URL format            | preserved                      |
| Remove Objects runner        | preserved from previous branch |
| next branch                  | `feature/template-runner`      |

---

## Back

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
