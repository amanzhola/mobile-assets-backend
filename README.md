# 🚀 Mobile Assets Backend

Backend для AI-генерации изображений, шаблонов, upload-сервиса, ComfyUI и AI-энхансеров.

---

# 📚 Branches

Каждая большая функциональность развивается в отдельной ветке и имеет собственный README.

| Branch                             | Назначение                      | Основная функциональность                 | README                                                                                                           |
| ---------------------------------- | ------------------------------- | ----------------------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| `main`                             | Главная ветка проекта           | Навигационный центр и базовая архитектура | текущий README                                                                                                   |
| `feature/catalog-api`              | Каталог инструментов и шаблонов | `/tools`, `/templates`, CatalogService    | [README](https://github.com/amanzhola/mobile-assets-backend/tree/feature/catalog-api/README.md)              |
| `feature/image-upload`             | Загрузка изображений            | multipart upload, imageId                 | [README](https://github.com/amanzhola/mobile-assets-backend/tree/feature/image-upload/README.md)             |
| `feature/task-storage`             | Постоянное хранение задач       | tasks.json, persistence                   | [README](https://github.com/amanzhola/mobile-assets-backend/tree/feature/task-storage/README.md)             |
| `template-prompt-mapping`          | Связь templateId и prompt       | prompt mapping                            | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/template-prompt-mapping/README.md)          |
| `feature/serve-uploads`            | Раздача загруженных файлов      | `/uploads/{file}`                         | [README](https://github.com/amanzhola/mobile-assets-backend/tree/feature/serve-uploads/README.md)            |
| `feature/local-mock-results`       | Mock-генерация                  | fake results                              | [README](https://github.com/amanzhola/mobile-assets-backend/tree/feature/local-mock-results/README.md)       |
| `feature/comfyui-worker`           | Worker для ComfyUI              | queue + workflow execution                | [README](https://github.com/amanzhola/mobile-assets-backend/tree/feature/comfyui-worker/README.md)           |
| `feature/prompt-multi-image-comfy` | Multi-image режим               | 1-4 images                                | [README](https://github.com/amanzhola/mobile-assets-backend/tree/feature/prompt-multi-image-comfy/README.md) |
| `feature/real-ai-enhancer-upscale` | Реальный upscale                | Real workflow                             | [README](https://github.com/amanzhola/mobile-assets-backend/tree/feature/real-ai-enhancer-upscale/README.md) |
| `feature/ai-enhancer-ultrasharp`   | UltraSharp enhancer             | sharpen workflow                          | [README](https://github.com/amanzhola/mobile-assets-backend/tree/feature/ai-enhancer-ultrasharp/README.md)   |
| `feature/ai-enhancer-kaggle-comfy` | Kaggle + ComfyUI                | external workflows                        | [README](https://github.com/amanzhola/mobile-assets-backend/tree/feature/ai-enhancer-kaggle-comfy/README.md) |
| `feature/tools-sdxl-actions`       | SDXL actions для Tools          | общий `tool_img2img.json`, 10 tool actions через ComfyUI SDXL img2img, AI Enhancer отдельно | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/tools-sdxl-actions/README.md) |
| `feature/templates-sdxl-actions`   | SDXL generation для Templates   | 24 templates через `template_img2img.json`, user image участвует в композиции, template background + transparent subject + SDXL blending | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/templates-sdxl-actions/README.md) |
| `feature/local-tool-runner`        | Локальный runner > backend tools| вынесен `remove_background` в `LocalToolRunner`, прямой `rembg` flow без ComfyUI/SDXL, подготовлена архитектура для честных локальных tools | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/local-tool-runner/README.md) |
| `feature/remove-objects-auto-mask` | Remove Objects без ручной mask  | backend сам строит mask по prompt через CLIPSeg и удаляет объект через CPU OpenCV inpaint, без ComfyUI/SDXL и без Android mask drawing | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/remove-objects-auto-mask/README.md) |
| `feature/remove-objects-sam2-flux-fill` | Remove Objects через SAM + ComfyUI inpaint | GroundingDINO/SAM mask generation, ComfyUI inpaint, post-composite original outside mask, лучший текущий auto remove objects backend | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/remove-objects-sam2-flux-fill/README.md) |
| `feature/remove-objects-manual-mask`| Manual cleanup для Remove Objects | второй проход удаления объектов через Android brush mask, `remove_objects_cleanup`, `prepare_manual_cleanup_mask.py`, ComfyUI inpaint и post-composite только по выделенной области | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/remove-objects-manual-mask/README.md) |
| `feature/local-tool-runners`       | Local tool runners split | вынесены `remove_background` и `remove_objects_cleanup` в отдельные local runners | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/local-tool-runners/README.md) |
| `feature/remove-objects-runner`    | Remove Objects runner | добавлен отдельный `RemoveObjectsRunner` для auto remove objects flow | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/remove-objects-runner/README.md) |
| `feature/ai-enhancer-runner`       | AI Enhancer runner | вынесен AI Enhancer flow в отдельный runner | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/ai-enhancer-runner/README.md) |
| `feature/template-runner`          | Template runner | вынесен template generation flow в отдельный runner | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/template-runner/README.md) |
| `feature/tool-action-runner`       | Tool Action runner | общий runner для SDXL tool actions и очистка `GenerationService` от `RunSingleImageViaComfy` | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/tool-action-runner/README.md) |
| `feature/prompt-runner`            | Prompt runner | добавлен prompt runner и collage script для prompt image composition | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/prompt-runner/README.md) |
| `feature/upscale-realesrgan-runner`| RealESRGAN runner | добавлен local upscale runner через `realesrgan-ncnn-vulkan` | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/upscale-realesrgan-runner/README.md) |
| `feature/remove-objects-mask-runner`| Remove Objects mask runner | `LocalToolRunner` переименован и разделён в `RemoveObjectsMaskRunner` | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/remove-objects-mask-runner/README.md) |
| `feature/action-runners-directory` | Action runners directory | все runners перенесены из `local_tools` в `src/action_runners` | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/action-runners-directory/README.md) |
| `feature/generation-action-router` | Generation action router | добавлен `GenerationActionRouter` для маршрутизации actions между runners | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/generation-action-router/README.md) |
| `feature/background-progress-cleanup`| Background progress cleanup | очищен progress flow для remove background runner и router | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/background-progress-cleanup/README.md) |
| `feature/template-workflow-cleanup` | Template workflow cleanup | template workflow logic перенесена в runner, старый `generation_template_workflow` удалён | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/template-workflow-cleanup/README.md) |
| `feature/skin-improve-runner`      | Skin Improve runner | добавлен отдельный `SkinImproveRunner` и подключение через action router | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/skin-improve-runner/README.md) |
| `feature/smile-edit-runner`        | Smile Edit runner | добавлен `SmileEditRunner`, LivePortrait workflow и ComfyClient fixes | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/smile-edit-runner/README.md) |
| `feature/glam-makeup-runner`       | Glam Makeup runner | добавлен `GlamMakeupRunner` как отдельный runner для face editing pipeline | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/glam-makeup-runner/README.md) |
| `feature/prompt-platform`          | Prompt Platform + Face Editing pipeline | модульная архитектура action runners, Qwen prompt translation, локальный Glam Makeup через Face Masks/MediaPipe/OpenCV/Pillow, Remove Objects manual cleanup, AI Enhancer/Upscale/Background/Showcase assets | [README](https://github.com/amanzhola/mobile-assets-backend/blob/feature/prompt-platform/README.md) |

---

# 🖥 Terminal 1 — Build + Run + Git

| Назначение             | Команда                                                                               |
| ---------------------- | ------------------------------------------------------------------------------------- |
| Перейти в проект       | `cd ~/mobile-assets-backend`                                                          |
| Посмотреть ветку       | `git branch`                                                                          |
| Переключиться на main  | `git checkout main`                                                                   |
| Получить изменения     | `git pull`                                                                            |
| Очистить build         | `rm -rf build`                                                                        |
| Создать build          | `mkdir build && cd build`                                                             |
| Установить зависимости | `conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11` |
| Конфигурация           | `cmake .. -DCMAKE_BUILD_TYPE=Debug`                                                   |
| Сборка                 | `cmake --build .`                                                                     |
| Запуск                 | `./bin/mobile_assets_backend`                                                         |
| Проверить процессы     | `ps aux \| grep mobile_assets_backend`                                                |
| Остановить процесс     | `pkill mobile_assets_backend`                                                         |

---

# 🖥 Terminal 2 — API Tests

| Endpoint             | Команда                                                                 |
| -------------------- | ----------------------------------------------------------------------- |
| Health               | `curl http://localhost:8080/health`                                     |
| Tools                | `curl http://localhost:8080/tools`                                      |
| Templates            | `curl http://localhost:8080/templates`                                  |
| Создать задачу       | `curl -X POST http://localhost:8080/generations ...`                    |
| Получить статус      | `curl http://localhost:8080/generations/mock_task_1`                    |
| Получить результат   | `curl http://localhost:8080/generations/mock_task_1/result`             |
| Повторить генерацию  | `curl -X POST http://localhost:8080/generations/mock_task_1/regenerate` |
| Проверить порт       | `ss -tulpn \| grep 8080`                                                |
| Проверить логи       | `tail -f log.txt`                                                       |
| Проверить JSON       | `curl ... \| jq`                                                        |
| Проверить код ответа | `curl -i http://localhost:8080/health`                                  |
| Проверить headers    | `curl -v http://localhost:8080/health`                                  |

---

# 🏗 Архитектура

```text
Android
    ↓
HTTP API
    ↓
ApiHandler
    ↓
GenerationService
    ↓
Storage
    ↓
ComfyUI Worker
    ↓
AI models
```

---

# 📂 Структура проекта

```text
mobile-assets-backend
│
├── src
│
├── data
│
├── storage
│   ├── input
│   └── output
│
├── workflows
│
├── build
│
├── CMakeLists.txt
├── conanfile.txt
├── .gitignore
└── README.md
```

---

# 📂 src

```text
src
├── main.cpp
├── http_server.h
├── http_server.cpp
├── api_handler.h
├── api_handler.cpp
├── generation_service.h
└── generation_service.cpp
```

---

# 🎯 Ответственность файлов

| Файл                   | Ответственность       |
| ---------------------- | --------------------- |
| `main.cpp`             | запуск приложения     |
| `http_server.*`        | HTTP сервер           |
| `api_handler.*`        | маршруты API          |
| `generation_service.*` | бизнес-логика         |
| `storage/`             | файлы и результаты    |
| `workflows/`           | JSON workflow ComfyUI |
| `data/`                | templates и tools     |

---

# 🔥 Основные endpoints

## Health

```http
GET /health
```

Ответ:

```json
{
  "status":"ok",
  "service":"mobile_assets_backend"
}
```

---

## Создание генерации

```http
POST /generations
```

---

## Получить задачу

```http
GET /generations/{taskId}
```

---

## Получить результат

```http
GET /generations/{taskId}/result
```

---

## Повторить генерацию

```http
POST /generations/{taskId}/regenerate
```

---

# 📦 Зависимости

```text
Boost 1.78
Beast
Asio
Boost.JSON
Threads
Conan
CMake
C++20
```

---

# ⚙️ Сборка

```bash
mkdir build
cd build

conan install .. \
--build=missing \
-s build_type=Debug \
-s compiler.libcxx=libstdc++11

cmake .. -DCMAKE_BUILD_TYPE=Debug

cmake --build .
```

---

# ▶ Запуск

```bash
./bin/mobile_assets_backend
```

---

# 🧪 Проверка

Health:

```bash
curl http://localhost:8080/health
```

---

Создание задачи:

```bash
curl -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
    "toolType":"GHIBLI",
    "serverAction":"ghibli",
    "sourceImageUri":"content://media/external/file/1",
    "prompt":"Transform image into anime inspired look",
    "outputCount":2
}'
```

---

Получить статус:

```bash
curl http://localhost:8080/generations/mock_task_1
```

---

Получить результат:

```bash
curl http://localhost:8080/generations/mock_task_1/result
```

---

# 📁 storage

```text
storage
├── input
├── output
└── tasks.json
```

---

# 📁 data

```text
data
├── templates.json
└── tools.json
```

---

# 📁 workflows

```text
workflows
├── ai_enhancer.json
├── ghibli.json
├── ghostface.json
├── remove_background.json
├── hair_studio.json
├── glam_makeup.json
└── ...
```

---

# 🛣 Roadmap

### ✅ Clean architecture

* main.cpp
* HttpServer
* ApiHandler
* GenerationService

---

### ⏳ Catalog API

```text
GET /tools
GET /templates
```

---

### ⏳ Upload service

```text
POST /images/upload
```

---

### ⏳ Task persistence

```text
storage/tasks.json
```

---

### ⏳ Static uploads

```text
GET /uploads/{file}
```

---

### ⏳ ComfyUI Worker

```text
queue
workflow execution
background jobs
```

---

### ⏳ Multi-image prompt

```text
1-4 images
prompt mode
```

---

### ⏳ Real AI enhancer

```text
upscale
UltraSharp
Kaggle
ComfyUI
```

---

# ⚠️ Частые ошибки

## task_not_found

Причина:

```text
сервер был перезапущен
```

Пока задачи живут только в памяти:

```cpp
unordered_map<string, GenerationTask>
```

Решение:

```text
feature/task-storage
```

---

## end of stream [beast.http:1]

Не ошибка.

Это:

```text
curl закрыл соединение
```

Можно игнорировать.

---

## Unknown serverAction

Причина:

```text
действие отсутствует
```

Решение:

Добавить action в:

```text
GenerationService::IsKnownAction()
```

---

## outputCount must be from 1 to 4

Допустимый диапазон:

```text
1 ≤ outputCount ≤ 4
```

---

## bad_json

Тело запроса содержит некорректный JSON.

---

# 🌳 Git Flow

```text
main
│
├── feature/catalog-api
├── feature/image-upload
├── feature/task-storage
├── template-prompt-mapping
├── feature/serve-uploads
├── feature/local-mock-results
├── feature/comfyui-worker
├── feature/prompt-multi-image-comfy
├── feature/real-ai-enhancer-upscale
├── feature/ai-enhancer-ultrasharp
└── feature/ai-enhancer-kaggle-comfy
```

---

# 🚀 Главная идея

```text
main
=
чистая архитектура
+
навигационный центр

Каждая feature-ветка развивается независимо
и имеет собственный README.
```
