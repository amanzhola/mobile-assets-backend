# 📚 feature/catalog-api

| Branch                | Parent | Назначение                        | Главная идея                                                                        | Что добавлено                                                            | API                                                                   | Данные                 | Назад                               |
| --------------------- | ------ | --------------------------------- | ----------------------------------------------------------------------------------- | ------------------------------------------------------------------------ | --------------------------------------------------------------------- | ---------------------- | ----------------------------------- |
| `feature/catalog-api` | `main` | Добавить metadata API для Android | Android получает tools/templates с backend, а не хранит их жёстко внутри приложения | `CatalogService`, `tools.json`, `templates.json`, `/tools`, `/templates` | `GET /tools`, `GET /templates`, старые generation endpoints сохранены | 11 tools, 24 templates | [Back to main README](../README.md) |

---

## ✅ Что сделано

| Категория | Файл / endpoint                     | Было                       | Стало                                | Зачем                                | Статус |
| --------- | ----------------------------------- | -------------------------- | ------------------------------------ | ------------------------------------ | ------ |
| Branch    | `feature/catalog-api`               | Только `main`              | Отдельная ветка каталога             | Изолировать metadata API             | ✅      |
| Data      | `data/tools.json`                   | Не было                    | 11 tools                             | Android получает список инструментов | ✅      |
| Data      | `data/templates.json`               | Не было                    | 24 templates                         | Android получает список шаблонов     | ✅      |
| Service   | `src/catalog_service.h`             | Не было                    | Интерфейс `CatalogService`           | Читать JSON-каталог                  | ✅      |
| Service   | `src/catalog_service.cpp`           | Не было                    | Загрузка и парсинг JSON-файлов       | Переиспользуемая логика каталога     | ✅      |
| API       | `GET /tools`                        | Не было                    | Возвращает tools JSON                | Metadata для Android                 | ✅      |
| API       | `GET /templates`                    | Не было                    | Возвращает templates JSON            | Metadata для Android                 | ✅      |
| API       | `POST /generations`                 | Было                       | Сохранено                            | Mock generation работает дальше      | ✅      |
| API       | `GET /generations/{id}`             | Было                       | Сохранено                            | Проверка task status                 | ✅      |
| API       | `GET /generations/{id}/result`      | Было                       | Сохранено                            | Получение result URLs                | ✅      |
| API       | `POST /generations/{id}/regenerate` | Было                       | Сохранено                            | Повтор генерации                     | ✅      |
| Build     | `CMakeLists.txt`                    | Без catalog service        | Добавлены `catalog_service.cpp/.h`   | Сборка новых файлов                  | ✅      |
| Main      | `src/main.cpp`                      | Только `GenerationService` | `GenerationService + CatalogService` | Dependency injection в `ApiHandler`  | ✅      |
| Handler   | `src/api_handler.h/.cpp`            | Generation routes          | Generation + Catalog routes          | Единая точка API                     | ✅      |

---

## 🧱 Структура ветки

| Root                     | src                                                                                       | data                                              | build                             | API layer    | Business layer                        | Storage layer        |
| ------------------------ | ----------------------------------------------------------------------------------------- | ------------------------------------------------- | --------------------------------- | ------------ | ------------------------------------- | -------------------- |
| `mobile-assets-backend/` | `main.cpp`, `http_server.*`, `api_handler.*`, `generation_service.*`, `catalog_service.*` | `tools.json`, `templates.json`, `onboarding.json` | `build/bin/mobile_assets_backend` | `ApiHandler` | `GenerationService`, `CatalogService` | JSON-файлы в `data/` |

---

## 🧭 Архитектура

| Client  | HTTP                                | ApiHandler           | Service                                 | Source                | Response        |
| ------- | ----------------------------------- | -------------------- | --------------------------------------- | --------------------- | --------------- |
| Android | `GET /tools`                        | route `/tools`       | `CatalogService::GetTools()`            | `data/tools.json`     | JSON tools      |
| Android | `GET /templates`                    | route `/templates`   | `CatalogService::GetTemplates()`        | `data/templates.json` | JSON templates  |
| Android | `POST /generations`                 | route `/generations` | `GenerationService::CreateGeneration()` | memory mock tasks     | task JSON       |
| Android | `GET /generations/{id}`             | route task id        | `GenerationService::GetTask()`          | memory mock tasks     | status JSON     |
| Android | `GET /generations/{id}/result`      | route result         | `GenerationService::GetResult()`        | memory mock tasks     | result URLs     |
| Android | `POST /generations/{id}/regenerate` | route regenerate     | `GenerationService::Regenerate()`       | memory mock tasks     | new result JSON |

