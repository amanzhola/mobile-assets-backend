# feature/template-workflow-cleanup

| Branch                              | Previous                              | Next                          | Main Goal                                                      | Problem Before                                                                                  | Main Change                                                                     | Updated Runner   | Removed Files                         | Added Script              | Updated Build    | Result                                         | Back                                                                                  |
| ----------------------------------- | ------------------------------------- | ----------------------------- | -------------------------------------------------------------- | ----------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------- | ---------------- | ------------------------------------- | ------------------------- | ---------------- | ---------------------------------------------- | ------------------------------------------------------------------------------------- |
| `feature/template-workflow-cleanup` | `feature/background-progress-cleanup` | `feature/skin-improve-runner` | почистить Template workflow и убрать лишний старый helper слой | template workflow logic была частично размазана между runner и `generation_template_workflow.*` | template workflow перенесён в `TemplateRunner`, старый generation helper удалён | `TemplateRunner` | `generation_template_workflow.h/.cpp` | `convert_image_to_png.py` | `CMakeLists.txt` | Template action стал чище и проще сопровождать | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Architecture

| Layer                             | До этой ветки                                                | После этой ветки                               | Что стало лучше                 | Почему это важно                       |
| --------------------------------- | ------------------------------------------------------------ | ---------------------------------------------- | ------------------------------- | -------------------------------------- |
| `TemplateRunner`                  | выполнял template flow, но часть workflow logic была снаружи | стал главным владельцем template workflow flow | меньше лишних hops              | проще отлаживать template generation   |
| `generation_template_workflow.*`  | отдельный helper для template workflow                       | удалён                                         | меньше дублирования             | template logic живёт рядом с runner    |
| `scripts/convert_image_to_png.py` | отсутствовал                                                 | добавлен общий converter                       | проще нормализовать image input | полезно для template/composition flows |
| `GenerationService`               | уже использовал router/runners                               | не должен знать template workflow details      | architecture preserved          | service остаётся task lifecycle layer  |
| `CMakeLists.txt`                  | содержал старые template workflow files                      | обновлён после удаления                        | сборка чище                     | нет dead sources                       |

---

## Files

| Type    | File                                              | Action   | Purpose                                          | Notes                 |
| ------- | ------------------------------------------------- | -------- | ------------------------------------------------ | --------------------- |
| updated | `src/action_runners/template_runner.h`            | modified | keep template workflow API inside runner         | template owner        |
| updated | `src/action_runners/template_runner.cpp`          | modified | move/own workflow preparation logic              | cleaner template flow |
| removed | `src/generation/generation_template_workflow.h`   | deleted  | old template workflow helper removed             | no longer needed      |
| removed | `src/generation/generation_template_workflow.cpp` | deleted  | old template workflow implementation removed     | no longer needed      |
| new     | `scripts/convert_image_to_png.py`                 | created  | convert/normalize image format to PNG            | shared helper         |
| updated | `CMakeLists.txt`                                  | modified | remove old sources / keep current runner sources | required for build    |

---

## Cleanup Summary

| Area                        | Before                                                    | After                             | Result                  |
| --------------------------- | --------------------------------------------------------- | --------------------------------- | ----------------------- |
| Template workflow ownership | split between runner and `generation_template_workflow.*` | owned by `TemplateRunner`         | cleaner module boundary |
| Old helper files            | still present                                             | removed with `git rm`             | less dead code          |
| PNG conversion              | no shared helper                                          | `scripts/convert_image_to_png.py` | reusable utility        |
| Build config                | referenced old files                                      | cleaned                           | no stale CMake entries  |
| Future template metadata    | harder to add                                             | runner-centered                   | easier to extend        |

---

## Commands Used

| Action                     | Command                                                  |
| -------------------------- | -------------------------------------------------------- |
| edit runner header         | `nano src/action_runners/template_runner.h`              |
| edit runner implementation | `nano src/action_runners/template_runner.cpp`            |
| remove old helper cpp      | `git rm src/generation/generation_template_workflow.cpp` |
| remove old helper header   | `git rm src/generation/generation_template_workflow.h`   |
| create converter script    | `nano scripts/convert_image_to_png.py`                   |
| update build               | `nano CMakeLists.txt`                                    |

---

## Template Flow After Cleanup

