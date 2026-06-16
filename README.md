# 🖼️ feature/image-upload

| Branch                 | Parent                               | Назначение                   | Главная идея                                                                                 | Что добавлено                                                                  | API                   | Storage         | Назад                                                                                         |
| ---------------------- | ------------------------------------ | ---------------------------- | -------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------ | --------------------- | --------------- | --------------------------------------------------------------------------------------------- |
| `feature/image-upload` | `main` / после `feature/catalog-api` | Добавить файловый upload API | Android сначала загружает картинку на backend, backend сохраняет файл и возвращает `imageId` | `UploadService`, `/images/upload`, `storage/input`, сохранение raw binary body | `POST /images/upload` | `storage/input` | [Back to main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## ✅ Что сделано

| Категория   | Файл / endpoint          | Было                                  | Стало                                                | Зачем                                               | Статус |
| ----------- | ------------------------ | ------------------------------------- | ---------------------------------------------------- | --------------------------------------------------- | ------ |
| Branch      | `feature/image-upload`   | Каталог и generation API              | Отдельная ветка upload API                           | Изолировать загрузку файлов                         | ✅      |
| Storage     | `storage/input/`         | Не было директории для входных файлов | Папка создаётся для загруженных изображений          | Хранить input images перед ComfyUI                  | ✅      |
| Service     | `src/upload_service.h`   | Не было                               | Интерфейс `UploadService`                            | Сервис сохранения файлов                            | ✅      |
| Service     | `src/upload_service.cpp` | Не было                               | Реализация сохранения raw body                       | Генерация `imageId`, выбор расширения, запись файла | ✅      |
| API         | `POST /images/upload`    | Android слал `content://...`          | Android отправляет файл на backend                   | Backend получает реальный файл                      | ✅      |
| API Handler | `src/api_handler.h`      | `GenerationService + CatalogService`  | Добавлен `UploadService`                             | Handler получил upload dependency                   | ✅      |
| API Handler | `src/api_handler.cpp`    | Не было upload route                  | Добавлен route `/images/upload`                      | Upload работает через HTTP                          | ✅      |
| Main        | `src/main.cpp`           | Нет upload service                    | Создаётся `UploadService{"../storage/input"}`        | Сервис подключён в приложение                       | ✅      |
| Build       | `CMakeLists.txt`         | Без upload files                      | Добавлены `upload_service.cpp/.h`                    | Новые файлы собираются                              | ✅      |
| Response    | JSON                     | Не было                               | `imageId`, `fileName`, `path`, `contentType`, `size` | Android получает данные загруженного файла          | ✅      |

---

## 🧱 Структура ветки

| Root                     | src                                                                                                           | storage          | API layer                   | Service layer                           | Build                                                                |
| ------------------------ | ------------------------------------------------------------------------------------------------------------- | ---------------- | --------------------------- | --------------------------------------- | -------------------------------------------------------------------- |
| `mobile-assets-backend/` | `main.cpp`, `api_handler.*`, `upload_service.*`, `catalog_service.*`, `generation_service.*`, `http_server.*` | `storage/input/` | `ApiHandler::UploadImage()` | `upload::UploadService::SaveRawImage()` | `CMakeLists.txt`, `conanfile.txt`, `build/bin/mobile_assets_backend` |

---

## 🧭 Архитектура upload

| Client         | HTTP                  | ApiHandler             | UploadService                     | Filesystem                      | Response                                                    |
| -------------- | --------------------- | ---------------------- | --------------------------------- | ------------------------------- | ----------------------------------------------------------- |
| Android        | `POST /images/upload` | route `/images/upload` | `SaveRawImage(body, contentType)` | `storage/input/img_...png`      | JSON с `imageId`, `fileName`, `path`, `contentType`, `size` |
| curl           | raw binary body       | `UploadImage(request)` | extension by `Content-Type`       | `.png`, `.jpg`, `.webp`, `.bin` | JSON upload result                                          |
| Future ComfyUI | use `imageId` / path  | generation request     | find uploaded image               | input file exists               | workflow gets local image                                   |

---

## 🔥 Endpoints после ветки

| Method | Endpoint                           | Input                 | Output              | Success  | Error                                             |
| ------ | ---------------------------------- | --------------------- | ------------------- | -------- | ------------------------------------------------- |
| `GET`  | `/health`                          | none                  | `{status, service}` | `200 OK` | none                                              |
| `GET`  | `/tools`                           | none                  | tools JSON          | `200 OK` | `catalog_error`                                   |
| `GET`  | `/templates`                       | none                  | templates JSON      | `200 OK` | `catalog_error`                                   |
| `POST` | `/images/upload`                   | raw binary image body | upload JSON         | `200 OK` | `upload_failed`                                   |
| `POST` | `/generations`                     | JSON generation body  | task JSON           | `200 OK` | `bad_json`, `bad_request`, `Unknown serverAction` |
| `GET`  | `/generations/{taskId}`            | task id               | task JSON           | `200 OK` | `task_not_found`                                  |
| `GET`  | `/generations/{taskId}/result`     | task id               | result URLs         | `200 OK` | `task_not_found`                                  |
| `POST` | `/generations/{taskId}/regenerate` | task id               | regenerated task    | `200 OK` | `task_not_found`                                  |

---

## 📦 UploadService

| Method                                   | Input                                          | Logic                                   | Output                          | Error                                             |
| ---------------------------------------- | ---------------------------------------------- | --------------------------------------- | ------------------------------- | ------------------------------------------------- |
| `UploadService(input_dir)`               | path                                           | создаёт директорию `storage/input`      | service ready                   | filesystem error                                  |
| `MakeImageId()`                          | none                                           | timestamp + random hex                  | `img_<time>_<random>`           | none                                              |
| `ExtensionFromContentType(content_type)` | `image/png`, `image/jpeg`, `image/webp`, other | выбирает расширение                     | `.png`, `.jpg`, `.webp`, `.bin` | none                                              |
| `SaveRawImage(body, content_type)`       | raw body + content type                        | проверяет body, создаёт имя, пишет файл | JSON object                     | `Empty upload body`, `Failed to open upload file` |

---

## 📄 Файлы

| File                     | Action | Content                                                               |
| ------------------------ | ------ | --------------------------------------------------------------------- |
| `src/upload_service.h`   | create | declaration of `upload::UploadService`                                |
| `src/upload_service.cpp` | create | implementation of raw image saving                                    |
| `storage/input/`         | create | folder for uploaded input images                                      |
| `src/api_handler.h`      | modify | include `upload_service.h`, constructor param, field, `UploadImage()` |
| `src/api_handler.cpp`    | modify | constructor injection, route `/images/upload`, method `UploadImage()` |
| `src/main.cpp`           | modify | include upload service, create `upload_service`, pass to `ApiHandler` |
| `CMakeLists.txt`         | modify | add `src/upload_service.cpp`, `src/upload_service.h`                  |

---

## 🖥 Terminal 1 — branch, build, run

| Step | Command                                                                               | Expected                |
| ---- | ------------------------------------------------------------------------------------- | ----------------------- |
| 1    | `cd ~/mobile-assets-backend`                                                          | перейти в проект        |
| 2    | `git checkout -b feature/image-upload`                                                | новая ветка             |
| 3    | `touch src/upload_service.h src/upload_service.cpp`                                   | upload files созданы    |
| 4    | `mkdir -p storage/input`                                                              | input storage создан    |
| 5    | изменить `api_handler.h/.cpp`, `main.cpp`, `CMakeLists.txt`                           | upload подключён        |
| 6    | `rm -rf build && mkdir build && cd build`                                             | чистая сборка           |
| 7    | `conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11` | зависимости установлены |
| 8    | `cmake .. -DCMAKE_BUILD_TYPE=Debug`                                                   | CMake configured        |
| 9    | `cmake --build .`                                                                     | binary собран           |
| 10   | `./bin/mobile_assets_backend`                                                         | backend запущен         |

```bash
cd ~/mobile-assets-backend

git checkout -b feature/image-upload

touch src/upload_service.h
touch src/upload_service.cpp
mkdir -p storage/input

rm -rf build
mkdir build
cd build

conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .

./bin/mobile_assets_backend
```

Expected log:

```text
PIXO backend started on 0.0.0.0:8080
```

---

## 🖥 Terminal 2 — проверки upload API

| Check             | Command                                                                                          | Expected                                         |
| ----------------- | ------------------------------------------------------------------------------------------------ | ------------------------------------------------ |
| Health            | `curl http://localhost:8080/health`                                                              | backend alive                                    |
| Upload PNG        | `curl -X POST ... -H "Content-Type: image/png" --data-binary "@file.png"`                        | JSON with `imageId`                              |
| Upload SVG as SVG | `curl -X POST ... -H "Content-Type: image/svg+xml" --data-binary "@cube.svg"`                    | saved as `.bin` because unsupported content type |
| Check storage     | `ls -lh ~/mobile-assets-backend/storage/input`                                                   | uploaded file exists                             |
| Empty body check  | `curl -X POST http://localhost:8080/images/upload -H "Content-Type: image/png" --data-binary ""` | `upload_failed`                                  |

```bash
curl http://localhost:8080/health

curl -X POST http://localhost:8080/images/upload \
  -H "Content-Type: image/png" \
  --data-binary "@/home/ubuntu/cppbackend/sprint4/problems/leave_game/precode/static/images/cube.svg"

curl -X POST http://localhost:8080/images/upload \
  -H "Content-Type: image/svg+xml" \
  --data-binary "@/home/ubuntu/cppbackend/sprint4/problems/leave_game/precode/static/images/cube.svg"

ls -lh ~/mobile-assets-backend/storage/input
```

---

## 📤 Пример upload response

| Field         | Meaning                   | Example                                     |
| ------------- | ------------------------- | ------------------------------------------- |
| `imageId`     | backend id файла          | `img_123456789_abc123`                      |
| `fileName`    | имя файла на диске        | `img_123456789_abc123.png`                  |
| `path`        | путь к сохранённому файлу | `../storage/input/img_123456789_abc123.png` |
| `contentType` | HTTP content type         | `image/png`                                 |
| `size`        | размер body в байтах      | `12345`                                     |

```json
{
  "imageId": "img_123456789_abc123",
  "fileName": "img_123456789_abc123.png",
  "path": "../storage/input/img_123456789_abc123.png",
  "contentType": "image/png",
  "size": 12345
}
```

---

## 🧩 src/upload_service.h

```cpp
#pragma once

#include <boost/json.hpp>

#include <filesystem>
#include <string>

namespace upload {

namespace json = boost::json;

class UploadService {
public:
    explicit UploadService(std::filesystem::path input_dir);

    json::object SaveRawImage(std::string body, std::string content_type);

private:
    std::string MakeImageId() const;
    std::string ExtensionFromContentType(const std::string& content_type) const;

private:
    std::filesystem::path input_dir_;
};

}  // namespace upload
```

---

## 🧩 src/upload_service.cpp

```cpp
#include "upload_service.h"

#include <chrono>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>

namespace upload {

UploadService::UploadService(std::filesystem::path input_dir)
    : input_dir_{std::move(input_dir)} {
    std::filesystem::create_directories(input_dir_);
}

std::string UploadService::MakeImageId() const {
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();

    static thread_local std::mt19937_64 generator{std::random_device{}()};
    std::uniform_int_distribution<std::uint64_t> dist;

    std::ostringstream out;
    out << "img_" << now << "_" << std::hex << dist(generator);

    return out.str();
}

std::string UploadService::ExtensionFromContentType(const std::string& content_type) const {
    if (content_type.starts_with("image/png")) {
        return ".png";
    }

    if (content_type.starts_with("image/jpeg")) {
        return ".jpg";
    }

    if (content_type.starts_with("image/webp")) {
        return ".webp";
    }

    return ".bin";
}

json::object UploadService::SaveRawImage(std::string body, std::string content_type) {
    if (body.empty()) {
        throw std::runtime_error("Empty upload body");
    }

    const std::string image_id = MakeImageId();
    const std::string extension = ExtensionFromContentType(content_type);

    const auto file_path = input_dir_ / (image_id + extension);

    std::ofstream output(file_path, std::ios::binary);

    if (!output.is_open()) {
        throw std::runtime_error("Failed to open upload file: " + file_path.string());
    }

    output.write(body.data(), static_cast<std::streamsize>(body.size()));

    json::object response;
    response["imageId"] = image_id;
    response["fileName"] = file_path.filename().string();
    response["path"] = file_path.string();
    response["contentType"] = content_type;
    response["size"] = body.size();

    return response;
}

}  // namespace upload
```

---

## 🧩 api_handler changes

| File                  | Add                                                           |
| --------------------- | ------------------------------------------------------------- |
| `src/api_handler.h`   | `#include "upload_service.h"`                                 |
| `src/api_handler.h`   | constructor parameter `upload::UploadService& upload_service` |
| `src/api_handler.h`   | field `upload::UploadService& upload_service_;`               |
| `src/api_handler.h`   | method `UploadImage(...)`                                     |
| `src/api_handler.cpp` | constructor init `upload_service_{upload_service}`            |
| `src/api_handler.cpp` | route before `/generations`                                   |
| `src/api_handler.cpp` | implementation of `UploadImage`                               |

Route:

```cpp
if (request.method() == http::verb::post && target == "/images/upload") {
    return UploadImage(request);
}
```

Method:

```cpp
http::response<http::string_body> ApiHandler::UploadImage(
    const http::request<http::string_body>& request
) {
    try {
        auto content_type_it = request.find(http::field::content_type);

        std::string content_type = content_type_it == request.end()
            ? "application/octet-stream"
            : std::string(content_type_it->value());

        json::object result = upload_service_.SaveRawImage(
            request.body(),
            std::move(content_type)
        );

        return JsonResponse(request, std::move(result));

    } catch (const std::exception& e) {
        return JsonResponse(
            request,
            MakeError("upload_failed", e.what()),
            http::status::bad_request
        );
    }
}
```

---

## 🧩 main.cpp and CMake changes

| File             | Change                 | Code                                                                                |
| ---------------- | ---------------------- | ----------------------------------------------------------------------------------- |
| `src/main.cpp`   | include upload service | `#include "upload_service.h"`                                                       |
| `src/main.cpp`   | create service         | `upload::UploadService upload_service{fs::path{"../storage/input"}};`               |
| `src/main.cpp`   | pass to handler        | `api::ApiHandler api_handler{generation_service, catalog_service, upload_service};` |
| `CMakeLists.txt` | add source             | `src/upload_service.cpp`                                                            |
| `CMakeLists.txt` | add header             | `src/upload_service.h`                                                              |

```cpp
#include "upload_service.h"
```

```cpp
upload::UploadService upload_service{fs::path{"../storage/input"}};
```

```cpp
api::ApiHandler api_handler{
    generation_service,
    catalog_service,
    upload_service
};
```

```cmake
src/upload_service.cpp
src/upload_service.h
```

---

## ⚠️ Ошибки и решения

| Error                                       | Где              | Причина                                             | Проверка                              | Решение                                           |
| ------------------------------------------- | ---------------- | --------------------------------------------------- | ------------------------------------- | ------------------------------------------------- |
| `upload_failed: Empty upload body`          | `/images/upload` | body пустой                                         | проверить `--data-binary "@file"`     | передать реальный файл                            |
| `upload_failed: Failed to open upload file` | `/images/upload` | нет доступа к `storage/input`                       | `ls -ld storage/input`                | создать папку / проверить права                   |
| `.bin` вместо `.svg`                        | upload SVG       | `image/svg+xml` не входит в supported extensions    | посмотреть `contentType`              | добавить SVG extension или оставить `.bin`        |
| `not_found`                                 | `/images/upload` | route не добавлен в `Handle()`                      | проверить `api_handler.cpp`           | добавить route перед `/generations`               |
| compile error `UploadService not declared`  | build            | нет include                                         | проверить `api_handler.h`, `main.cpp` | добавить `#include "upload_service.h"`            |
| linker error                                | build            | `upload_service.cpp` не добавлен в CMake            | проверить `CMakeLists.txt`            | добавить cpp в target                             |
| file not visible                            | storage          | backend запущен из `build`, путь `../storage/input` | `pwd` в build                         | проверять `~/mobile-assets-backend/storage/input` |

---

## 🧾 Git

| Action       | Command                                     |
| ------------ | ------------------------------------------- |
| Check status | `git status`                                |
| Add files    | `git add .`                                 |
| Commit       | `git commit -m "Add image upload endpoint"` |
| Push         | `git push -u origin feature/image-upload`   |
| Back to main | `git checkout main`                         |

```bash
cd ~/mobile-assets-backend

git status
git add .
git commit -m "Add image upload endpoint"
git push -u origin feature/image-upload
```

---

## 🏁 Итог

| Branch                 | Done               | Android benefit                                                   | Backend benefit                                        | Next                   |
| ---------------------- | ------------------ | ----------------------------------------------------------------- | ------------------------------------------------------ | ---------------------- |
| `feature/image-upload` | ✅ Upload API готов | Android больше не передаёт `content://...` напрямую в AI pipeline | Backend хранит реальный input image и отдаёт `imageId` | `feature/task-storage` |

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
