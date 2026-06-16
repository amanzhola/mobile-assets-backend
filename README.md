# 💾 feature/task-storage

| Branch                 | Parent                                | Назначение                  | Главная идея                          | Что добавлено                          | Persistence        | API              | Назад                                                                  |
| ---------------------- | ------------------------------------- | --------------------------- | ------------------------------------- | -------------------------------------- | ------------------ | ---------------- | ---------------------------------------------------------------------- |
| `feature/task-storage` | `main` / после `feature/image-upload` | Сохранение generation tasks | task не должен исчезать после restart | `tasks.json`, LoadTasks(), SaveTasks() | local JSON storage | `/generations/*` | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |

---

# ✅ Что сделано

| Компонент          | Было                   | Стало                   | Назначение                    | Статус |
| ------------------ | ---------------------- | ----------------------- | ----------------------------- | ------ |
| GenerationService  | только RAM             | JSON persistence        | переживать restart            | ✅      |
| storage/tasks.json | отсутствовал           | локальное хранилище     | хранение tasks                | ✅      |
| LoadTasks()        | отсутствовал           | загрузка при старте     | восстановление состояния      | ✅      |
| SaveTasks()        | отсутствовал           | запись после изменений  | сохранение данных             | ✅      |
| next_task_id_      | сбрасывался            | восстанавливается       | уникальные taskId             | ✅      |
| CreateGeneration() | RAM only               | SaveTasks()             | новые задачи сохраняются      | ✅      |
| Regenerate()       | RAM only               | SaveTasks()             | regenerate переживает restart | ✅      |
| GetTask()          | только пока сервер жив | работает после restart  | persistence                   | ✅      |
| GetResult()        | только пока сервер жив | работает после restart  | persistence                   | ✅      |
| main.cpp           | GenerationService()    | GenerationService(path) | подключение storage           | ✅      |

---

# 🧱 Структура

| Root                  | Source                 | Storage            | Runtime              | Build                           |
| --------------------- | ---------------------- | ------------------ | -------------------- | ------------------------------- |
| mobile-assets-backend | generation_service.h   | storage/tasks.json | in-memory map + JSON | build/bin/mobile_assets_backend |
| src                   | generation_service.cpp | storage/           | task cache           | CMake                           |
| api                   | main.cpp               | tasks persistence  | nextTaskId           | Conan                           |

---

# 📦 GenerationService

| Метод              | Назначение         | Запись файла | Используется          |
| ------------------ | ------------------ | ------------ | --------------------- |
| LoadTasks()        | загрузка tasks     | нет          | startup               |
| SaveTasks()        | сохранение tasks   | да           | create/regenerate     |
| CreateGeneration() | создать task       | да           | POST /generations     |
| GetTask()          | вернуть task       | нет          | GET /generations/{id} |
| GetResult()        | вернуть result     | нет          | GET result            |
| Regenerate()       | создать новый task | да           | POST regenerate       |
| MakeTaskId()       | mock_task_N        | nextTaskId   | create                |

---

# 📂 storage

| Файл                   | Назначение        | Git           |
| ---------------------- | ----------------- | ------------- |
| storage/tasks.json     | persistence tasks | не коммитится |
| storage/tasks.json.tmp | временная запись  | удаляется     |
| .gitignore             | скрывает storage  | правильно     |

---

# 🔥 Endpoints

| Method | Endpoint                         | После restart работает | Статус     |
| ------ | -------------------------------- | ---------------------- | ---------- |
| POST   | /generations                     | ✅                      | create     |
| GET    | /generations/{taskId}            | ✅                      | task       |
| GET    | /generations/{taskId}/result     | ✅                      | result     |
| POST   | /generations/{taskId}/regenerate | ✅                      | regenerate |
| GET    | /health                          | не зависит             | health     |

---

# 🔄 Жизненный цикл

| Шаг                        | Что происходит         |
| -------------------------- | ---------------------- |
| CreateGeneration()         | создаётся task         |
| tasks_ map                 | добавление             |
| SaveTasks()                | запись JSON            |
| restart server             | RAM очищается          |
| constructor                | вызывается LoadTasks() |
| tasks.json                 | читается               |
| map восстанавливается      |                        |
| старые task снова доступны |                        |

---

# 📄 Формат tasks.json

```json
{
  "nextTaskId": 5,
  "tasks": [
    {
      "taskId": "mock_task_1",
      "status": "completed",
      "serverAction": "ghibli",
      "toolType": "GHIBLI",
      "prompt": "Transform image",
      "templateId": null,
      "outputCount": 2,
      "resultImageUrls": [
        "...",
        "..."
      ]
    }
  ]
}
```

---

# 🖥 Terminal 1 — branch + build + run

