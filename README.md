# 🧰 feature/local-tool-runner

| Branch                      | Parent                           | Goal                                             | Main Result                                                             | Local Tool        | ComfyUI                        | AI Enhancer | Tools SDXL | Templates | Android            | Back                                                                                  |
| --------------------------- | -------------------------------- | ------------------------------------------------ | ----------------------------------------------------------------------- | ----------------- | ------------------------------ | ----------- | ---------- | --------- | ------------------ | ------------------------------------------------------------------------------------- |
| `feature/local-tool-runner` | `feature/templates-sdxl-actions` | вынести локальные инструменты в отдельный runner | `remove_background` больше не живёт внутри большого `GenerationService` | `LocalToolRunner` | не нужен для remove background | unchanged   | unchanged  | unchanged | No Android changes | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## 🌳 Tree

```text
mobile-assets-backend/
├── src/
│   ├── local_tools/
│   │   ├── local_tool_runner.h
│   │   └── local_tool_runner.cpp
│   ├── generation_service.h
│   ├── generation_service.cpp
│   ├── output_service.h
│   ├── output_service.cpp
│   └── main.cpp
├── scripts/
│   └── remove_background.py
├── storage/
│   ├── input/
│   └── output/
├── workflows/
│   ├── ai_enhancer.json
│   ├── tool_img2img.json
│   └── template_img2img.json
├── CMakeLists.txt
└── README.md
```

---

## ✅ What was done

| #  | Area               | Before                                             | After                                   | Result                         |
| -- | ------------------ | -------------------------------------------------- | --------------------------------------- | ------------------------------ |
| 1  | Remove Background  | logic lived inside `GenerationService`             | logic moved to `LocalToolRunner`        | cleaner architecture           |
| 2  | Local tools        | no dedicated local tool layer                      | added `src/local_tools/`                | future local tools have a home |
| 3  | Background removal | direct block inside generation flow                | `RunRemoveBackground()` method          | reusable runner method         |
| 4  | White mode         | handled in mixed generation code                   | handled by local runner + Python script | stable white background        |
| 5  | Transparent mode   | handled in mixed generation code                   | handled by local runner + Python script | stable RGBA alpha PNG          |
| 6  | ComfyUI dependency | remove background could be mixed with SDXL logic   | remove background does not use ComfyUI  | correct tool behavior          |
| 7  | Android options    | needed `backgroundType` / `backgroundMode` support | runner reads both                       | Android-compatible             |
| 8  | Output URL         | generated manually inside generation logic         | generated via `OutputService`           | consistent `/outputs/...`      |
| 9  | Project root       | unsafe path from current dir                       | fixed `root` from `main.cpp`            | stable command path            |
| 10 | Include path       | wrong include from nested folder                   | `#include "../output_service.h"`        | build fixed                    |

---

## 🧱 Main files

| File                                    | Purpose                                                        |
| --------------------------------------- | -------------------------------------------------------------- |
| `src/local_tools/local_tool_runner.h`   | declares `LocalToolRunner`                                     |
| `src/local_tools/local_tool_runner.cpp` | implements local remove background execution                   |
| `src/generation_service.h`              | receives `LocalToolRunner&`                                    |
| `src/generation_service.cpp`            | delegates `remove_background` to local runner                  |
| `src/main.cpp`                          | creates `LocalToolRunner` and passes it to `GenerationService` |
| `CMakeLists.txt`                        | adds `src/local_tools/local_tool_runner.cpp`                   |
| `scripts/remove_background.py`          | actual `rembg + Pillow` implementation                         |

---

## 🔁 Remove Background flow

| Step | Component         | Action                                           | Result                                   |
| ---- | ----------------- | ------------------------------------------------ | ---------------------------------------- |
| 1    | Android           | sends `serverAction=remove_background`           | backend task created                     |
| 2    | GenerationService | extracts uploaded file name                      | input file known                         |
| 3    | GenerationService | calls `local_tool_runner_.RunRemoveBackground()` | local path begins                        |
| 4    | LocalToolRunner   | reads `backgroundType` or `backgroundMode`       | `white` or `transparent`                 |
| 5    | LocalToolRunner   | runs `scripts/remove_background.py`              | PNG output created                       |
| 6    | OutputService     | builds public URL                                | `/outputs/pixo_remove_background_...png` |
| 7    | GenerationService | duplicates output if `outputCount > 1`           | result array filled                      |
| 8    | Android           | receives completed result                        | Result Screen shows image                |

---

## 🧼 Supported modes

| Mode          | Android option                                               | Script mode   | Output                           |
| ------------- | ------------------------------------------------------------ | ------------- | -------------------------------- |
| white         | `backgroundType=white` or `backgroundMode=white`             | `white`       | subject on pure white background |
| transparent   | `backgroundType=transparent` or `backgroundMode=transparent` | `transparent` | RGBA PNG with alpha              |
| empty/unknown | missing or other value                                       | `white`       | safe visible white result        |

---

## 🧩 LocalToolRunner API

