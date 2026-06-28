# feature/local-tool-runners

| Branch                       | Previous                             | Next                            | Main Goal                                            | Problem Before                                                                                                    | Main Change                                            | Added Runners                                          | Updated Core                                     | Build File       | Result                                     | Back                                                                                  |
| ---------------------------- | ------------------------------------ | ------------------------------- | ---------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------ | ------------------------------------------------------ | ------------------------------------------------ | ---------------- | ------------------------------------------ | ------------------------------------------------------------------------------------- |
| `feature/local-tool-runners` | `feature/remove-objects-manual-mask` | `feature/remove-objects-runner` | вынести локальные инструменты из `GenerationService` | `GenerationService` начал превращаться в свалку локальной логики, Python-вызовов, ComfyUI-вызовов и маршрутизации | созданы отдельные local runners для локальных операций | `RemoveBackgroundRunner`, `RemoveObjectsCleanupRunner` | `generation_service.h`, `generation_service.cpp` | `CMakeLists.txt` | первый шаг к нормальной runner-архитектуре | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Architecture

| Layer                  | До этой ветки                            | После этой ветки                       | Что стало лучше                  | Почему это важно                          |
| ---------------------- | ---------------------------------------- | -------------------------------------- | -------------------------------- | ----------------------------------------- |
| `GenerationService`    | сам выполнял remove background и cleanup | только вызывает отдельные runners      | меньше кода внутри сервиса       | проще добавлять новые tools               |
| `Local tools`          | не были выделены как слой                | появился `src/local_tools/`            | локальная обработка отделена     | Python/CPU tools не смешиваются с ComfyUI |
| Remove Background      | логика была внутри generation flow       | вынесена в `RemoveBackgroundRunner`    | один класс отвечает за один tool | проще менять `rembg`                      |
| Remove Objects Cleanup | cleanup был частью общего flow           | вынесен в `RemoveObjectsCleanupRunner` | manual cleanup отделён           | проще развивать mask cleanup              |
| Build system           | не знал про новые cpp                    | добавлены новые cpp в CMake            | сборка включает runners          | ветка собирается штатно                   |

---

## Files

| Type    | File                                                | Action   | Purpose                                       | Notes                     |
| ------- | --------------------------------------------------- | -------- | --------------------------------------------- | ------------------------- |
| new     | `src/local_tools/remove_background_runner.h`        | created  | declaration for background removal runner     | local tool                |
| new     | `src/local_tools/remove_background_runner.cpp`      | created  | implementation for background removal runner  | calls local script        |
| new     | `src/local_tools/remove_objects_cleanup_runner.h`   | created  | declaration for manual cleanup runner         | cleanup tool              |
| new     | `src/local_tools/remove_objects_cleanup_runner.cpp` | created  | implementation for manual cleanup runner      | cleanup mask flow         |
| updated | `src/generation_service.h`                          | modified | inject/use local runners                      | service API changed       |
| updated | `src/generation_service.cpp`                        | modified | `RunGenerationViaComfy` delegates local flows | removes direct local code |
| updated | `CMakeLists.txt`                                    | modified | add new runner sources                        | required for build        |

---

## Flow Comparison

| Flow                   | Before                                                                 | After                                                                                               | Output                                              |
| ---------------------- | ---------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------- | --------------------------------------------------- |
| Remove Background      | `GenerationService → remove_background.py → output`                    | `GenerationService → RemoveBackgroundRunner → remove_background.py → output`                        | `/outputs/pixo_remove_background_...png`            |
| Remove Objects Cleanup | `GenerationService → prepare mask → cleanup workflow → post-composite` | `GenerationService → RemoveObjectsCleanupRunner → prepare mask → cleanup workflow → post-composite` | `/outputs/final_pixo_remove_objects_cleanup_...png` |
| ComfyUI actions        | mixed with local actions                                               | still handled by generation flow                                                                    | unchanged                                           |
| Local CPU tools        | embedded in service                                                    | start moving into runners                                                                           | cleaner                                             |

---

## Responsibility Split

| Component                    | Responsibility                                                    | Should NOT do                          |
| ---------------------------- | ----------------------------------------------------------------- | -------------------------------------- |
| `GenerationService`          | route generation request, update task status, call correct runner | contain every tool implementation      |
| `RemoveBackgroundRunner`     | handle local background removal                                   | know about template/prompt/AI enhancer |
| `RemoveObjectsCleanupRunner` | handle second-pass manual cleanup                                 | handle normal remove background        |
| Python scripts               | image processing                                                  | routing HTTP/backend requests          |
| `CMakeLists.txt`             | compile new C++ sources                                           | business logic                         |

---

## What This Branch Solves

| Problem                                          | Fix                       | Result                            |
| ------------------------------------------------ | ------------------------- | --------------------------------- |
| `GenerationService` was growing too much         | local runners introduced  | service becomes smaller           |
| local tools mixed with ComfyUI tools             | local tools get own layer | clearer architecture              |
| remove background was hard to evolve             | separate runner           | easier to modify mode/path/script |
| manual cleanup was buried inside generation flow | separate runner           | easier to debug cleanup           |
| future tools would make service worse            | runner pattern started    | ready for next branches           |

---

## Validation

| Check                         | Expected                        |
| ----------------------------- | ------------------------------- |
| build                         | `cmake --build .` passes        |
| remove background white       | returns white background output |
| remove background transparent | returns alpha PNG output        |
| remove objects cleanup        | returns final cleanup output    |
| ComfyUI generation            | existing actions still work     |
| API response format           | unchanged                       |
| Android flow                  | unchanged                       |

---

## Commands

| Step     | Command                                                                                                                         |
| -------- | ------------------------------------------------------------------------------------------------------------------------------- |
| checkout | `git checkout feature/local-tool-runners`                                                                                       |
| build    | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                           |
| run      | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend` |
| commit   | `git add . && git commit -m "Extract local tool runners"`                                                                       |
| push     | `git push -u origin feature/local-tool-runners`                                                                                 |

---

## Final State

| Item                         | Status                          |
| ---------------------------- | ------------------------------- |
| `RemoveBackgroundRunner`     | added                           |
| `RemoveObjectsCleanupRunner` | added                           |
| `GenerationService`          | partially cleaned               |
| `CMakeLists.txt`             | updated                         |
| API behavior                 | preserved                       |
| Android behavior             | preserved                       |
| next branch                  | `feature/remove-objects-runner` |

---

## Back

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