| Step | Component                | Meaning                                 |
| ---- | ------------------------ | --------------------------------------- |
| 1    | Android request          | sends `serverAction=template`           |
| 2    | `GenerationActionRouter` | routes to `TemplateRunner`              |
| 3    | `TemplateRunner`         | owns template preparation               |
| 4    | template asset cache     | provides selected template image        |
| 5    | optional PNG conversion  | normalizes image input if needed        |
| 6    | workflow builder         | builds `template_img2img.json` request  |
| 7    | ComfyUI                  | generates template result               |
| 8    | output service           | saves result                            |
| 9    | API response             | returns `/outputs/pixo_template_...png` |

```text
template request
↓
GenerationActionRouter
↓
TemplateRunner
↓
template asset + png normalization
↓
template workflow
↓
ComfyUI
↓
storage/output
↓
resultImageUrls
```

---

## Why `generation_template_workflow.*` Was Removed

| Reason                               | Explanation                                            |
| ------------------------------------ | ------------------------------------------------------ |
| duplicated responsibility            | template runner already owns template action           |
| too many layers                      | request → router → runner → helper was unnecessary     |
| harder debugging                     | template workflow errors were split across files       |
| future metadata needs runner context | x/y/size/template-specific settings belong near runner |
| cleaner build                        | fewer source files and fewer stale includes            |

---

## `convert_image_to_png.py`

| Item       | Purpose                                                  |
| ---------- | -------------------------------------------------------- |
| Script     | `scripts/convert_image_to_png.py`                        |
| Role       | normalize arbitrary image input to PNG                   |
| Useful For | template composition, masks, alpha-safe flows            |
| Why Added  | some workflows expect PNG-like stable input              |
| Future Use | prompt composition, template assets, local preprocessing |

---

## Responsibility Split

| Component                 | Responsibility                               | Should NOT do                  |
| ------------------------- | -------------------------------------------- | ------------------------------ |
| `GenerationService`       | task lifecycle and persistence               | template workflow construction |
| `GenerationActionRouter`  | route `template` to `TemplateRunner`         | perform image conversion       |
| `TemplateRunner`          | own template action and workflow preparation | handle unrelated tools         |
| `convert_image_to_png.py` | image format normalization                   | backend routing                |
| `TemplateAssetService`    | template asset retrieval/cache               | generate final image           |
| `WorkflowBuilder`         | create workflow JSON                         | own task state                 |

---

## What This Branch Solves

| Problem                                          | Fix                                      | Result                                     |
| ------------------------------------------------ | ---------------------------------------- | ------------------------------------------ |
| template workflow code was split                 | moved into `TemplateRunner`              | simpler ownership                          |
| stale helper files remained                      | removed `generation_template_workflow.*` | less dead code                             |
| image format normalization was missing           | added `convert_image_to_png.py`          | more reliable template input               |
| CMake referenced old files                       | build cleaned                            | compile consistency                        |
| future template improvements needed cleaner base | runner-centered design                   | ready for template metadata/placement work |

---

## Validation

| Check                   | Expected                                         |
| ----------------------- | ------------------------------------------------ |
| build                   | `cmake --build .` passes                         |
| no stale includes       | no `generation_template_workflow` include errors |
| `serverAction=template` | routes through `TemplateRunner`                  |
| template image          | resolved from cache/service                      |
| workflow                | generated correctly                              |
| result                  | saved to `storage/output`                        |
| API response            | returns `/outputs/pixo_template_...png`          |
| other actions           | unchanged                                        |

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

| Step               | Command                                                                                                                         |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------- |
| checkout           | `git checkout feature/template-workflow-cleanup`                                                                                |
| build              | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                           |
| run                | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend` |
| check removed refs | `grep -R "generation_template_workflow" -n src CMakeLists.txt`                                                                  |
| commit             | `git add . && git commit -m "Clean template workflow ownership"`                                                                |
| push               | `git push -u origin feature/template-workflow-cleanup`                                                                          |

---

## Final State

| Item                               | Status                             |
| ---------------------------------- | ---------------------------------- |
| `TemplateRunner`                   | owns template workflow preparation |
| `generation_template_workflow.h`   | removed                            |
| `generation_template_workflow.cpp` | removed                            |
| `convert_image_to_png.py`          | added                              |
| CMake                              | cleaned                            |
| template action                    | preserved                          |
| API behavior                       | unchanged                          |
| next branch                        | `feature/skin-improve-runner`      |

---

## Back

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