---

## 🔥 Endpoints

| Method | Endpoint                           | Input     | Output              | Success  | Error                                             |
| ------ | ---------------------------------- | --------- | ------------------- | -------- | ------------------------------------------------- |
| `GET`  | `/health`                          | none      | `{status, service}` | `200 OK` | none                                              |
| `GET`  | `/tools`                           | none      | `{tools:[...]}`     | `200 OK` | `catalog_error`                                   |
| `GET`  | `/templates`                       | none      | `{templates:[...]}` | `200 OK` | `catalog_error`                                   |
| `POST` | `/generations`                     | JSON body | task object         | `200 OK` | `bad_json`, `bad_request`, `Unknown serverAction` |
| `GET`  | `/generations/{taskId}`            | task id   | task object         | `200 OK` | `task_not_found`                                  |
| `GET`  | `/generations/{taskId}/result`     | task id   | result URLs         | `200 OK` | `task_not_found`                                  |
| `POST` | `/generations/{taskId}/regenerate` | task id   | regenerated task    | `200 OK` | `task_not_found`                                  |

---

## 📦 Tools и Templates

| Type      | Count | File                  | JSON root   | Используется для        | Примеры                                                                                                                                                                                                                                                                                                                                                                    |
| --------- | ----: | --------------------- | ----------- | ----------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Tools     |    11 | `data/tools.json`     | `tools`     | Главные AI-инструменты  | `ai_enhancer`, `glam_makeup`, `remove_objects`, `remove_background`, `skin_improve`, `upscale_image`, `change_scene`, `hair_studio`, `smile_edit`, `ghostface`, `ghibli`                                                                                                                                                                                                   |
| Templates |    24 | `data/templates.json` | `templates` | Готовые стили / пресеты | `gloria_model`, `cherry`, `travel_style`, `one_love`, `warm_day`, `pink_captivity`, `80s_gloss`, `match_point`, `japan_breathe`, `easter_morning`, `sea_breathe`, `blossom`, `darning_noir`, `love_in_paris`, `queen_of_the_day`, `old_money_muse`, `sport_and_healthy`, `rapunzel_glow`, `safary`, `housewives`, `morning_routine`, `oscar`, `retro_style`, `metro_style` |

---

## 🖥 Terminal 1 — build and run

| Step | Command                                                                               | Expected                               |
| ---- | ------------------------------------------------------------------------------------- | -------------------------------------- |
| 1    | `cd ~/mobile-assets-backend`                                                          | переход в проект                       |
| 2    | `git checkout feature/catalog-api`                                                    | ветка catalog API                      |
| 3    | `rm -rf build && mkdir build && cd build`                                             | чистая сборка                          |
| 4    | `conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11` | зависимости установлены                |
| 5    | `cmake .. -DCMAKE_BUILD_TYPE=Debug`                                                   | CMake configured                       |
| 6    | `cmake --build .`                                                                     | binary собран                          |
| 7    | `./bin/mobile_assets_backend`                                                         | `PIXO backend started on 0.0.0.0:8080` |

```bash
cd ~/mobile-assets-backend
git checkout feature/catalog-api

rm -rf build
mkdir build
cd build

conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .

./bin/mobile_assets_backend
```

---

## 🖥 Terminal 2 — проверки API

| Check             | Command                                                                 | Expected                                            |
| ----------------- | ----------------------------------------------------------------------- | --------------------------------------------------- |
| Health            | `curl http://localhost:8080/health`                                     | `{"status":"ok","service":"mobile_assets_backend"}` |
| Tools             | `curl http://localhost:8080/tools`                                      | JSON с массивом `tools`                             |
| Templates         | `curl http://localhost:8080/templates`                                  | JSON с массивом `templates`                         |
| Create generation | `curl -X POST http://localhost:8080/generations ...`                    | `taskId`, `status`, `resultImageUrls`               |
| Get task          | `curl http://localhost:8080/generations/mock_task_1`                    | task JSON                                           |
| Get result        | `curl http://localhost:8080/generations/mock_task_1/result`             | result URLs                                         |
| Regenerate        | `curl -X POST http://localhost:8080/generations/mock_task_1/regenerate` | regenerated result                                  |

