# feature/generation-action-router

| Branch                             | Previous                           | Next                                  | Main Goal                                                                          | Problem Before                                                                                          | Main Change                     | Added Router             | Updated Core                                     | Updated Runners                          | Build File       | Result                                                                           | Back                                                                                  |
| ---------------------------------- | ---------------------------------- | ------------------------------------- | ---------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------- | ------------------------------- | ------------------------ | ------------------------------------------------ | ---------------------------------------- | ---------------- | -------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------- |
| `feature/generation-action-router` | `feature/action-runners-directory` | `feature/background-progress-cleanup` | вынести маршрутизацию generation actions из `GenerationService` в отдельный router | `GenerationService` всё ещё знал слишком много о том, какой runner запускать для каждого `serverAction` | создан `GenerationActionRouter` | `GenerationActionRouter` | `generation_service.h`, `generation_service.cpp` | action runners подключаются через router | `CMakeLists.txt` | `GenerationService` стал ближе к task lifecycle, а routing ушёл в отдельный слой | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Architecture

| Layer               | До этой ветки                                                     | После этой ветки                                     | Что стало лучше                         | Почему это важно                   |
| ------------------- | ----------------------------------------------------------------- | ---------------------------------------------------- | --------------------------------------- | ---------------------------------- |
| `GenerationService` | сам выбирал runner по `serverAction`                              | делегирует выбор в `GenerationActionRouter`          | меньше routing-кода                     | service отвечает за task lifecycle |
| Action runners      | лежали в `src/action_runners/`, но вызывались напрямую из service | вызываются через router                              | единая точка маршрутизации              | проще добавлять новые actions      |
| Router layer        | отсутствовал                                                      | появился `src/generation/generation_action_router.*` | routing стал отдельной ответственностью | меньше условий в service           |
| `main.cpp`          | создавал runners и передавал их в service                         | создаёт runners/router и передаёт router             | composition root стал понятнее          | dependency graph стал чище         |
| CMake               | не знал про router                                                | добавлены router sources                             | сборка включает новый слой              | нормальная build structure         |

---

## Files

| Type    | File                                          | Action   | Purpose                                                 | Notes                        |
| ------- | --------------------------------------------- | -------- | ------------------------------------------------------- | ---------------------------- |
| new     | `src/generation/generation_action_router.h`   | created  | declaration for action router                           | central generation routing   |
| new     | `src/generation/generation_action_router.cpp` | created  | implementation for action routing                       | dispatch by `serverAction`   |
| updated | `src/generation_service.h`                    | modified | depend on router instead of many direct runner branches | service cleanup              |
| updated | `src/generation_service.cpp`                  | modified | call router for action execution                        | routing removed from service |
| updated | `CMakeLists.txt`                              | modified | add router source                                       | required for build           |

---

## Router Responsibility

| Responsibility                      | Description                                                          |
| ----------------------------------- | -------------------------------------------------------------------- |
| read `serverAction`                 | determines requested generation action                               |
| select correct runner               | routes to AI Enhancer, Template, Prompt, Tools, Remove Objects, etc. |
| keep action dispatch centralized    | avoids spreading `if/else` action logic across service               |
| preserve API contract               | does not change request/response shape                               |
| return action result                | gives generated URLs back to `GenerationService`                     |
| isolate routing from task lifecycle | service remains responsible for status/progress/storage              |

---

## Flow Comparison

| Flow              | Before                                                | After                                                 | Result           |
| ----------------- | ----------------------------------------------------- | ----------------------------------------------------- | ---------------- |
| Action routing    | `GenerationService → if serverAction == ... → runner` | `GenerationService → GenerationActionRouter → runner` | cleaner service  |
| AI Enhancer       | service selected `AiEnhancerRunner`                   | router selects `AiEnhancerRunner`                     | unchanged output |
| Template          | service selected `TemplateRunner`                     | router selects `TemplateRunner`                       | unchanged output |
| Prompt            | service selected `PromptRunner`                       | router selects `PromptRunner`                         | unchanged output |
| Tool actions      | service selected `ToolActionRunner`                   | router selects `ToolActionRunner`                     | unchanged output |
| Remove Objects    | service selected object runners                       | router selects object runners                         | unchanged output |
| Remove Background | service selected background runner                    | router selects background runner                      | unchanged output |

---

## Generation Flow After Router

| Step | Component                | Meaning                                   |
| ---- | ------------------------ | ----------------------------------------- |
| 1    | HTTP request             | Android sends `/generations`              |
| 2    | `GenerationService`      | creates task, stores request, sets status |
| 3    | `GenerationActionRouter` | reads action and chooses runner           |
| 4    | selected runner          | executes action-specific pipeline         |
| 5    | `GenerationService`      | stores completed/error status             |
| 6    | API result               | client reads task result                  |

