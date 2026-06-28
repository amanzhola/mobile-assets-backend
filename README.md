# feature/remove-objects-runner

| Branch                          | Previous                     | Next                         | Main Goal                                                    | Problem Before                                                        | Main Change                            | Added Runner          | Updated Core                                     | Build File       | Result                                                 | Back                                                                                  |
| ------------------------------- | ---------------------------- | ---------------------------- | ------------------------------------------------------------ | --------------------------------------------------------------------- | -------------------------------------- | --------------------- | ------------------------------------------------ | ---------------- | ------------------------------------------------------ | ------------------------------------------------------------------------------------- |
| `feature/remove-objects-runner` | `feature/local-tool-runners` | `feature/ai-enhancer-runner` | вынести основной auto Remove Objects flow в отдельный runner | логика `remove_objects` всё ещё оставалась внутри `GenerationService` | создан отдельный `RemoveObjectsRunner` | `RemoveObjectsRunner` | `generation_service.h`, `generation_service.cpp` | `CMakeLists.txt` | auto Remove Objects стал самостоятельным action runner | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Architecture

| Layer               | До этой ветки                             | После этой ветки                                  | Что стало лучше            | Почему это важно                                              |
| ------------------- | ----------------------------------------- | ------------------------------------------------- | -------------------------- | ------------------------------------------------------------- |
| `GenerationService` | сам управлял auto remove objects pipeline | делегирует remove objects в `RemoveObjectsRunner` | меньше tool-specific кода  | сервис становится orchestration layer                         |
| Remove Objects      | был частью общего generation flow         | стал отдельным runner                             | проще тестировать и менять | auto mask / inpaint / post-composite можно развивать отдельно |
| Mask generation     | запускалась из общего сервиса             | запускается внутри runner                         | логика ближе к tool        | меньше связности                                              |
| ComfyUI inpaint     | был связан с общим flow                   | используется внутри runner                        | workflow isolated          | проще менять remove_objects workflow                          |
| CMake               | не знал про runner                        | добавлен new cpp                                  | сборка включает runner     | ветка собирается штатно                                       |

---

## Files

| Type    | File                                        | Action   | Purpose                                       | Notes                           |
| ------- | ------------------------------------------- | -------- | --------------------------------------------- | ------------------------------- |
| new     | `src/local_tools/remove_objects_runner.h`   | created  | declaration for auto Remove Objects runner    | main remove objects flow        |
| new     | `src/local_tools/remove_objects_runner.cpp` | created  | implementation for auto Remove Objects runner | mask + ComfyUI + post-composite |
| updated | `src/generation_service.h`                  | modified | connect new runner to generation service      | dependency added                |
| updated | `src/generation_service.cpp`                | modified | delegate `remove_objects` to runner           | service cleanup                 |
| updated | `CMakeLists.txt`                            | modified | add new runner source                         | required for build              |

---

## Flow Comparison

| Flow                   | Before                                                                                                | After                                                                                                                       | Output                                              |
| ---------------------- | ----------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------- |
| Auto Remove Objects    | `GenerationService → create_object_mask_sam.py → remove_objects_inpaint.json → apply_inpaint_mask.py` | `GenerationService → RemoveObjectsRunner → create_object_mask_sam.py → remove_objects_inpaint.json → apply_inpaint_mask.py` | `/outputs/final_pixo_remove_objects_...png`         |
| Remove Background      | already extracted in previous branch                                                                  | remains in own runner                                                                                                       | `/outputs/pixo_remove_background_...png`            |
| Remove Objects Cleanup | already extracted in previous branch                                                                  | remains in cleanup runner                                                                                                   | `/outputs/final_pixo_remove_objects_cleanup_...png` |
| Other ComfyUI actions  | handled by generation flow                                                                            | unchanged                                                                                                                   | unchanged                                           |

---

## Remove Objects Runner Responsibility

| Responsibility             | Description                                                      |
| -------------------------- | ---------------------------------------------------------------- |
| resolve input image        | takes uploaded image filename / URL resolved by generation layer |
| run object mask generation | calls SAM/GroundingDINO mask script                              |
| prepare inpaint workflow   | uses remove objects workflow                                     |
| queue ComfyUI prompt       | sends workflow to ComfyUI                                        |
| download ComfyUI result    | gets generated inpaint result                                    |
| apply post-composite       | restores original outside mask                                   |
| return output URL          | gives standard `/outputs/...` result back to task flow           |

