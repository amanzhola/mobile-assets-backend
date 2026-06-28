# feature/remove-objects-mask-runner

| Branch                               | Previous                            | Next                               | Main Goal                                                      | Problem Before                                                             | Main Change                                                            | Renamed Runner                                       | New/Updated Runner                                         | Updated Core                                                 | Build File       | Result                                        | Back                                                                                  |
| ------------------------------------ | ----------------------------------- | ---------------------------------- | -------------------------------------------------------------- | -------------------------------------------------------------------------- | ---------------------------------------------------------------------- | ---------------------------------------------------- | ---------------------------------------------------------- | ------------------------------------------------------------ | ---------------- | --------------------------------------------- | ------------------------------------------------------------------------------------- |
| `feature/remove-objects-mask-runner` | `feature/upscale-realesrgan-runner` | `feature/action-runners-directory` | отделить mask generation для Remove Objects в отдельный runner | `LocalToolRunner` всё ещё использовался как временный helper для auto-mask | `LocalToolRunner` переименован и превращён в `RemoveObjectsMaskRunner` | `local_tool_runner.* → remove_objects_mask_runner.*` | `RemoveObjectsMaskRunner` + обновлён `RemoveObjectsRunner` | `generation_service.h`, `generation_service.cpp`, `main.cpp` | `CMakeLists.txt` | auto-mask стал отдельным понятным компонентом | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Architecture

| Layer                 | До этой ветки                                  | После этой ветки                     | Что стало лучше                                  | Почему это важно                                  |
| --------------------- | ---------------------------------------------- | ------------------------------------ | ------------------------------------------------ | ------------------------------------------------- |
| `LocalToolRunner`     | временный helper для разных локальных операций | удалён как общее имя                 | исчезло неопределённое назначение                | меньше путаницы                                   |
| Mask generation       | была спрятана в generic local helper           | выделена в `RemoveObjectsMaskRunner` | mask generation стала отдельной ответственностью | легче менять SAM/GroundingDINO                    |
| `RemoveObjectsRunner` | сам зависел от старого helper                  | использует `RemoveObjectsMaskRunner` | auto remove flow стал чище                       | runner отвечает за inpaint, mask runner — за mask |
| `GenerationService`   | хранил старые зависимости                      | обновлён под новый runner            | меньше временной архитектуры                     | подготовка к директории `action_runners`          |
| `main.cpp`            | создавал старый local helper                   | создаёт новый mask runner            | зависимости стали точнее                         | понятнее composition root                         |

---

## Files

| Type    | File                                                                                       | Action   | Purpose                                         | Notes                              |
| ------- | ------------------------------------------------------------------------------------------ | -------- | ----------------------------------------------- | ---------------------------------- |
| renamed | `src/local_tools/local_tool_runner.h` → `src/local_tools/remove_objects_mask_runner.h`     | `git mv` | rename generic helper into specific mask runner | removes vague name                 |
| renamed | `src/local_tools/local_tool_runner.cpp` → `src/local_tools/remove_objects_mask_runner.cpp` | `git mv` | implementation renamed                          | same responsibility clarified      |
| updated | `src/local_tools/remove_objects_mask_runner.h`                                             | modified | declare mask generation runner                  | dedicated mask API                 |
| updated | `src/local_tools/remove_objects_mask_runner.cpp`                                           | modified | implement mask generation logic                 | SAM/GroundingDINO script calls     |
| updated | `src/local_tools/remove_objects_runner.h`                                                  | modified | depend on mask runner                           | cleaner dependency                 |
| updated | `src/local_tools/remove_objects_runner.cpp`                                                | modified | use mask runner for mask creation               | separates mask and inpaint         |
| updated | `src/generation_service.h`                                                                 | modified | update runner dependencies                      | remove old local runner dependency |
| updated | `src/generation_service.cpp`                                                               | modified | route with new dependencies                     | service cleanup                    |
| updated | `src/main.cpp`                                                                             | modified | create/inject `RemoveObjectsMaskRunner`         | composition root updated           |
| updated | `CMakeLists.txt`                                                                           | modified | replace old source with new source              | required for build                 |

---

## Rename

| Old Name                | New Name                         | Why                                 |
| ----------------------- | -------------------------------- | ----------------------------------- |
| `LocalToolRunner`       | `RemoveObjectsMaskRunner`        | старое имя было слишком общее       |
| `local_tool_runner.h`   | `remove_objects_mask_runner.h`   | файл теперь говорит точную роль     |
| `local_tool_runner.cpp` | `remove_objects_mask_runner.cpp` | implementation соответствует runner |
| generic helper          | object mask runner               | меньше архитектурной грязи          |

---

## Flow Comparison

| Flow                 | Before                                                                 | After                                                                              | Output                                              |
| -------------------- | ---------------------------------------------------------------------- | ---------------------------------------------------------------------------------- | --------------------------------------------------- |
| Auto mask generation | `RemoveObjectsRunner → LocalToolRunner → create_object_mask_sam.py`    | `RemoveObjectsRunner → RemoveObjectsMaskRunner → create_object_mask_sam.py`        | mask PNG                                            |
| Auto Remove Objects  | `RemoveObjectsRunner → helper mask → ComfyUI inpaint → post-composite` | `RemoveObjectsRunner → RemoveObjectsMaskRunner → ComfyUI inpaint → post-composite` | `/outputs/final_pixo_remove_objects_...png`         |
| Manual cleanup       | separate cleanup runner                                                | unchanged                                                                          | `/outputs/final_pixo_remove_objects_cleanup_...png` |
| Remove Background    | separate background runner                                             | unchanged                                                                          | `/outputs/pixo_remove_background_...png`            |
| Upscale              | separate upscale runner                                                | unchanged                                                                          | `/outputs/pixo_upscale_image_...png`                |