```bash
curl http://localhost:8080/health

curl http://localhost:8080/tools

curl http://localhost:8080/templates

curl -X POST http://localhost:8080/generations \
  -H "Content-Type: application/json" \
  -d '{
    "toolType": "GHIBLI",
    "serverAction": "ghibli",
    "sourceImageUri": "content://media/external/file/1",
    "prompt": "Transform image into anime inspired look",
    "options": {},
    "outputCount": 2
  }'

curl http://localhost:8080/generations/mock_task_1

curl http://localhost:8080/generations/mock_task_1/result

curl -X POST http://localhost:8080/generations/mock_task_1/regenerate
```

---

## 🧪 Пример успешного POST `/generations`

| Field             | Value                                                                                                              |
| ----------------- | ------------------------------------------------------------------------------------------------------------------ |
| `taskId`          | `mock_task_1`                                                                                                      |
| `status`          | `completed`                                                                                                        |
| `serverAction`    | `ghibli`                                                                                                           |
| `toolType`        | `GHIBLI`                                                                                                           |
| `workflow`        | `workflows/ghibli.json`                                                                                            |
| `imageCount`      | `1`                                                                                                                |
| `outputCount`     | `2`                                                                                                                |
| `resultImageUrls` | `https://mock.pixo.ai/results/ghibli_mock_task_1_1.webp`, `https://mock.pixo.ai/results/ghibli_mock_task_1_2.webp` |

---

## ⚠️ Ошибки и решения

| Error                          | Где появляется         | Причина                                   | Проверка                                 | Решение                              |
| ------------------------------ | ---------------------- | ----------------------------------------- | ---------------------------------------- | ------------------------------------ |
| `catalog_error`                | `/tools`, `/templates` | не найден JSON-файл или ошибка парсинга   | `ls data/tools.json data/templates.json` | проверить путь `../data` из `build/` |
| `bad_json`                     | `POST /generations`    | тело запроса не JSON                      | проверить кавычки                        | исправить JSON                       |
| `bad_request`                  | `POST /generations`    | body не объект                            | отправить `{...}`                        | не отправлять массив/строку          |
| `Unknown serverAction`         | `POST /generations`    | action не поддерживается                  | проверить `serverAction`                 | использовать существующий action     |
| `task_not_found`               | `/generations/{id}`    | task не существует или сервер перезапущен | создать новый task                       | повторить POST                       |
| `not_found`                    | любой URL              | неправильный endpoint                     | сверить таблицу endpoints                | исправить URL                        |
| `end of stream [beast.http:1]` | Terminal 1 лог         | curl закрыл соединение                    | не критично                              | можно игнорировать                   |

---

## 🧾 Git

| Action        | Command                                  |
| ------------- | ---------------------------------------- |
| Create branch | `git checkout -b feature/catalog-api`    |
| Check status  | `git status`                             |
| Add files     | `git add .`                              |
| Commit        | `git commit -m "Add PIXO catalog API"`   |
| Push          | `git push -u origin feature/catalog-api` |
| Back to main  | `git checkout main`                      |

```bash
cd ~/mobile-assets-backend

git status
git add .
git commit -m "Add PIXO catalog API"
git push -u origin feature/catalog-api
```

---

## 🏁 Итог

| Branch                | Done                | Android benefit                                  | Backend benefit                  | Next                   |
| --------------------- | ------------------- | ------------------------------------------------ | -------------------------------- | ---------------------- |
| `feature/catalog-api` | ✅ Catalog API готов | Android может получать tools/templates с backend | Backend стал источником metadata | `feature/image-upload` |

---

# ⬅️ Navigation

| Repository               | Branch | README                                    |
| ------------------------ | ------ | ----------------------------------------- |
| 🚀 Mobile Assets Backend | `main` |[⬅️ Back to Main](https://github.com/amazhola/mobile-assets-backend/tree/main)|

---
