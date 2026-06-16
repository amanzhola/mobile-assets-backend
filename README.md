# 🧠 feature/comfyui-worker

| Branch                   | Parent                  | Назначение                   | Главная идея                                                         | Что подключено                                                                                                           | Pipeline                                                                  | API                                               | Назад                                                                                         |
| ------------------------ | ----------------------- | ---------------------------- | -------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------- | ------------------------------------------------- | --------------------------------------------------------------------------------------------- |
| `feature/comfyui-worker` | `feature/serve-uploads` | Подключить backend к ComfyUI | Заменить mock-only generation на реальный ComfyUI execution pipeline | `ComfyClient`, `WorkflowBuilder`, `workflows/*.json`, `/comfy/health`, `/comfy/test-prompt`, `/comfy/test-prompt-result` | `Backend → ComfyUI /prompt → history → output → storage/output → Android` | `/generations`, `/outputs/{fileName}`, `/comfy/*` | [Back to main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## ✅ Что сделано

| Категория       | Файл / команда                                                                | Было                          | Стало                                              | Зачем                                              | Статус |
| --------------- | ----------------------------------------------------------------------------- | ----------------------------- | -------------------------------------------------- | -------------------------------------------------- | ------ |
| Stable commit   | `git commit -m "Refactor generation data layer and stabilize backend errors"` | рабочее состояние без ComfyUI | зафиксирована стабильная база                      | безопасно начать большую интеграцию                | ✅      |
| Branch          | `feature/comfyui-worker`                                                      | `feature/serve-uploads`       | новая ветка                                        | отдельная интеграция ComfyUI                       | ✅      |
| ComfyUI install | `~/ComfyUI`                                                                   | не было                       | ComfyUI установлен рядом с backend                 | локальный AI worker                                | ✅      |
| ComfyUI API     | `http://127.0.0.1:8188`                                                       | нет worker-а                  | `/system_stats`, `/prompt`, `/history/{prompt_id}` | backend может общаться с ComfyUI                   | ✅      |
| Workflow        | `workflows/ai_enhancer.json`                                                  | неправильный fake JSON        | настоящий ComfyUI API workflow                     | `LoadImage → SaveImage`                            | ✅      |
| Builder         | `src/comfy/workflow_builder.*`                                                | не было                       | placeholder replacement                            | подстановка `{{input_image}}`, `{{output_prefix}}` | ✅      |
| Client          | `src/comfy/comfy_client.*`                                                    | не было                       | queue prompt + history polling                     | отправка workflow в ComfyUI                        | ✅      |
| Backend API     | `/comfy/health`                                                               | не было                       | проверка ComfyUI из backend                        | диагностика                                        | ✅      |
| Backend API     | `/comfy/test-prompt`                                                          | не было                       | тест queue prompt                                  | доказать `/prompt`                                 | ✅      |
| Backend API     | `/comfy/test-prompt-result`                                                   | не было                       | тест output result                                 | доказать output file                               | ✅      |
| Generation      | `RunGenerationViaComfy()`                                                     | mock URLs                     | ComfyUI copy pipeline                              | все actions идут через workflow                    | ✅      |
| Output          | `storage/output/`                                                             | не было real result           | result image копируется из ComfyUI                 | Android показывает локальную картинку              | ✅      |
| HTTP cleanup    | `http_server.cpp`, `api_handler.cpp`                                          | шум `Broken pipe`, debug logs | тише и стабильнее                                  | mobile-friendly HTTP                               | ✅      |
| HEAD support    | `/uploads`, `/outputs`                                                        | только GET                    | GET + HEAD                                         | Android/Coil diagnostics                           | ✅      |

---

## 🧱 Структура ветки

| Root                      | ComfyUI     | Backend source                                                                                                          | Workflows                                                                                                         | Storage input    | Storage output    | API                                                                                                   |
| ------------------------- | ----------- | ----------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------- | ---------------- | ----------------- | ----------------------------------------------------------------------------------------------------- |
| `~/mobile-assets-backend` | `~/ComfyUI` | `src/comfy/comfy_client.*`, `src/comfy/workflow_builder.*`, `generation_service.*`, `api_handler.*`, `output_service.*` | `workflows/ai_enhancer.json`, `workflows/ghibli.json`, `workflows/template.json`, `workflows/prompt.json`, others | `storage/input/` | `storage/output/` | `/comfy/health`, `/comfy/test-prompt`, `/comfy/test-prompt-result`, `/generations`, `/outputs/{file}` |

---

## 🧭 Архитектура ComfyUI pipeline

| Step | Component         | Input                                     | Action                                               | Output                |
| ---- | ----------------- | ----------------------------------------- | ---------------------------------------------------- | --------------------- |
| 1    | Android           | image                                     | `POST /images/upload`                                | `/uploads/file`       |
| 2    | Backend           | generation JSON                           | `POST /generations`                                  | task created          |
| 3    | GenerationService | uploaded file                             | copy to `~/ComfyUI/input`                            | ComfyUI input file    |
| 4    | WorkflowBuilder   | `serverAction`, input file, output prefix | load `workflows/{action}.json`, replace placeholders | ComfyUI workflow JSON |
| 5    | ComfyClient       | workflow                                  | `POST /prompt`                                       | `prompt_id`           |
| 6    | ComfyClient       | `prompt_id`                               | poll `/history/{prompt_id}`                          | output file name      |
| 7    | OutputService     | `~/ComfyUI/output/file.png`               | copy to `storage/output`                             | backend output file   |
| 8    | Backend           | output file                               | public URL                                           | `/outputs/file.png`   |
| 9    | Android           | result URL                                | show image                                           | Result screen visible |

---

## 🔥 Endpoints после ветки

| Method | Endpoint                       | Input           | Output                    | Purpose                  | Status |
| ------ | ------------------------------ | --------------- | ------------------------- | ------------------------ | ------ |
| `GET`  | `/health`                      | none            | backend health            | backend check            | ✅      |
| `GET`  | `/comfy/health`                | none            | ComfyUI status            | worker check             | ✅      |
| `POST` | `/comfy/test-prompt`           | none            | `promptId`                | queue test workflow      | ✅      |
| `POST` | `/comfy/test-prompt-result`    | none            | `outputUrl`               | full ComfyUI output test | ✅      |
| `POST` | `/images/upload`               | raw image       | `imageUrl`                | upload input image       | ✅      |
| `GET`  | `/uploads/{fileName}`          | file name       | uploaded image            | serve input image        | ✅      |
| `GET`  | `/outputs/{fileName}`          | file name       | generated image           | serve result image       | ✅      |
| `POST` | `/generations`                 | generation JSON | task with real result URL | real generation pipeline | ✅      |
| `GET`  | `/generations/{taskId}`        | task id         | task status               | task check               | ✅      |
| `GET`  | `/generations/{taskId}/result` | task id         | result URLs               | Android Result screen    | ✅      |

---

## 🖥 Terminal 1 — ComfyUI

| Шаг | Команда                                                   | Ожидание                |
| --- | --------------------------------------------------------- | ----------------------- |
| 1   | `cd ~`                                                    | home                    |
| 2   | `git clone https://github.com/comfyanonymous/ComfyUI.git` | ComfyUI скачан          |
| 3   | `cd ComfyUI`                                              | папка ComfyUI           |
| 4   | `python3 -m venv venv`                                    | venv создан             |
| 5   | `source venv/bin/activate`                                | venv активирован        |
| 6   | `pip install -r requirements.txt`                         | зависимости установлены |
| 7   | `python main.py --listen 0.0.0.0 --port 8188 --cpu`       | ComfyUI запущен         |
| 8   | `curl http://127.0.0.1:8188/system_stats`                 | ComfyUI отвечает        |

```bash
cd ~

git clone https://github.com/comfyanonymous/ComfyUI.git

cd ComfyUI

python3 -m venv venv

source venv/bin/activate

pip install -r requirements.txt

python main.py --listen 0.0.0.0 --port 8188 --cpu
```

Проверка из другого окна:

```bash
curl http://127.0.0.1:8188/system_stats
```

---

## 🖥 Terminal 2 — Backend

| Шаг | Команда                                                                               | Ожидание               |
| --- | ------------------------------------------------------------------------------------- | ---------------------- |
| 1   | `cd ~/mobile-assets-backend`                                                          | backend repo           |
| 2   | `git status`                                                                          | проверить стабильность |
| 3   | `git add .`                                                                           | staged                 |
| 4   | `git commit -m "Refactor generation data layer and stabilize backend errors"`         | stable commit          |
| 5   | `git push`                                                                            | push текущей ветки     |
| 6   | `git checkout -b feature/comfyui-worker`                                              | новая ветка            |
| 7   | создать `src/comfy/*`, `workflows/*`, `storage/output`                                | ComfyUI слой           |
| 8   | `rm -rf build && mkdir build && cd build`                                             | clean build            |
| 9   | `conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11` | deps                   |
| 10  | `cmake .. -DCMAKE_BUILD_TYPE=Debug`                                                   | configure              |
| 11  | `cmake --build .`                                                                     | build                  |
| 12  | `export PUBLIC_BASE_URL="http://192.168.0.177:8080"`                                  | public URLs            |
| 13  | `./bin/mobile_assets_backend`                                                         | backend started        |

```bash
cd ~/mobile-assets-backend

git status
git add .
git commit -m "Refactor generation data layer and stabilize backend errors"
git push

git checkout -b feature/comfyui-worker

rm -rf build
mkdir build
cd build

conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11

cmake .. -DCMAKE_BUILD_TYPE=Debug

cmake --build .

export PUBLIC_BASE_URL="http://192.168.0.177:8080"

./bin/mobile_assets_backend
```

---

## 🖥 Terminal 3 — Tests

| Проверка                 | Команда                                                       | Ожидание    |
| ------------------------ | ------------------------------------------------------------- | ----------- |
| Backend health           | `curl http://localhost:8080/health`                           | backend ok  |
| Comfy health via backend | `curl http://localhost:8080/comfy/health`                     | comfy ok    |
| Queue prompt             | `curl -X POST http://localhost:8080/comfy/test-prompt`        | `promptId`  |
| Full prompt result       | `curl -X POST http://localhost:8080/comfy/test-prompt-result` | `outputUrl` |
| Check output file        | `ls -lh storage/output`                                       | png exists  |
| HEAD output              | `curl -I http://localhost:8080/outputs/file.png`              | 200         |
| Download output          | `curl -o /tmp/out.png http://localhost:8080/outputs/file.png` | file saved  |

```bash
curl http://localhost:8080/health; echo

curl http://localhost:8080/comfy/health; echo

curl -X POST http://localhost:8080/comfy/test-prompt; echo

curl -X POST http://localhost:8080/comfy/test-prompt-result; echo

ls -lh ~/mobile-assets-backend/storage/output | tail
```

После получения `outputUrl`:

```bash
curl -I http://localhost:8080/outputs/ИМЯ_ФАЙЛА.png

curl -o /tmp/out.png http://localhost:8080/outputs/ИМЯ_ФАЙЛА.png

ls -lh /tmp/out.png
```

---

## 📄 Workflows

| Workflow file            | Current pipeline        | Real AI status | Purpose                                 |
| ------------------------ | ----------------------- | -------------- | --------------------------------------- |
| `ai_enhancer.json`       | `LoadImage → SaveImage` | copy workflow  | first real ComfyUI execution            |
| `ghibli.json`            | `LoadImage → SaveImage` | copy workflow  | route ghibli through ComfyUI            |
| `ghostface.json`         | `LoadImage → SaveImage` | copy workflow  | route ghostface through ComfyUI         |
| `glam_makeup.json`       | `LoadImage → SaveImage` | copy workflow  | route glam makeup through ComfyUI       |
| `remove_objects.json`    | `LoadImage → SaveImage` | copy workflow  | route remove objects through ComfyUI    |
| `remove_background.json` | `LoadImage → SaveImage` | copy workflow  | route remove background through ComfyUI |
| `skin_improve.json`      | `LoadImage → SaveImage` | copy workflow  | route skin improve through ComfyUI      |
| `upscale_image.json`     | `LoadImage → SaveImage` | copy workflow  | route upscale through ComfyUI           |
| `change_scene.json`      | `LoadImage → SaveImage` | copy workflow  | route change scene through ComfyUI      |
| `hair_studio.json`       | `LoadImage → SaveImage` | copy workflow  | route hair studio through ComfyUI       |
| `smile_edit.json`        | `LoadImage → SaveImage` | copy workflow  | route smile edit through ComfyUI        |
| `template.json`          | `LoadImage → SaveImage` | copy workflow  | route templates through ComfyUI         |
| `prompt.json`            | `LoadImage → SaveImage` | copy workflow  | route prompt through ComfyUI            |

---

## 🧩 workflows/ai_enhancer.json

```json
{
  "1": {
    "class_type": "LoadImage",
    "inputs": {
      "image": "{{input_image}}"
    },
    "_meta": {
      "title": "Load Image"
    }
  },
  "2": {
    "class_type": "SaveImage",
    "inputs": {
      "images": [
        "1",
        0
      ],
      "filename_prefix": "{{output_prefix}}"
    },
    "_meta": {
      "title": "Save Image"
    }
  }
}
```

---

## 📄 Create workflows for all actions

| Action        | Command                                             |
| ------------- | --------------------------------------------------- |
| copy workflow | `cp workflows/ai_enhancer.json workflows/${f}.json` |
| verify        | `ls workflows`                                      |

```bash
cd ~/mobile-assets-backend

for f in \
  glam_makeup \
  remove_objects \
  remove_background \
  skin_improve \
  upscale_image \
  change_scene \
  hair_studio \
  smile_edit \
  ghostface \
  ghibli \
  template \
  prompt
do
  cp workflows/ai_enhancer.json workflows/${f}.json
done

ls workflows
```

---

## 🧩 WorkflowBuilder

| Method                                                     | Input                  | Output               | Purpose                          |
| ---------------------------------------------------------- | ---------------------- | -------------------- | -------------------------------- |
| `LoadWorkflowTemplate(file_name)`                          | workflow file          | JSON object          | load workflow                    |
| `ReplacePlaceholders(value, input_image, output_prefix)`   | JSON value             | modified JSON        | replace placeholders recursively |
| `BuildWorkflow(server_action, input_image, output_prefix)` | action + file + prefix | ComfyUI prompt JSON  | generic workflow builder         |
| `BuildAiEnhancerWorkflow(...)`                             | input + prefix         | ai_enhancer workflow | wrapper                          |

---

## 🧩 ComfyClient

| Method                              | Endpoint                      | Purpose                    | Output           |
| ----------------------------------- | ----------------------------- | -------------------------- | ---------------- |
| `CheckHealth()`                     | `/system_stats`               | проверить ComfyUI          | bool             |
| `QueuePrompt(workflow)`             | `/prompt`                     | поставить prompt в очередь | `prompt_id`      |
| `WaitForFirstOutputFile(prompt_id)` | `/history/{prompt_id}`        | ждать output               | filename         |
| HTTP POST                           | `127.0.0.1:8188/prompt`       | отправить workflow         | ComfyUI response |
| HTTP GET                            | `127.0.0.1:8188/history/{id}` | получить историю           | output info      |

---

## 🧩 GenerationService changes

| Было                      | Стало                           | Зачем                                |
| ------------------------- | ------------------------------- | ------------------------------------ |
| mock result URL           | ComfyUI result URL              | real backend pipeline                |
| `RunAiEnhancerViaComfy()` | `RunGenerationViaComfy()`       | общий метод для всех actions         |
| только `ai_enhancer`      | все `serverAction`              | Tools/Templates/Prompt через ComfyUI |
| outputCount mock URLs     | first ComfyUI result duplicated | пока copy pipeline                   |
| prompt 2 photos           | использует первое фото          | временный этап                       |
| fallback                  | input image URL                 | если ComfyUI не смог                 |

---

## 🔁 RunGenerationViaComfy logic

| Шаг | Действие                                                                         |
| --- | -------------------------------------------------------------------------------- |
| 1   | `ExtractUploadedFileName(request)`                                               |
| 2   | проверить файл в `storage/input`                                                 |
| 3   | создать `~/ComfyUI/input`                                                        |
| 4   | copy backend input → ComfyUI input                                               |
| 5   | `output_prefix = "pixo_" + server_action + "_" + task_id`                        |
| 6   | `workflow_builder_.BuildWorkflow(server_action, input_file_name, output_prefix)` |
| 7   | `comfy_client_.QueuePrompt(workflow)`                                            |
| 8   | `comfy_client_.WaitForFirstOutputFile(prompt_id)`                                |
| 9   | copy ComfyUI output → `storage/output`                                           |
| 10  | return `/outputs/file.png`                                                       |

---

## 📱 Android test matrix

| Screen           | serverAction   | Expected backend path              | Expected result      |
| ---------------- | -------------- | ---------------------------------- | -------------------- |
| AI Enhancer      | `ai_enhancer`  | ComfyUI workflow                   | result image visible |
| Ghibli           | `ghibli`       | ComfyUI workflow                   | result image visible |
| Ghostface        | `ghostface`    | ComfyUI workflow                   | result image visible |
| Template cherry  | `template`     | template prompt + ComfyUI workflow | result image visible |
| Template blossom | `template`     | template prompt + ComfyUI workflow | result image visible |
| Prompt 1 photo   | `prompt`       | first image → ComfyUI              | result image visible |
| Prompt 2 photos  | `prompt`       | first image only for now           | result image visible |
| Upload preview   | `/uploads/...` | backend file serving               | image visible        |
| Result screen    | `/outputs/...` | backend output serving             | image visible        |

---

## 🧼 HTTP cleanup

| File                  | Было                        | Стало                                   | Почему                             |
| --------------------- | --------------------------- | --------------------------------------- | ---------------------------------- |
| `src/http_server.cpp` | noisy `Broken pipe` logs    | ignores normal mobile disconnect errors | Android can close sockets normally |
| `src/api_handler.cpp` | `[SERVE_UPLOAD]` debug logs | removed                                 | clean logs                         |
| `/uploads`            | GET only                    | GET + HEAD                              | Android/Coil diagnostics           |
| `/outputs`            | GET only                    | GET + HEAD                              | Android/Coil diagnostics           |
| responses             | keep-alive possible         | `connection: close`                     | simpler mobile behavior            |

---

## ⚠️ Ошибки и решения

| Error                                                 | Где                  | Причина                                 | Решение                                                        |
| ----------------------------------------------------- | -------------------- | --------------------------------------- | -------------------------------------------------------------- |
| `AttributeError: 'str' object has no attribute 'get'` | ComfyUI              | workflow был не node-object API format  | использовать настоящий workflow `{ "1": {"class_type": ...} }` |
| `Workflow file not found`                             | backend              | нет `workflows/{serverAction}.json`     | создать workflow file                                          |
| `Failed to queue ComfyUI prompt`                      | `/comfy/test-prompt` | ComfyUI не запущен или workflow invalid | запустить ComfyUI, проверить JSON                              |
| `ComfyUI health false`                                | `/comfy/health`      | ComfyUI недоступен                      | проверить `curl 127.0.0.1:8188/system_stats`                   |
| output не появился                                    | ComfyUI              | workflow не выполнился                  | смотреть окно ComfyUI                                          |
| `file_not_found`                                      | `/outputs/file.png`  | output не скопирован                    | проверить `storage/output`                                     |
| `Broken pipe`                                         | server log           | клиент закрыл соединение                | игнорировать / cleanup уже добавлен                            |
| Prompt 2 photos использует 1 фото                     | generation           | multi-image workflow ещё не сделан      | следующий этап `prompt_multi_image`                            |
| AI enhance не улучшает фото                           | workflow             | сейчас copy pipeline                    | позже заменить workflows на real AI                            |

---

## 🧾 Git

| Action             | Command                                                                       |
| ------------------ | ----------------------------------------------------------------------------- |
| status             | `git status`                                                                  |
| add                | `git add .`                                                                   |
| commit base stable | `git commit -m "Refactor generation data layer and stabilize backend errors"` |
| create branch      | `git checkout -b feature/comfyui-worker`                                      |
| commit comfy       | `git commit -m "Route all generation actions through ComfyUI copy workflows"` |
| commit cleanup     | `git commit -m "Clean ComfyUI copy pipeline HTTP handling"`                   |
| push               | `git push -u origin feature/comfyui-worker`                                   |

```bash
cd ~/mobile-assets-backend

git status

git add .

git commit -m "Route all generation actions through ComfyUI copy workflows"

git push -u origin feature/comfyui-worker
```

---

## 🏁 Итог

| Возможность                                | Статус |
| ------------------------------------------ | ------ |
| ComfyUI установлен локально                | ✅      |
| Backend проверяет ComfyUI health           | ✅      |
| Backend отправляет real `/prompt`          | ✅      |
| ComfyUI создаёт output file                | ✅      |
| Backend копирует output в `storage/output` | ✅      |
| Backend отдаёт `/outputs/{file}`           | ✅      |
| Android Result показывает картинку         | ✅      |
| Tools проходят через ComfyUI               | ✅      |
| Templates проходят через ComfyUI           | ✅      |
| Prompt проходит через ComfyUI              | ✅      |
| Mock-only generation заменён copy-pipeline | ✅      |
| Real AI workflows ещё впереди              | ⏳      |

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