```text
/generations
↓
GenerationService
↓
GenerationActionRouter
↓
ActionRunner
↓
result URLs
↓
GenerationService task update
```

---

## Action Dispatch

| `serverAction`           | Routed To                    | Notes                                 |
| ------------------------ | ---------------------------- | ------------------------------------- |
| `ai_enhancer`            | `AiEnhancerRunner`           | dedicated AI Enhancer flow            |
| `template`               | `TemplateRunner`             | template asset + template workflow    |
| `prompt`                 | `PromptRunner`               | prompt / collage / SDXL prompt flow   |
| `remove_objects`         | `RemoveObjectsRunner`        | auto mask + inpaint + post-composite  |
| `remove_objects_cleanup` | `RemoveObjectsCleanupRunner` | manual mask cleanup                   |
| `remove_background`      | `RemoveBackgroundRunner`     | local rembg flow                      |
| `upscale_image`          | `UpscaleRunner`              | local RealESRGAN flow                 |
| generic SDXL tools       | `ToolActionRunner`           | ghibli, ghostface, change_scene, etc. |

---

## Responsibility Split

| Component                | Responsibility                                       | Should NOT do                      |
| ------------------------ | ---------------------------------------------------- | ---------------------------------- |
| `GenerationService`      | create/update tasks, persist task state, call router | know every action runner detail    |
| `GenerationActionRouter` | map `serverAction` to runner                         | store tasks or write API responses |
| `ActionRunner` classes   | execute specific action pipeline                     | own global task lifecycle          |
| `ApiHandler`             | parse HTTP and return responses                      | know runner internals              |
| `main.cpp`               | construct dependencies                               | run action logic                   |
| `CMakeLists.txt`         | compile router and runners                           | business logic                     |

---

## What This Branch Solves

| Problem                                                | Fix                                                       | Result                  |
| ------------------------------------------------------ | --------------------------------------------------------- | ----------------------- |
| `GenerationService` still had routing responsibility   | added `GenerationActionRouter`                            | service became smaller  |
| action dispatch was growing with every tool            | centralized dispatch                                      | easier to add actions   |
| runners existed but were not behind one router         | router connects them                                      | cleaner architecture    |
| future action additions would touch service repeatedly | new actions can be added to router                        | less service churn      |
| project needed a real generation layer                 | `src/generation/` now contains router + task/json helpers | better module structure |

---

## Validation

| Check                  | Expected                         |
| ---------------------- | -------------------------------- |
| build                  | `cmake --build .` passes         |
| AI Enhancer            | routes and completes             |
| Template               | routes and completes             |
| Prompt                 | routes and completes             |
| Remove Objects         | routes and completes             |
| Remove Objects Cleanup | routes and completes             |
| Remove Background      | routes and completes             |
| Upscale                | routes and completes             |
| Tool actions           | route through `ToolActionRunner` |
| API format             | unchanged                        |
| task storage           | unchanged                        |

---

## Test Matrix

| Action            | Example `serverAction`   | Expected Output                                     |
| ----------------- | ------------------------ | --------------------------------------------------- |
| AI Enhancer       | `ai_enhancer`            | `/outputs/pixo_ai_enhancer_...png`                  |
| Template          | `template`               | `/outputs/pixo_template_...png`                     |
| Prompt            | `prompt`                 | `/outputs/pixo_prompt_...png`                       |
| Remove Objects    | `remove_objects`         | `/outputs/final_pixo_remove_objects_...png`         |
| Manual Cleanup    | `remove_objects_cleanup` | `/outputs/final_pixo_remove_objects_cleanup_...png` |
| Remove Background | `remove_background`      | `/outputs/pixo_remove_background_...png`            |
| Upscale           | `upscale_image`          | `/outputs/pixo_upscale_image_...png`                |
| Ghibli            | `ghibli`                 | `/outputs/pixo_ghibli_...png`                       |

---

## Commands

| Step       | Command                                                                                                                         |
| ---------- | ------------------------------------------------------------------------------------------------------------------------------- |
| checkout   | `git checkout feature/generation-action-router`                                                                                 |
| build      | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                           |
| run        | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend` |
| smoke test | `curl http://localhost:8080/health; echo`                                                                                       |
| commit     | `git add . && git commit -m "Add generation action router"`                                                                     |
| push       | `git push -u origin feature/generation-action-router`                                                                           |

---

## Final State

| Item                     | Status                                |
| ------------------------ | ------------------------------------- |
| `GenerationActionRouter` | added                                 |
| action routing           | extracted                             |
| `GenerationService`      | cleaner                               |
| action runners directory | preserved                             |
| task lifecycle           | preserved in service                  |
| API behavior             | unchanged                             |
| output URL format        | unchanged                             |
| next branch              | `feature/background-progress-cleanup` |

---

## Back

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
