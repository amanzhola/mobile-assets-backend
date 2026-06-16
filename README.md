# 🧪 feature/local-mock-results

| Branch                       | Parent                  | Назначение                                         | Главная идея                                             | Что добавлено                                                                       | Главный backend fix                         | Android result                          | Назад                                                                                         |
| ---------------------------- | ----------------------- | -------------------------------------------------- | -------------------------------------------------------- | ----------------------------------------------------------------------------------- | ------------------------------------------- | --------------------------------------- | --------------------------------------------------------------------------------------------- |
| `feature/local-mock-results` | `feature/serve-uploads` | Вернуть реальную uploaded-картинку как mock-result | Android должен видеть настоящий Result Screen до ComfyUI | `sourceImageUrl`, `uploadedImageUrls`, local resultImageUrls, стабильный HTTP close | `1 socket = 1 request = 1 response = close` | Result Screen показывает uploaded image | [Back to main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## ✅ Что сделано

| Категория          | Было                               | Стало                                         | Почему важно                                  | Статус |
| ------------------ | ---------------------------------- | --------------------------------------------- | --------------------------------------------- | ------ |
| Mock result        | `https://mock.pixo.ai/...`         | `/uploads/{fileName}` или полный backend URL  | Android реально загружает картинку            | ✅      |
| Upload flow        | upload был отдельно                | uploaded image используется как result        | Можно проверить UI без ComfyUI                | ✅      |
| Generation request | `sourceImageUri`                   | `sourceImageUrl` / `uploadedImageUrls`        | Backend получает URL сохранённого файла       | ✅      |
| GenerationService  | всегда делал mock external URL     | если есть `/uploads/...`, возвращает его      | Result Screen открывает backend image         | ✅      |
| HTTP server        | держал keep-alive socket           | закрывает соединение после каждого ответа     | OkHttp/Coil больше не зависают                | ✅      |
| JsonResponse       | мог давать нестабильный ответ      | `Connection: close` + точный `Content-Length` | Retrofit не падает `unexpected end of stream` | ✅      |
| ServeUploadedFile  | `prepare_payload()`                | явный `content_length()`                      | стабильная отдача картинки                    | ✅      |
| Android emulator   | мог использовать неправильный IP   | `10.0.2.2:8080`                               | Эмулятор видит backend                        | ✅      |
| Real phone         | использует LAN IP                  | `192.168.0.177:8080`                          | Телефон видит backend в Wi-Fi                 | ✅      |
| Template           | мог отправлять неверный id         | отправляет `template.templateId`              | Template работает как Tools/Prompt            | ✅      |
| Error handling     | Retrofit exception мог крашить app | `runCatching` → `GenerationStatus.Error`      | App не падает без backend                     | ✅      |

---

## 🧱 Архитектура текущего mock-flow

| Step | Android              | Backend endpoint                   | Backend action              | Response          | Result                |
| ---- | -------------------- | ---------------------------------- | --------------------------- | ----------------- | --------------------- |
| 1    | выбирает фото        | —                                  | сжимает в WebP/JPEG         | bytes             | готово к upload       |
| 2    | отправляет фото      | `POST /images/upload`              | сохраняет в `storage/input` | `imageUrl`        | есть URL файла        |
| 3    | создаёт генерацию    | `POST /generations`                | берёт `sourceImageUrl`      | task JSON         | task created          |
| 4    | backend делает mock  | `GenerationService`                | result = uploaded image     | `resultImageUrls` | локальный mock-result |
| 5    | Android ждёт status  | `GET /generations/{taskId}`        | возвращает completed        | task status       | success               |
| 6    | Android берёт result | `GET /generations/{taskId}/result` | возвращает URL              | result JSON       | Result Screen         |
| 7    | Coil грузит картинку | `GET /uploads/{fileName}`          | отдаёт binary               | image body        | картинка видна        |

---

## 🔥 Endpoints после ветки

| Method | Endpoint                           | Input                   | Output                            | Для чего                       | Статус |
| ------ | ---------------------------------- | ----------------------- | --------------------------------- | ------------------------------ | ------ |
| `GET`  | `/health`                          | none                    | health JSON                       | проверить backend              | ✅      |
| `GET`  | `/tools`                           | none                    | tools JSON                        | Android catalog                | ✅      |
| `GET`  | `/templates`                       | none                    | templates JSON                    | Android templates              | ✅      |
| `POST` | `/images/upload`                   | WebP/JPEG/PNG raw bytes | `imageId`, `fileName`, `imageUrl` | загрузить input image          | ✅      |
| `GET`  | `/uploads/{fileName}`              | file name               | binary image                      | показать uploaded/result image | ✅      |
| `POST` | `/generations`                     | JSON с `sourceImageUrl` | task JSON                         | создать mock generation        | ✅      |
| `GET`  | `/generations/{taskId}`            | task id                 | task status                       | polling                        | ✅      |
| `GET`  | `/generations/{taskId}/result`     | task id                 | `resultImageUrls`                 | Result Screen                  | ✅      |
| `POST` | `/generations/{taskId}/regenerate` | task id                 | regenerated task                  | повтор                         | ✅      |

---

## 📦 Форматы изображений

| Use case                   | Лучший формат                 | Запасной формат            | Не использовать для фото     | Причина                         |
| -------------------------- | ----------------------------- | -------------------------- | ---------------------------- | ------------------------------- |
| Android input camera photo | `image/webp` quality `80–90%` | `image/jpeg` quality `85%` | `image/png`, `image/svg+xml` | WebP/JPEG меньше по размеру     |
| AI result                  | `image/webp`                  | `image/jpeg`               | SVG                          | result должен быть raster image |
| Dev test                   | SVG можно                     | favicon/cube               | —                            | только для проверки endpoint    |
| Upload API                 | WebP/JPEG/PNG/SVG/BIN         | any binary                 | —                            | backend сохраняет raw body      |

---

## 🖥 Terminal 1 — branch + build + run

| Шаг | Команда                                                                               | Что делает                 |
| --- | ------------------------------------------------------------------------------------- | -------------------------- |
| 1   | `cd ~/mobile-assets-backend`                                                          | перейти в проект           |
| 2   | `git checkout feature/serve-uploads`                                                  | базовая ветка              |
| 3   | `git checkout -b feature/local-mock-results`                                          | новая ветка                |
| 4   | изменить `src/generation_service.cpp`                                                 | local mock result          |
| 5   | изменить `src/http_server.cpp`                                                        | close connection fix       |
| 6   | изменить `src/api_handler.cpp`                                                        | stable JSON/file responses |
| 7   | `rm -rf build && mkdir build && cd build`                                             | чистая сборка              |
| 8   | `conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11` | зависимости                |
| 9   | `cmake .. -DCMAKE_BUILD_TYPE=Debug`                                                   | configure                  |
| 10  | `cmake --build .`                                                                     | build                      |
| 11  | `./bin/mobile_assets_backend`                                                         | run                        |

```bash
cd ~/mobile-assets-backend

git checkout feature/serve-uploads
git checkout -b feature/local-mock-results

rm -rf build
mkdir build
cd build

conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11

cmake .. -DCMAKE_BUILD_TYPE=Debug

cmake --build .

./bin/mobile_assets_backend
```

---

## 🖥 Terminal 2 — upload + local mock result test

| Проверка          | Команда                                | Ожидание                                |
| ----------------- | -------------------------------------- | --------------------------------------- |
| upload image      | `POST /images/upload`                  | JSON с `imageUrl`                       |
| extract url       | `jq -r '.imageUrl'`                    | `/uploads/...`                          |
| create generation | `POST /generations` с `sourceImageUrl` | `resultImageUrls` содержит uploaded URL |
| get task          | `GET /generations/mock_task_1`         | completed task                          |
| get result        | `GET /generations/mock_task_1/result`  | result URLs                             |
| open file         | `GET /uploads/{fileName}`              | image body                              |

```bash
UPLOAD_RESPONSE=$(curl -s -X POST http://localhost:8080/images/upload \
  -H "Content-Type: image/jpeg" \
  --data-binary "@/home/ubuntu/cppbackend/sprint4/problems/leave_game/precode/static/favicon.ico")

echo "$UPLOAD_RESPONSE"

sudo apt install -y jq

IMAGE_URL=$(echo "$UPLOAD_RESPONSE" | jq -r '.imageUrl')

echo "$IMAGE_URL"

curl -X POST http://localhost:8080/generations \
  -H "Content-Type: application/json" \
  -d "{
    \"toolType\": \"GHIBLI\",
    \"serverAction\": \"ghibli\",
    \"sourceImageUrl\": \"$IMAGE_URL\",
    \"sourceImageUri\": \"content://media/external/file/1\",
    \"prompt\": \"Transform image into anime inspired look\",
    \"options\": {},
    \"outputCount\": 2
  }"; echo

curl http://localhost:8080/generations/mock_task_1; echo

curl http://localhost:8080/generations/mock_task_1/result; echo
```

---

## 📄 generation_service.cpp — local mock result

| Место                | Что добавить / заменить                        | Зачем                                                 |
| -------------------- | ---------------------------------------------- | ----------------------------------------------------- |
| anonymous namespace  | `ReadFirstInputImageUrl()`                     | взять `sourceImageUrl` или первый `uploadedImageUrls` |
| `CreateGeneration()` | `first_input_image_url`                        | сохранить первый input URL                            |
| result loop          | если URL начинается с `/uploads/`, вернуть его | Android видит реальную картинку                       |
| fallback             | `MakeMockResultUrl(...)`                       | если input URL нет, оставить старый mock              |

```cpp
std::string ReadFirstInputImageUrl(const json::object& request) {
    std::string source_image_url = ReadStringOrEmpty(request, "sourceImageUrl");

    if (!source_image_url.empty()) {
        return source_image_url;
    }

    auto uploaded_urls = ReadStringArray(request, "uploadedImageUrls");

    if (!uploaded_urls.empty()) {
        return uploaded_urls.front();
    }

    return {};
}
```

```cpp
const std::string first_input_image_url = ReadFirstInputImageUrl(request);
```

```cpp
for (int i = 1; i <= output_count; ++i) {
    if (!first_input_image_url.empty() && first_input_image_url.starts_with("/uploads/")) {
        task.result_image_urls.push_back(first_input_image_url);
    } else {
        task.result_image_urls.push_back(MakeMockResultUrl(server_action, task_id, i));
    }
}
```

---

## 🧯 Главный backend bugfix — blocking keep-alive

| Было                                                            | Почему ломалось                                  | Стало                                       | Почему работает                             |
| --------------------------------------------------------------- | ------------------------------------------------ | ------------------------------------------- | ------------------------------------------- |
| `for (;;) { http::read(socket,...); http::write(socket,...); }` | сервер ждал следующий request от того же клиента | `1 socket = 1 request = 1 response = close` | сервер не зависает на keep-alive            |
| Android/OkHttp держал socket                                    | backend блокировался в `http::read()`            | `Connection: close`                         | клиент понимает, что соединение закрывается |
| Coil грузил `/uploads/...`                                      | сервер был занят старым соединением              | socket закрывается после write              | новые запросы принимаются                   |
| result image timeout                                            | сервер не принимал новое соединение              | стабильный close                            | Result Screen работает                      |

---

## 📄 http_server.cpp — stable one-request session

| Часть                                    | Что важно                  |
| ---------------------------------------- | -------------------------- |
| `request_parser`                         | body limit для upload      |
| `handler_.Handle(request)`               | получить response          |
| `connection: close`                      | запрет keep-alive          |
| `keep_alive(false)`                      | явно выключить keep-alive  |
| `content_length(response.body().size())` | точный размер body         |
| `http::write(socket, response)`          | отправить response         |
| `shutdown_send`                          | закрыть отправку аккуратно |

```cpp
void HttpServer::HandleSession(tcp::socket socket) {
    beast::flat_buffer buffer;

    try {
        http::request_parser<http::string_body> parser;
        parser.body_limit(25 * 1024 * 1024);

        http::read(socket, buffer, parser);

        auto request = parser.release();
        auto response = handler_.Handle(request);

        response.set(http::field::connection, "close");
        response.keep_alive(false);
        response.content_length(response.body().size());

        http::write(socket, response);

    } catch (const beast::system_error& e) {
        if (e.code() != http::error::end_of_stream) {
            std::cerr << "session error: " << e.what() << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "session error: " << e.what() << std::endl;
    }

    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
}
```

---

## 📄 api_handler.cpp — stable JsonResponse and files

| Response type | Что выставить                    | Почему                         |
| ------------- | -------------------------------- | ------------------------------ |
| JSON          | `Content-Type: application/json` | Retrofit ждёт JSON             |
| JSON          | `Cache-Control: no-cache`        | не кэшировать mock API         |
| JSON          | `Connection: close`              | не держать socket              |
| JSON          | `content_length(body.size())`    | OkHttp читает ровно body size  |
| File          | content type по расширению       | Coil понимает image            |
| File          | `Connection: close`              | не держать socket              |
| File          | `content_length(body.size())`    | нет `unexpected end of stream` |

```cpp
http::response<http::string_body> ApiHandler::JsonResponse(
    const http::request<http::string_body>& request,
    json::value body,
    http::status status
) const {
    http::response<http::string_body> response{status, request.version()};

    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.set(http::field::connection, "close");

    response.keep_alive(false);

    response.body() = json::serialize(body);
    response.content_length(response.body().size());

    return response;
}
```

For uploaded files:

```cpp
response.set(http::field::cache_control, "no-cache");
response.set(http::field::connection, "close");
response.keep_alive(false);

response.body() = std::move(body);
response.content_length(response.body().size());

return response;
```

---

## 🤖 Android addresses

| Environment        | Backend base URL                   | Почему                                              |
| ------------------ | ---------------------------------- | --------------------------------------------------- |
| Android Emulator   | `http://10.0.2.2:8080/`            | emulator обращается к host machine через `10.0.2.2` |
| Real phone         | `http://192.168.0.177:8080/`       | телефон идёт по Wi-Fi LAN IP                        |
| WSL/curl           | `http://localhost:8080/`           | локальная проверка                                  |
| Browser phone test | `http://192.168.0.177:8080/health` | проверить доступность backend                       |

```kotlin
private fun pixoBackendBaseUrl(): String {
    return if (isAndroidEmulator()) {
        "http://10.0.2.2:8080/"
    } else {
        "http://192.168.0.177:8080/"
    }
}
```

---

## 🧩 Template fix

| Было                                             | Стало                                               | Почему                               |
| ------------------------------------------------ | --------------------------------------------------- | ------------------------------------ |
| template мог отправлять индекс / неправильный id | `templateId = template.templateId`                  | backend ищет prompt по настоящему id |
| options могли не совпадать                       | `options = mapOf("templateId" to serverTemplateId)` | единый id в request                  |
| sourceImageUri мог быть невалидный               | валидный URI / uploaded URL                         | generation request корректен         |
| emulator URL мог отличаться                      | общий `backendBaseUrl`                              | Template работает как Tools/Prompt   |

```kotlin
val serverTemplateId = template.templateId

templateId = serverTemplateId

options = mapOf("templateId" to serverTemplateId)
```

---

## 🧼 Android cleanup

| Где           | Убрать                                                                     | Оставить                     |
| ------------- | -------------------------------------------------------------------------- | ---------------------------- |
| `PixoApp.kt`  | временный global Coil loader                                               | обычную app config           |
| Result screen | debug Text, `Log.d`, `Log.e`, errorText, `rememberAsyncImagePainter` debug | чистый `AsyncImage`          |
| Repository    | hardcode `local_test_task`                                                 | настоящий backend taskId     |
| Repository    | hardcode image URL                                                         | URL из backend response      |
| Repository    | raw Retrofit exceptions                                                    | `runCatching` → domain error |

Clean result image:

```kotlin
@Composable
private fun PixoPromptFlowResultImage(
    resultImageUrl: String?,
    modifier: Modifier = Modifier
) {
    Box(
        modifier = modifier
            .clip(RoundedCornerShape(dimensionResource(R.dimen._16)))
            .background(BgSurface400),
        contentAlignment = Alignment.Center
    ) {
        if (resultImageUrl.isNullOrBlank()) {
            PixoResultPlaceholder(url = resultImageUrl)
        } else {
            AsyncImage(
                model = resultImageUrl,
                contentDescription = null,
                modifier = Modifier.fillMaxSize(),
                contentScale = ContentScale.Crop
            )
        }
    }
}
```

---

## 🧱 Android architecture cleanup

| Layer             | Path                                             | Responsibility                             |
| ----------------- | ------------------------------------------------ | ------------------------------------------ |
| Domain model      | `domain/model/GenerationCreateRequest.kt`        | чистая request-модель без Retrofit         |
| Domain repository | `domain/repository/GenerationRepository.kt`      | интерфейс генерации                        |
| Remote API        | `data/remote/PixoBackendApi.kt`                  | Retrofit endpoints                         |
| DTO               | `data/remote/dto/PixoBackendDtos.kt`             | backend DTO                                |
| Mapper            | `data/mapper/GenerationMappers.kt`               | DTO ↔ domain                               |
| Image             | `data/image/ImageCompressor.kt`                  | интерфейс сжатия                           |
| Image Android     | `data/image/AndroidImageCompressor.kt`           | реальное сжатие WebP/JPEG                  |
| URL               | `data/url/BackendUrlResolver.kt`                 | base URL + absolute URL                    |
| Repository impl   | `data/repository/BackendGenerationRepository.kt` | связывает API, mapper, image, url, history |

```text
data/
├── image/
│   ├── ImageCompressor.kt
│   └── AndroidImageCompressor.kt
├── mapper/
│   └── GenerationMappers.kt
├── remote/
│   ├── PixoBackendApi.kt
│   └── dto/
│       └── PixoBackendDtos.kt
├── repository/
│   └── BackendGenerationRepository.kt
└── url/
    └── BackendUrlResolver.kt
```

---

## 🛡️ Backend unavailable не должен крашить app

| Метод                 | Раньше                                       | Теперь                                            |
| --------------------- | -------------------------------------------- | ------------------------------------------------- |
| `createGeneration()`  | Retrofit exception мог упасть в UI coroutine | `runCatching` возвращает `GenerationStatus.Error` |
| `observeGeneration()` | polling exception мог крашить app            | flow emits `GenerationStatus.Error`               |
| `getResult()`         | exception мог крашить Result Screen          | возвращает controlled error                       |
| UI                    | crash                                        | показывает ошибку                                 |
| Domain                | exception                                    | error state                                       |

```kotlin
runCatching {
    api.createGeneration(dto)
}.getOrElse { error ->
    GenerationCreateResult(
        taskId = "failed_${System.currentTimeMillis()}",
        status = GenerationStatus.Error(
            taskId = "failed_${System.currentTimeMillis()}",
            message = error.message ?: "Backend is unavailable"
        )
    )
}
```

---

## ⚠️ Ошибки и решения

| Error                              | Где                   | Реальная причина                          | Fix                                                     |
| ---------------------------------- | --------------------- | ----------------------------------------- | ------------------------------------------------------- |
| Coil timeout                       | Result Screen         | backend держал keep-alive socket          | one request → close                                     |
| `unexpected end of stream`         | Retrofit              | неверный close / Content-Length           | explicit `Connection: close` + `content_length`         |
| `/generations/local_test_task` 404 | Android test          | hardcoded taskId не существует на backend | использовать настоящий taskId или mock flow без backend |
| Template падает на emulator        | Android Template flow | неправильный base URL / templateId        | `10.0.2.2`, настоящий `template.templateId`             |
| backend off crash                  | Android               | exception не пойман                       | `runCatching` to `GenerationStatus.Error`               |
| `/uploads/...` 404                 | backend               | fileName не существует                    | проверить upload response и storage                     |
| Broken pipe                        | backend logs          | клиент закрыл соединение                  | нормально для dev, close handling стабилизирован        |
| `storage/` in git                  | Git                   | runtime files staged                      | `.gitignore`, `git reset storage`                       |

---

## 🧪 Android test checklist

| Test                | Emulator               | Real phone                  | Expected                  |
| ------------------- | ---------------------- | --------------------------- | ------------------------- |
| Health              | `10.0.2.2:8080/health` | `192.168.0.177:8080/health` | OK                        |
| Tools               | ✅                      | ✅                           | catalog loads             |
| Templates           | ✅                      | ✅                           | 24 templates load         |
| Prompt 1 image      | ✅                      | ✅                           | result image shown        |
| Prompt 2–5 images   | ✅                      | ✅                           | generation works          |
| Template generation | ✅                      | ✅                           | correct templateId        |
| Backend off         | ✅                      | ✅                           | app shows error, no crash |
| Result image        | ✅                      | ✅                           | Coil loads backend URL    |

---

## 🚀 Следующий план

| Step | Что сделать                | Почему                                |
| ---- | -------------------------- | ------------------------------------- |
| 1    | Commit рабочего local mock | зафиксировать стабильную цепочку      |
| 2    | Android integration test   | убедиться, что UI работает            |
| 3    | `storage/output/`          | место для AI results                  |
| 4    | ComfyUI adapter            | подключить real generation            |
| 5    | async task status          | `queued/running/completed/failed`     |
| 6    | real result WebP           | Android получит настоящую AI-картинку |
| 7    | multipart endpoint later   | не ломать текущий рабочий API         |

---

## 🧾 Git

| Action          | Command                                                                      |
| --------------- | ---------------------------------------------------------------------------- |
| status          | `git status`                                                                 |
| add backend fix | `git add src/generation_service.cpp src/http_server.cpp src/api_handler.cpp` |
| commit          | `git commit -m "Return uploaded image URLs for local mock results"`          |
| push            | `git push -u origin feature/local-mock-results`                              |

```bash
cd ~/mobile-assets-backend

git status

git add src/generation_service.cpp src/http_server.cpp src/api_handler.cpp

git commit -m "Return uploaded image URLs for local mock results"

git push -u origin feature/local-mock-results
```

---

## 🏁 Handover

| Area                               | Status    |
| ---------------------------------- | --------- |
| `POST /images/upload`              | ✅         |
| `GET /uploads/{fileName}`          | ✅         |
| `POST /generations`                | ✅         |
| `GET /generations/{taskId}`        | ✅         |
| `GET /generations/{taskId}/result` | ✅         |
| Local resultImageUrls              | ✅         |
| Android Result Screen              | ✅         |
| Real phone                         | ✅         |
| Emulator                           | ✅         |
| Backend unavailable no-crash       | ✅         |
| ComfyUI                            | next step |

```text
PIXO backend now supports:
- POST /images/upload
- POST /generations
- GET /generations/{taskId}
- GET /generations/{taskId}/result
- GET /uploads/{fileName}

The critical bug was in the synchronous HTTP server.
It kept client sockets open and waited for more requests from the same connection.
Because the server is blocking and single-threaded, one keep-alive connection blocked all other requests.
Android Coil timed out while loading uploaded images.

Fix:
One request per connection.
Each response forces Connection: close.
Socket is shutdown after write.

Result:
Android uploads image → backend saves it → backend returns URL → generation returns uploaded URL as local mock result → Result Screen loads image from backend successfully.
```

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
