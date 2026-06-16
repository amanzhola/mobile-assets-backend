# 📡 feature/serve-uploads

| Branch                  | Parent                            | Назначение                                    | Главная идея                                                                                         | Что исправлено перед стартом                                          | Что добавлено                                                                       | API                                              | Назад                                                                                         |
| ----------------------- | --------------------------------- | --------------------------------------------- | ---------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------- | ----------------------------------------------------------------------------------- | ------------------------------------------------ | --------------------------------------------------------------------------------------------- |
| `feature/serve-uploads` | `feature/template-prompt-mapping` | Раздавать загруженные изображения по HTTP URL | После upload backend возвращает `imageUrl`, а Android может открыть файл через `/uploads/{fileName}` | `storage/` добавлен в `.gitignore`, `storage/tasks.json` убран из Git | `GET /uploads/{fileName}`, `imageUrl` в upload response, content-type по расширению | `POST /images/upload`, `GET /uploads/{fileName}` | [Back to main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## ✅ Что сделано

| Категория       | Файл / команда           | Было                                                 | Стало                                         | Зачем                                   | Статус |
| --------------- | ------------------------ | ---------------------------------------------------- | --------------------------------------------- | --------------------------------------- | ------ |
| Git cleanup     | `git reset storage`      | `storage/` мог попасть в staged                      | `storage/` убран из staged                    | Runtime-файлы не коммитятся             | ✅      |
| Git ignore      | `.gitignore`             | Не было полного правила                              | `build/`, `storage/`, `*.tmp`                 | Игнорировать build и runtime storage    | ✅      |
| Branch          | `feature/serve-uploads`  | `feature/template-prompt-mapping`                    | Новая ветка                                   | Добавить static serving uploaded images | ✅      |
| Upload response | `src/upload_service.cpp` | `imageId`, `fileName`, `path`, `contentType`, `size` | Добавлен `imageUrl`                           | Android получает URL файла              | ✅      |
| Upload service  | `src/upload_service.h`   | Не было file lookup                                  | `GetFilePath()`, `GetContentTypeByFileName()` | Найти файл и определить MIME type       | ✅      |
| API handler     | `src/api_handler.h`      | Не было file serving method                          | `ServeUploadedFile()`                         | Отдавать файл по HTTP                   | ✅      |
| API handler     | `src/api_handler.cpp`    | Не было route `/uploads/`                            | `GET /uploads/{fileName}`                     | Android может загрузить картинку        | ✅      |
| Security        | `GetFilePath()`          | Любой file path                                      | Запрет `/` и `\` в имени                      | Защита от path traversal                | ✅      |
| Android testing | emulator / phone         | Upload был без просмотра                             | Upload + URL preview                          | Можно тестировать приложение            | ✅      |

---

## 🧱 Структура ветки

| Root                     | src                                                                     | storage                         | API layer                               | Service layer   | Android                              |
| ------------------------ | ----------------------------------------------------------------------- | ------------------------------- | --------------------------------------- | --------------- | ------------------------------------ |
| `mobile-assets-backend/` | `api_handler.*`, `upload_service.*`, `main.cpp`, `generation_service.*` | `storage/input/` ignored by Git | `/images/upload`, `/uploads/{fileName}` | `UploadService` | получает `imageUrl` и открывает файл |

---

## 🧭 Архитектура upload + serve

| Client            | Upload request                  | Backend save               | Upload response                 | Serve request               | Serve response        |
| ----------------- | ------------------------------- | -------------------------- | ------------------------------- | --------------------------- | --------------------- |
| Android           | `POST /images/upload`           | `storage/input/img_...svg` | `imageUrl: /uploads/img_...svg` | `GET /uploads/img_...svg`   | binary image body     |
| curl              | raw `--data-binary`             | local file                 | JSON metadata                   | curl image URL              | file content          |
| Future generation | uploaded `imageUrl` / `imageId` | input file exists          | can be mapped to workflow input | ComfyUI can read local file | result pipeline ready |

---

## 🔥 Endpoints после ветки

| Method | Endpoint                       | Input                 | Output                   | Success  | Error                                            |
| ------ | ------------------------------ | --------------------- | ------------------------ | -------- | ------------------------------------------------ |
| `GET`  | `/health`                      | none                  | health JSON              | `200 OK` | none                                             |
| `GET`  | `/tools`                       | none                  | tools JSON               | `200 OK` | `catalog_error`                                  |
| `GET`  | `/templates`                   | none                  | templates JSON           | `200 OK` | `catalog_error`                                  |
| `POST` | `/images/upload`               | raw binary image body | upload JSON + `imageUrl` | `200 OK` | `upload_failed`                                  |
| `GET`  | `/uploads/{fileName}`          | file name             | binary file body         | `200 OK` | `file_not_found`, `bad_upload_path`              |
| `POST` | `/generations`                 | generation JSON       | task JSON                | `200 OK` | `bad_json`, `bad_request`, `unknown_template_id` |
| `GET`  | `/generations/{taskId}`        | task id               | task JSON                | `200 OK` | `task_not_found`                                 |
| `GET`  | `/generations/{taskId}/result` | task id               | result URLs              | `200 OK` | `task_not_found`                                 |

---

## 🖥 Terminal 1 — cleanup + branch + build + run

| Шаг | Команда                                                                               | Что делает                                 |
| --- | ------------------------------------------------------------------------------------- | ------------------------------------------ |
| 1   | `cd ~/mobile-assets-backend`                                                          | перейти в проект                           |
| 2   | `git reset storage`                                                                   | убрать `storage/` из staged                |
| 3   | `nano .gitignore`                                                                     | открыть ignore-файл                        |
| 4   | добавить `build/`, `storage/`, `*.tmp`                                                | игнорировать runtime/build                 |
| 5   | `git add .gitignore src/generation_service.cpp src/generation_service.h src/main.cpp` | добавить только нужные файлы прошлого шага |
| 6   | `git commit -m "Map template ids to backend prompts"`                                 | закрыть template mapping                   |
| 7   | `git push -u origin feature/template-prompt-mapping`                                  | отправить ветку                            |
| 8   | `git checkout -b feature/serve-uploads`                                               | создать новую ветку                        |
| 9   | изменить `upload_service.*`, `api_handler.*`                                          | добавить file serving                      |
| 10  | `rm -rf build && mkdir build && cd build`                                             | чистая сборка                              |
| 11  | `conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11` | зависимости                                |
| 12  | `cmake .. -DCMAKE_BUILD_TYPE=Debug`                                                   | configure                                  |
| 13  | `cmake --build .`                                                                     | build                                      |
| 14  | `./bin/mobile_assets_backend`                                                         | run                                        |

```bash
cd ~/mobile-assets-backend

git reset storage

nano .gitignore
```

```gitignore
build/
storage/
*.tmp
```

```bash
git add .gitignore src/generation_service.cpp src/generation_service.h src/main.cpp

git commit -m "Map template ids to backend prompts"

git push -u origin feature/template-prompt-mapping

git checkout -b feature/serve-uploads

rm -rf build
mkdir build
cd build

conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11

cmake .. -DCMAKE_BUILD_TYPE=Debug

cmake --build .

./bin/mobile_assets_backend
```

---

## 🖥 Terminal 2 — upload + serve проверка

| Проверка          | Команда                                            | Ожидание            |
| ----------------- | -------------------------------------------------- | ------------------- |
| Health            | `curl http://localhost:8080/health`                | backend alive       |
| Upload SVG        | `POST /images/upload` с `image/svg+xml`            | JSON с `imageUrl`   |
| Проверить storage | `ls -lh storage/input`                             | файл лежит на диске |
| Serve file        | `curl http://localhost:8080/uploads/FILE_NAME.svg` | содержимое файла    |
| Not found         | `curl http://localhost:8080/uploads/no_file.png`   | `file_not_found`    |
| Bad path          | `curl http://localhost:8080/uploads/../secret`     | `bad_upload_path`   |

```bash
curl http://localhost:8080/health

curl -X POST http://localhost:8080/images/upload \
  -H "Content-Type: image/svg+xml" \
  --data-binary "@/home/ubuntu/cppbackend/sprint4/problems/leave_game/precode/static/images/cube.svg"; echo

ls -lh ~/mobile-assets-backend/storage/input
```

После upload взять `fileName` из ответа и проверить:

```bash
curl http://localhost:8080/uploads/ИМЯ_ФАЙЛА.svg
```

---

## 📤 Новый upload response

| Field         | Было | Стало | Назначение                |
| ------------- | ---- | ----- | ------------------------- |
| `imageId`     | ✅    | ✅     | внутренний id изображения |
| `fileName`    | ✅    | ✅     | имя файла на диске        |
| `path`        | ✅    | ✅     | путь к файлу на backend   |
| `contentType` | ✅    | ✅     | MIME type upload-запроса  |
| `size`        | ✅    | ✅     | размер body               |
| `imageUrl`    | ❌    | ✅     | HTTP URL для Android      |

```json
{
  "imageId": "img_123456789_abc123",
  "fileName": "img_123456789_abc123.svg",
  "path": "../storage/input/img_123456789_abc123.svg",
  "contentType": "image/svg+xml",
  "size": 1234,
  "imageUrl": "/uploads/img_123456789_abc123.svg"
}
```

---

## 📄 `.gitignore`

| Pattern    | Зачем                                |
| ---------- | ------------------------------------ |
| `build/`   | не коммитить build artifacts         |
| `storage/` | не коммитить runtime uploads и tasks |
| `*.tmp`    | не коммитить временные файлы         |

```gitignore
build/
storage/
*.tmp
```

---

## 📄 Изменения в `src/upload_service.h`

| Метод                                 | Назначение                         | Защита                              |
| ------------------------------------- | ---------------------------------- | ----------------------------------- |
| `GetFilePath(file_name)`              | получить путь к uploaded file      | запрещает `/` и `\`                 |
| `GetContentTypeByFileName(file_name)` | определить MIME type по расширению | fallback `application/octet-stream` |

```cpp
std::filesystem::path GetFilePath(const std::string& file_name) const;

std::string GetContentTypeByFileName(const std::string& file_name) const;
```

---

## 📄 Изменения в `src/upload_service.cpp`

| Место                     | Изменение                                                             | Зачем                                         |
| ------------------------- | --------------------------------------------------------------------- | --------------------------------------------- |
| `SaveRawImage()` response | `response["imageUrl"] = "/uploads/" + file_path.filename().string();` | вернуть URL файла                             |
| конец файла               | `GetFilePath()`                                                       | безопасный путь к файлу                       |
| конец файла               | `GetContentTypeByFileName()`                                          | правильный `Content-Type` для browser/Android |

```cpp
response["imageUrl"] = "/uploads/" + file_path.filename().string();
```

```cpp
std::filesystem::path UploadService::GetFilePath(const std::string& file_name) const {
    if (file_name.find('/') != std::string::npos || file_name.find('\\') != std::string::npos) {
        throw std::runtime_error("Invalid file name");
    }

    return input_dir_ / file_name;
}

std::string UploadService::GetContentTypeByFileName(const std::string& file_name) const {
    if (file_name.ends_with(".png")) return "image/png";
    if (file_name.ends_with(".jpg")) return "image/jpeg";
    if (file_name.ends_with(".jpeg")) return "image/jpeg";
    if (file_name.ends_with(".webp")) return "image/webp";
    if (file_name.ends_with(".svg")) return "image/svg+xml";

    return "application/octet-stream";
}
```

---

## 📄 Изменения в `src/api_handler.h`

| Метод                                   | Назначение                                                |
| --------------------------------------- | --------------------------------------------------------- |
| `ServeUploadedFile(request, file_name)` | открыть файл из `storage/input` и вернуть binary response |

```cpp
http::response<http::string_body> ServeUploadedFile(
    const http::request<http::string_body>& request,
    const std::string& file_name
);
```

---

## 📄 Изменения в `src/api_handler.cpp`

| Место      | Изменение                                         |
| ---------- | ------------------------------------------------- |
| include    | добавить `<fstream>`, `<iterator>`                |
| new method | `ApiHandler::ServeUploadedFile(...)`              |
| `Handle()` | route `GET /uploads/{fileName}` до `/generations` |

```cpp
#include <fstream>
#include <iterator>
```

```cpp
http::response<http::string_body> ApiHandler::ServeUploadedFile(
    const http::request<http::string_body>& request,
    const std::string& file_name
) {
    try {
        const auto path = upload_service_.GetFilePath(file_name);

        std::ifstream input(path, std::ios::binary);

        if (!input.is_open()) {
            return JsonResponse(
                request,
                MakeError("file_not_found", "Uploaded file not found"),
                http::status::not_found
            );
        }

        std::string body{
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        };

        http::response<http::string_body> response{http::status::ok, request.version()};
        response.set(http::field::content_type, upload_service_.GetContentTypeByFileName(file_name));
        response.set(http::field::cache_control, "no-cache");
        response.keep_alive(request.keep_alive());
        response.body() = std::move(body);
        response.prepare_payload();

        return response;

    } catch (const std::exception& e) {
        return JsonResponse(
            request,
            MakeError("bad_upload_path", e.what()),
            http::status::bad_request
        );
    }
}
```

Route:

```cpp
constexpr std::string_view uploads_prefix = "/uploads/";

if (request.method() == http::verb::get && target.starts_with(uploads_prefix)) {
    return ServeUploadedFile(
        request,
        target.substr(uploads_prefix.size())
    );
}
```

---

## ⚠️ Ошибки и решения

| Error                               | Где                   | Причина                                        | Решение                                      |
| ----------------------------------- | --------------------- | ---------------------------------------------- | -------------------------------------------- |
| `storage` staged                    | Git                   | `git add .` добавил runtime storage            | `git reset storage`                          |
| `file_not_found`                    | `GET /uploads/{file}` | файла нет в `storage/input`                    | проверить `ls storage/input`                 |
| `bad_upload_path`                   | `GET /uploads/../...` | запрещённый путь                               | использовать только fileName                 |
| browser не показывает SVG           | response              | неправильный content type                      | проверить `.svg -> image/svg+xml`            |
| `.svg` сохранился как `.bin`        | upload                | `ExtensionFromContentType` не поддерживает svg | добавить `image/svg+xml -> .svg`, если нужно |
| compile error `ifstream`            | build                 | нет include                                    | добавить `<fstream>`                         |
| compile error `istreambuf_iterator` | build                 | нет include                                    | добавить `<iterator>`                        |
| endpoint не работает                | runtime               | route поставлен после generic routes           | поставить `/uploads/` до `/generations`      |

---

## 🤖 Android test

| Environment       | Команда / URL                   | Когда использовать             |
| ----------------- | ------------------------------- | ------------------------------ |
| Android Emulator  | `adb reverse tcp:8080 tcp:8080` | emulator на той же машине      |
| Emulator base URL | `http://127.0.0.1:8080`         | после `adb reverse`            |
| Physical phone    | `http://192.168.0.15:8080`      | телефон и PC в одной Wi-Fi     |
| First request     | `GET /health`                   | проверить связь                |
| Catalog           | `GET /tools`, `GET /templates`  | загрузить metadata             |
| Upload            | `POST /images/upload`           | загрузить картинку             |
| Serve uploaded    | `GET /uploads/{fileName}`       | проверить отображение картинки |
| Generation        | `POST /generations`             | создать task                   |

```bash
adb reverse tcp:8080 tcp:8080
```

Android base URL:

```text
http://127.0.0.1:8080
```

---

## 🧪 Android flow после ветки

| Step | Request                            | Result                               |
| ---- | ---------------------------------- | ------------------------------------ |
| 1    | `GET /health`                      | backend доступен                     |
| 2    | `GET /tools`                       | Android получил tools                |
| 3    | `GET /templates`                   | Android получил templates            |
| 4    | `POST /images/upload`              | backend вернул `imageId`, `imageUrl` |
| 5    | `GET /uploads/{fileName}`          | Android может открыть uploaded image |
| 6    | `POST /generations`                | task created                         |
| 7    | `GET /generations/{taskId}`        | status                               |
| 8    | `GET /generations/{taskId}/result` | result                               |

---

## 🧾 Git

| Action | Command                                    |
| ------ | ------------------------------------------ |
| status | `git status`                               |
| add    | `git add .`                                |
| commit | `git commit -m "Serve uploaded images"`    |
| push   | `git push -u origin feature/serve-uploads` |

```bash
cd ~/mobile-assets-backend

git status

git add .

git commit -m "Serve uploaded images"

git push -u origin feature/serve-uploads
```

---

## 🏁 Итог

| Возможность                                | Статус |
| ------------------------------------------ | ------ |
| `storage/` не коммитится                   | ✅      |
| upload response содержит `imageUrl`        | ✅      |
| backend отдаёт uploaded files              | ✅      |
| `/uploads/{fileName}` работает             | ✅      |
| content-type определяется по расширению    | ✅      |
| path traversal заблокирован                | ✅      |
| Android можно подключать для первого теста | ✅      |

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