---

## Pipeline

| Step | Tool / File                   | Meaning                         |
| ---- | ----------------------------- | ------------------------------- |
| 1    | source image                  | user uploaded image             |
| 2    | `create_object_mask_sam.py`   | creates object mask from prompt |
| 3    | `remove_objects_inpaint.json` | ComfyUI inpaint workflow        |
| 4    | ComfyUI                       | reconstructs masked area        |
| 5    | `apply_inpaint_mask.py`       | keeps original outside mask     |
| 6    | `storage/output`              | saves final image               |
| 7    | API result                    | returns `/outputs/...` URL      |

```text
source image
↓
RemoveObjectsRunner
↓
create_object_mask_sam.py
↓
ComfyUI remove_objects_inpaint.json
↓
apply_inpaint_mask.py
↓
final output
```

---

## Responsibility Split

| Component                    | Responsibility                          | Should NOT do                           |
| ---------------------------- | --------------------------------------- | --------------------------------------- |
| `GenerationService`          | route request and update task lifecycle | contain remove objects implementation   |
| `RemoveObjectsRunner`        | execute auto remove objects pipeline    | handle background/template/prompt logic |
| `RemoveBackgroundRunner`     | remove background only                  | handle object inpaint                   |
| `RemoveObjectsCleanupRunner` | manual cleanup only                     | generate first-pass auto object mask    |
| Python scripts               | mask/post-composite image operations    | backend routing                         |
| ComfyUI                      | inpaint masked region                   | decide task routing                     |

---

## What This Branch Solves

| Problem                                                        | Fix                                   | Result                        |
| -------------------------------------------------------------- | ------------------------------------- | ----------------------------- |
| `remove_objects` was still inside `GenerationService`          | moved to `RemoveObjectsRunner`        | service becomes cleaner       |
| mask/inpaint/post-composite were mixed with task orchestration | runner owns full tool pipeline        | easier debugging              |
| future manual/auto differences were harder to manage           | auto flow separated from cleanup flow | clearer two-pass architecture |
| adding object-specific improvements polluted service           | object logic now isolated             | easier SAM/Comfy tuning       |
| CMake did not include new runner                               | source added                          | normal build                  |

---

## Validation

| Check                         | Expected                                            |
| ----------------------------- | --------------------------------------------------- |
| build                         | `cmake --build .` passes                            |
| `serverAction=remove_objects` | routes to `RemoveObjectsRunner`                     |
| mask generation               | produces object mask                                |
| ComfyUI inpaint               | produces inpaint output                             |
| post-composite                | preserves original outside mask                     |
| result URL                    | returns `/outputs/final_pixo_remove_objects_...png` |
| other actions                 | unchanged                                           |

---

## Test Request

| Field            | Value                             |
| ---------------- | --------------------------------- |
| `serverAction`   | `remove_objects`                  |
| `toolType`       | `REMOVE_OBJECTS`                  |
| `prompt`         | object text, for example `зонтик` |
| `sourceImageUrl` | uploaded image URL                |
| `outputCount`    | `1`                               |

```bash
RESPONSE=$(curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "serverAction": "remove_objects",
  "toolType": "REMOVE_OBJECTS",
  "sourceImageUrl": "http://192.168.0.177:8080/uploads/YOUR_TEST_IMAGE.jpg",
  "prompt": "зонтик",
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
| checkout | `git checkout feature/remove-objects-runner`                                                                                    |
| build    | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                           |
| run      | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend` |
| commit   | `git add . && git commit -m "Extract remove objects runner"`                                                                    |
| push     | `git push -u origin feature/remove-objects-runner`                                                                              |

---

## Final State

| Item                          | Status                         |
| ----------------------------- | ------------------------------ |
| `RemoveObjectsRunner`         | added                          |
| `remove_objects` flow         | extracted                      |
| `GenerationService`           | cleaner                        |
| mask generation               | preserved                      |
| ComfyUI inpaint               | preserved                      |
| post-composite                | preserved                      |
| Remove Background runner      | preserved from previous branch |
| Remove Objects Cleanup runner | preserved from previous branch |
| next branch                   | `feature/ai-enhancer-runner`   |

---

## Back

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
