# 🚀 Mobile Assets Backend

Backend для AI-генерации изображений, шаблонов, upload-сервиса, ComfyUI и AI-энхансеров.

---

# 📚 Branches

Каждая большая функциональность развивается в отдельной ветке.

| Branch                             | Описание                       | README         |
| ---------------------------------- | ------------------------------ | -------------- |
| `main`                             | Главная ветка проекта          | текущий README |
| `feature/catalog-api`              | API каталога tools и templates | README branch  |
| `feature/image-upload`             | Upload изображений             | README branch  |
| `feature/task-storage`             | Хранение задач и результатов   | README branch  |
| `template-prompt-mapping`          | Связь templateId → prompt      | README branch  |
| `feature/serve-uploads`            | Раздача uploaded файлов        | README branch  |
| `feature/local-mock-results`       | Mock-результаты генерации      | README branch  |
| `feature/comfyui-worker`           | Интеграция с ComfyUI           | README branch  |
| `feature/prompt-multi-image-comfy` | Multi-image prompt workflow    | README branch  |
| `feature/real-ai-enhancer-upscale` | Реальный upscale workflow      | README branch  |
| `feature/ai-enhancer-ultrasharp`   | UltraSharp enhancer            | README branch  |
| `feature/ai-enhancer-kaggle-comfy` | Kaggle + ComfyUI workflows     | README branch  |

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