| Method                  | Input                                      | Output                     | Purpose                                                  |
| ----------------------- | ------------------------------------------ | -------------------------- | -------------------------------------------------------- |
| `RunRemoveBackground()` | `task_id`, `input_file_name`, request JSON | optional public output URL | execute local remove background                          |
| `ReadOptionString()`    | request JSON + key                         | string                     | read `options.backgroundType` / `options.backgroundMode` |

---

## ⚙️ Integration points

| Place                           | Change                                                                              |
| ------------------------------- | ----------------------------------------------------------------------------------- |
| `generation_service.h`          | include `local_tools/local_tool_runner.h`                                           |
| `GenerationService` constructor | add `local_tools::LocalToolRunner& local_tool_runner`                               |
| `GenerationService` fields      | add `local_tools::LocalToolRunner& local_tool_runner_`                              |
| `generation_service.cpp`        | replace big `remove_background` block with local runner call                        |
| `main.cpp`                      | create `backend_input_dir`, create `LocalToolRunner`, pass into `GenerationService` |
| `CMakeLists.txt`                | add `src/local_tools/local_tool_runner.cpp`                                         |

---

## 🛠 Build fixes

| Problem                       | Symptom                                       | Fix                                                                 |
| ----------------------------- | --------------------------------------------- | ------------------------------------------------------------------- |
| wrong include path            | `output_service.h: No such file or directory` | use `#include "../output_service.h"`                                |
| missing input dir variable    | `backend_input_dir was not declared`          | define `const fs::path backend_input_dir = root / "storage/input";` |
| unsafe project root           | command path could break from different cwd   | pass `root` into `LocalToolRunner`                                  |
| UploadService duplicated path | separate hardcoded path                       | use `backend_input_dir` for upload service too                      |
| GenerationService input path  | repeated `root / "storage/input"`             | pass `backend_input_dir`                                            |

---

## 🧪 Test commands

| Test                     | Expected                                                              |
| ------------------------ | --------------------------------------------------------------------- |
| white background request | `/outputs/pixo_remove_background_...png` with white background        |
| transparent request      | `/outputs/pixo_remove_background_...png` with alpha                   |
| backend build            | `cmake --build .` succeeds                                            |
| logs                     | `[LOCAL_REMOVE_BACKGROUND_START]` then `[LOCAL_REMOVE_BACKGROUND_OK]` |

```bash
cd ~/mobile-assets-backend/build
cmake --build .
```

```bash
PUBLIC_BASE_URL="http://192.168.0.177:8080" \
COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" \
./bin/mobile_assets_backend
```

---

## 🧪 White test

```bash
curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "serverAction":"remove_background",
  "toolType":"REMOVE_BACKGROUND",
  "sourceImageUrl":"http://192.168.0.177:8080/uploads/img_3134512171482_49c5b060e98f6d76.jpg",
  "options":{"backgroundType":"white"},
  "outputCount":1
}' | jq
```

---

## 🧪 Transparent test

```bash
curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "serverAction":"remove_background",
  "toolType":"REMOVE_BACKGROUND",
  "sourceImageUrl":"http://192.168.0.177:8080/uploads/img_3134512171482_49c5b060e98f6d76.jpg",
  "options":{"backgroundType":"transparent"},
  "outputCount":1
}' | jq
```

---

## ⚠️ What is NOT faked

| Tool                | Why not implemented here                                      |
| ------------------- | ------------------------------------------------------------- |
| `remove_objects`    | needs mask/inpainting workflow; without mask it would be fake |
| `smile_edit`        | needs face/expression model; simple prompt is not honest      |
| `hair_studio` local | needs segmentation/face/hair model                            |
| local SDXL tools    | still handled by ComfyUI workflows                            |

---

## 🔜 Next honest stage

| Next feature          | Requirement                                           |
| --------------------- | ----------------------------------------------------- |
| real `remove_objects` | user mask or auto mask + inpainting workflow          |
| real `smile_edit`     | face expression model or specialized ComfyUI workflow |
| better local tools    | separate local runners per tool                       |
| queue worker          | local tools and ComfyUI jobs can share task lifecycle |

---

## 🧾 Git

| Step          | Command                                                  |
| ------------- | -------------------------------------------------------- |
| create branch | `git checkout -b feature/local-tool-runner`              |
| status        | `git status`                                             |
| add           | `git add .`                                              |
| commit        | `git commit -m "Extract local remove background runner"` |
| push          | `git push -u origin feature/local-tool-runner`           |

---

## 🏁 Final result

| Capability                                             | Status |
| ------------------------------------------------------ | ------ |
| `LocalToolRunner` added                                | ✅      |
| `remove_background` extracted from `GenerationService` | ✅      |
| direct `rembg` path preserved                          | ✅      |
| white mode works                                       | ✅      |
| transparent mode works                                 | ✅      |
| Android request format supported                       | ✅      |
| ComfyUI not used for remove background                 | ✅      |
| architecture ready for future local tools              | ✅      |
| fake Remove Objects / Smile not added                  | ✅      |

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