| Шаг              | Команда                                                                             |
| ---------------- | ----------------------------------------------------------------------------------- |
| перейти в проект | cd ~/mobile-assets-backend                                                          |
| main             | git checkout main                                                                   |
| pull             | git pull                                                                            |
| branch           | git checkout -b feature/task-storage                                                |
| storage          | mkdir -p storage                                                                    |
| file             | touch storage/tasks.json                                                            |
| build            | rm -rf build                                                                        |
| mkdir            | mkdir build                                                                         |
| enter            | cd build                                                                            |
| conan            | conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11 |
| cmake            | cmake .. -DCMAKE_BUILD_TYPE=Debug                                                   |
| build            | cmake --build .                                                                     |
| run              | ./bin/mobile_assets_backend                                                         |

```bash
cd ~/mobile-assets-backend

git checkout main
git pull
git checkout -b feature/task-storage

mkdir -p storage
touch storage/tasks.json

rm -rf build
mkdir build
cd build

conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11

cmake .. -DCMAKE_BUILD_TYPE=Debug

cmake --build .

./bin/mobile_assets_backend
```

---

# 🖥 Terminal 2 — проверка persistence

| Проверка         | Команда                             |
| ---------------- | ----------------------------------- |
| create task      | POST /generations                   |
| посмотреть файл  | cat storage/tasks.json              |
| остановить       | Ctrl+C                              |
| запустить снова  | ./bin/mobile_assets_backend         |
| проверить task   | GET /generations/mock_task_1        |
| проверить result | GET /generations/mock_task_1/result |

---

### Create task

```bash
curl -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "toolType":"GHIBLI",
  "serverAction":"ghibli",
  "sourceImageUri":"content://media/external/file/1",
  "prompt":"Transform image into anime inspired look",
  "options":{},
  "outputCount":2
}'
```

---

### Проверить storage

```bash
cat ~/mobile-assets-backend/storage/tasks.json
```

---

### Остановить сервер

```bash
Ctrl+C
```

---

### Снова запустить

```bash
cd ~/mobile-assets-backend/build

./bin/mobile_assets_backend
```

---

### Проверить старый task

```bash
curl http://localhost:8080/generations/mock_task_1
```

---

### Проверить result

```bash
curl http://localhost:8080/generations/mock_task_1/result
```

---

# 📄 Изменённые файлы

| Файл                       | Изменение               |
| -------------------------- | ----------------------- |
| src/generation_service.h   | полностью заменён       |
| src/generation_service.cpp | полностью заменён       |
| src/main.cpp               | GenerationService(path) |
| storage/tasks.json         | создан                  |
| .gitignore                 | storage скрыт           |

---

# ⚙ Constructor

Было:

```cpp
generation::GenerationService generation_service;
```

Стало:

```cpp
generation::GenerationService generation_service{
    fs::path{"../storage/tasks.json"}
};
```

---

# 🧠 Внутренние структуры

| Поле           | Тип              | Назначение        |
| -------------- | ---------------- | ----------------- |
| storage_file_  | filesystem::path | путь к tasks.json |
| next_task_id_  | atomic_uint64_t  | следующий номер   |
| tasks_         | unordered_map    | cache tasks       |
| GenerationTask | struct           | одна generation   |

---

# 🔥 Что теперь переживает restart

| Возможность  | Было | Стало |
| ------------ | ---- | ----- |
| task storage | ❌    | ✅     |
| mock_task_1  | ❌    | ✅     |
| nextTaskId   | ❌    | ✅     |
| regenerate   | ❌    | ✅     |
| result urls  | ❌    | ✅     |
| GET task     | ❌    | ✅     |
| GET result   | ❌    | ✅     |

---

# ⚠ Возможные ошибки

| Ошибка                           | Причина                   | Решение                      |
| -------------------------------- | ------------------------- | ---------------------------- |
| Failed to open task storage file | нет storage               | mkdir -p storage             |
| parse error                      | битый JSON                | удалить tasks.json           |
| task_not_found                   | task отсутствует          | создать task                 |
| nextTaskId снова 1               | пустой файл               | проверить SaveTasks()        |
| после restart ничего нет         | LoadTasks() не вызвался   | проверить constructor        |
| пустой tasks.json                | SaveTasks() не вызывается | проверить CreateGeneration() |

---

# Git

```bash
cd ~/mobile-assets-backend

git status

git add .

git commit -m "Persist generation tasks to local storage"

git push -u origin feature/task-storage
```

---

# 🏁 После feature/task-storage

| Возможность                              | Статус |
| ---------------------------------------- | ------ |
| tasks живут после restart                | ✅      |
| taskId продолжает расти                  | ✅      |
| regenerate сохраняется                   | ✅      |
| /generations/{id} работает после restart | ✅      |
| /result работает после restart           | ✅      |

---

# ⬅️ Назад

### Main README

https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md
