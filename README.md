# feature/action-runners-directory

| Branch                             | Previous                             | Next                               | Main Goal                                          | Problem Before                                                                | Main Change                                     | Directory Created     | Moved From         | Moved To              | Updated Core                       | Build File       | Result                                            | Back                                                                                  |
| ---------------------------------- | ------------------------------------ | ---------------------------------- | -------------------------------------------------- | ----------------------------------------------------------------------------- | ----------------------------------------------- | --------------------- | ------------------ | --------------------- | ---------------------------------- | ---------------- | ------------------------------------------------- | ------------------------------------------------------------------------------------- |
| `feature/action-runners-directory` | `feature/remove-objects-mask-runner` | `feature/generation-action-router` | собрать все action runners в одну правильную папку | runners лежали в `src/local_tools/`, хотя часть из них уже была не local-only | создана отдельная директория для action runners | `src/action_runners/` | `src/local_tools/` | `src/action_runners/` | `generation_service.h`, `main.cpp` | `CMakeLists.txt` | архитектура получила понятный слой action runners | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Architecture

| Layer                 | До этой ветки                                    | После этой ветки                      | Что стало лучше                | Почему это важно              |
| --------------------- | ------------------------------------------------ | ------------------------------------- | ------------------------------ | ----------------------------- |
| `src/local_tools/`    | содержал и local runners, и Comfy/action runners | перестал быть местом для всех runners | название больше не врёт        | меньше путаницы               |
| `src/action_runners/` | отсутствовал                                     | создан как основной слой actions      | runners лежат в одном месте    | проще искать и расширять      |
| `GenerationService`   | подключал runners из `local_tools`               | подключает из `action_runners`        | зависимости стали точнее       | подготовка к router           |
| `main.cpp`            | создавал runners из старой папки                 | создаёт runners из новой структуры    | composition root стал чище     | видно все action dependencies |
| `CMakeLists.txt`      | указывал старые пути                             | обновлён на новые пути                | сборка соответствует структуре | нет legacy путей              |

---

## Directory Move

| Old Directory               | New Directory              | Why                                                       |
| --------------------------- | -------------------------- | --------------------------------------------------------- |
| `src/local_tools/`          | `src/action_runners/`      | не все runners являются local tools                       |
| local helper naming         | action-level naming        | runners управляют actions, а не только локальными scripts |
| mixed responsibilities      | explicit action layer      | архитектура стала понятнее                                |
| temporary extraction folder | stable architecture folder | подготовка к дальнейшему refactor                         |

---

## Files Moved

| Runner                 | Old Path                                          | New Path                                             | Purpose                  |
| ---------------------- | ------------------------------------------------- | ---------------------------------------------------- | ------------------------ |
| AI Enhancer            | `src/local_tools/ai_enhancer_runner.*`            | `src/action_runners/ai_enhancer_runner.*`            | AI Enhancer action       |
| Remove Background      | `src/local_tools/remove_background_runner.*`      | `src/action_runners/remove_background_runner.*`      | local background removal |
| Remove Objects         | `src/local_tools/remove_objects_runner.*`         | `src/action_runners/remove_objects_runner.*`         | auto remove objects      |
| Remove Objects Cleanup | `src/local_tools/remove_objects_cleanup_runner.*` | `src/action_runners/remove_objects_cleanup_runner.*` | manual cleanup           |
| Remove Objects Mask    | `src/local_tools/remove_objects_mask_runner.*`    | `src/action_runners/remove_objects_mask_runner.*`    | mask generation          |
| Upscale                | `src/local_tools/upscale_runner.*`                | `src/action_runners/upscale_runner.*`                | local RealESRGAN upscale |
| Template               | `src/local_tools/template_runner.*`               | `src/action_runners/template_runner.*`               | template generation      |
| Prompt                 | `src/local_tools/prompt_runner.*`                 | `src/action_runners/prompt_runner.*`                 | prompt generation        |
| Tool Action            | `src/local_tools/tool_action_runner.*`            | `src/action_runners/tool_action_runner.*`            | shared SDXL tools        |

---

## Git Move Commands

| Step                   | Command                                                                                                                                                                                                                      |
| ---------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| create directory       | `mkdir -p src/action_runners`                                                                                                                                                                                                |
| move AI Enhancer       | `git mv src/local_tools/ai_enhancer_runner.h src/action_runners/ai_enhancer_runner.h && git mv src/local_tools/ai_enhancer_runner.cpp src/action_runners/ai_enhancer_runner.cpp`                                             |
| move Remove Background | `git mv src/local_tools/remove_background_runner.h src/action_runners/remove_background_runner.h && git mv src/local_tools/remove_background_runner.cpp src/action_runners/remove_background_runner.cpp`                     |
| move Remove Objects    | `git mv src/local_tools/remove_objects_runner.h src/action_runners/remove_objects_runner.h && git mv src/local_tools/remove_objects_runner.cpp src/action_runners/remove_objects_runner.cpp`                                 |
| move Cleanup           | `git mv src/local_tools/remove_objects_cleanup_runner.h src/action_runners/remove_objects_cleanup_runner.h && git mv src/local_tools/remove_objects_cleanup_runner.cpp src/action_runners/remove_objects_cleanup_runner.cpp` |
| move Mask Runner       | `git mv src/local_tools/remove_objects_mask_runner.h src/action_runners/remove_objects_mask_runner.h && git mv src/local_tools/remove_objects_mask_runner.cpp src/action_runners/remove_objects_mask_runner.cpp`             |
| move Upscale           | `git mv src/local_tools/upscale_runner.h src/action_runners/upscale_runner.h && git mv src/local_tools/upscale_runner.cpp src/action_runners/upscale_runner.cpp`                                                             |
| move Template          | `git mv src/local_tools/template_runner.h src/action_runners/template_runner.h && git mv src/local_tools/template_runner.cpp src/action_runners/template_runner.cpp`                                                         |
| move Prompt            | `git mv src/local_tools/prompt_runner.h src/action_runners/prompt_runner.h && git mv src/local_tools/prompt_runner.cpp src/action_runners/prompt_runner.cpp`                                                                 |
| move Tool Action       | `git mv src/local_tools/tool_action_runner.h src/action_runners/tool_action_runner.h && git mv src/local_tools/tool_action_runner.cpp src/action_runners/tool_action_runner.cpp`                                             |