---

## RemoveObjectsMaskRunner Responsibility

| Responsibility             | Description                                     |
| -------------------------- | ----------------------------------------------- |
| receive source image       | gets backend input image path                   |
| receive prompt/object text | object to remove, for example `зонтик`          |
| run mask script            | calls SAM/GroundingDINO mask generation         |
| validate mask output       | checks mask file exists and is usable           |
| return mask path           | gives mask back to `RemoveObjectsRunner`        |
| stay focused               | does not run ComfyUI, does not save final image |

---

## RemoveObjectsRunner Responsibility After This Branch

| Responsibility                  | Description                           |
| ------------------------------- | ------------------------------------- |
| orchestrate auto remove objects | controls full remove objects pipeline |
| ask mask runner for mask        | delegates mask generation             |
| run ComfyUI inpaint             | uses generated mask                   |
| apply post-composite            | keeps original outside mask           |
| save final result               | outputs final image                   |
| return URL                      | returns `/outputs/...`                |

---

## Pipeline

| Step | Component                     | Meaning                              |
| ---- | ----------------------------- | ------------------------------------ |
| 1    | uploaded source image         | user image                           |
| 2    | `RemoveObjectsRunner`         | auto remove objects orchestration    |
| 3    | `RemoveObjectsMaskRunner`     | creates object mask                  |
| 4    | `create_object_mask_sam.py`   | GroundingDINO/SAM mask script        |
| 5    | `remove_objects_inpaint.json` | ComfyUI inpaint workflow             |
| 6    | `apply_inpaint_mask.py`       | post-composite original outside mask |
| 7    | `storage/output`              | final image                          |
| 8    | API result                    | `/outputs/...` URL                   |

```text
source image
+
object prompt
↓
RemoveObjectsRunner
↓
RemoveObjectsMaskRunner
↓
create_object_mask_sam.py
↓
ComfyUI inpaint
↓
apply_inpaint_mask.py
↓
final output
```

---

## Responsibility Split

| Component                    | Responsibility                          | Should NOT do                        |
| ---------------------------- | --------------------------------------- | ------------------------------------ |
| `GenerationService`          | route request and update task lifecycle | create object masks                  |
| `RemoveObjectsRunner`        | full auto remove objects pipeline       | implement SAM mask creation directly |
| `RemoveObjectsMaskRunner`    | create object mask only                 | run ComfyUI / save final result      |
| `RemoveObjectsCleanupRunner` | manual cleanup second pass              | auto text-guided mask generation     |
| `RemoveBackgroundRunner`     | background removal                      | object removal                       |
| `UpscaleRunner`              | local upscale                           | masks / inpaint                      |
| Python scripts               | image/mask operations                   | backend routing                      |

---

## What This Branch Solves

| Problem                                          | Fix                                  | Result                          |
| ------------------------------------------------ | ------------------------------------ | ------------------------------- |
| `LocalToolRunner` name was unclear               | renamed to `RemoveObjectsMaskRunner` | exact purpose                   |
| mask generation was a hidden helper              | made explicit runner                 | easier debugging                |
| `RemoveObjectsRunner` had mixed responsibilities | mask creation delegated              | cleaner object removal pipeline |
| future action runners needed clearer naming      | removed generic local helper         | better architecture             |
| `main.cpp` dependency graph was vague            | injects explicit mask runner         | cleaner composition root        |

---

## Validation

| Check                         | Expected                               |
| ----------------------------- | -------------------------------------- |
| build                         | `cmake --build .` passes               |
| `serverAction=remove_objects` | still works                            |
| mask generation               | handled by `RemoveObjectsMaskRunner`   |
| ComfyUI inpaint               | still handled by `RemoveObjectsRunner` |
| post-composite                | still preserves original outside mask  |
| manual cleanup                | unchanged                              |
| remove background             | unchanged                              |
| upscale                       | unchanged                              |

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

| Step         | Command                                                                                                                                                                                  |
| ------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| checkout     | `git checkout feature/remove-objects-mask-runner`                                                                                                                                        |
| rename files | `git mv src/local_tools/local_tool_runner.h src/local_tools/remove_objects_mask_runner.h && git mv src/local_tools/local_tool_runner.cpp src/local_tools/remove_objects_mask_runner.cpp` |
| build        | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                                                                                    |
| run          | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend`                                                          |
| commit       | `git add . && git commit -m "Extract remove objects mask runner"`                                                                                                                        |
| push         | `git push -u origin feature/remove-objects-mask-runner`                                                                                                                                  |

---

## Final State

| Item                      | Status                             |
| ------------------------- | ---------------------------------- |
| `LocalToolRunner`         | removed/renamed                    |
| `RemoveObjectsMaskRunner` | added                              |
| auto mask generation      | extracted                          |
| `RemoveObjectsRunner`     | cleaner                            |
| `GenerationService`       | cleaner                            |
| `main.cpp`                | updated dependency injection       |
| CMake                     | updated                            |
| next branch               | `feature/action-runners-directory` |

---

## Back

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