---

## Project Structure

| Area             | Structure                                                                                                                                                                                                                                                                                                                                                                                                         |
| ---------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Action runners   | `src/action_runners/ai_enhancer_runner.*`, `src/action_runners/remove_background_runner.*`, `src/action_runners/remove_objects_runner.*`, `src/action_runners/remove_objects_cleanup_runner.*`, `src/action_runners/remove_objects_mask_runner.*`, `src/action_runners/upscale_runner.*`, `src/action_runners/template_runner.*`, `src/action_runners/prompt_runner.*`, `src/action_runners/tool_action_runner.*` |
| Core service     | `src/generation_service.h`, `src/generation_service.cpp`                                                                                                                                                                                                                                                                                                                                                          |
| Composition root | `src/main.cpp`                                                                                                                                                                                                                                                                                                                                                                                                    |
| Build            | `CMakeLists.txt`                                                                                                                                                                                                                                                                                                                                                                                                  |

---

## Flow After Move

| Request Type             | Runner Path                                          | Result                                              |
| ------------------------ | ---------------------------------------------------- | --------------------------------------------------- |
| `ai_enhancer`            | `src/action_runners/ai_enhancer_runner.*`            | `/outputs/pixo_ai_enhancer_...png`                  |
| `remove_background`      | `src/action_runners/remove_background_runner.*`      | `/outputs/pixo_remove_background_...png`            |
| `remove_objects`         | `src/action_runners/remove_objects_runner.*`         | `/outputs/final_pixo_remove_objects_...png`         |
| `remove_objects_cleanup` | `src/action_runners/remove_objects_cleanup_runner.*` | `/outputs/final_pixo_remove_objects_cleanup_...png` |
| `upscale_image`          | `src/action_runners/upscale_runner.*`                | `/outputs/pixo_upscale_image_...png`                |
| `template`               | `src/action_runners/template_runner.*`               | `/outputs/pixo_template_...png`                     |
| `prompt`                 | `src/action_runners/prompt_runner.*`                 | `/outputs/pixo_prompt_...png`                       |
| generic tools            | `src/action_runners/tool_action_runner.*`            | `/outputs/pixo_<action>_...png`                     |

---

## Responsibility Split

| Component             | Responsibility                            | Should NOT do                     |
| --------------------- | ----------------------------------------- | --------------------------------- |
| `src/action_runners/` | contain all action execution classes      | store unrelated scripts/models    |
| `GenerationService`   | task lifecycle and orchestration          | own tool implementation details   |
| `main.cpp`            | construct dependencies and inject runners | contain runner logic              |
| `CMakeLists.txt`      | compile sources from new paths            | keep old `local_tools` references |
| `scripts/`            | Python helpers                            | C++ action orchestration          |

---

## What This Branch Solves

| Problem                                                    | Fix                           | Result                               |
| ---------------------------------------------------------- | ----------------------------- | ------------------------------------ |
| runners were under misleading `local_tools` folder         | moved to `action_runners`     | correct naming                       |
| Comfy-based runners looked like local tools                | directory now matches reality | less confusion                       |
| many future runners would make `local_tools` worse         | created stable layer          | scalable structure                   |
| include paths and CMake paths needed cleanup               | updated core/build files      | consistent project structure         |
| next router refactor needed a clean runner directory first | action runners grouped        | ready for `generation_action_router` |

---

## Validation

| Check                | Expected                                        |
| -------------------- | ----------------------------------------------- |
| build                | `cmake --build .` passes                        |
| CMake paths          | no old `src/local_tools/*runner.cpp` references |
| includes             | updated to `action_runners/...`                 |
| `main.cpp`           | constructs runners from new headers             |
| `GenerationService`  | uses new runner types                           |
| all existing actions | behavior unchanged                              |
| output URLs          | unchanged                                       |

---

## Commands

| Step            | Command                                                                                                                         |
| --------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| checkout        | `git checkout feature/action-runners-directory`                                                                                 |
| build           | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                           |
| run             | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend` |
| check old paths | `grep -R "local_tools" -n src CMakeLists.txt`                                                                                   |
| commit          | `git add . && git commit -m "Move action runners into dedicated directory"`                                                     |
| push            | `git push -u origin feature/action-runners-directory`                                                                           |

---

## Final State

| Item                             | Status                             |
| -------------------------------- | ---------------------------------- |
| `src/action_runners/`            | added                              |
| all runners                      | moved                              |
| `src/local_tools/` runner role   | removed                            |
| `GenerationService` includes     | updated                            |
| `main.cpp` includes/dependencies | updated                            |
| `CMakeLists.txt`                 | updated                            |
| behavior                         | unchanged                          |
| next branch                      | `feature/generation-action-router` |

---

## Back

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
