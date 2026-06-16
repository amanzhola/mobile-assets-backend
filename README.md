# 🖼️ feature/prompt-multi-image-comfy

| Branch                             | Parent                   | Назначение                                             | Главная идея                                                                      | Что исправлено перед стартом                      | Что добавлено                                                                                   | Pipeline                                                                   | Назад                                                                                         |
| ---------------------------------- | ------------------------ | ------------------------------------------------------ | --------------------------------------------------------------------------------- | ------------------------------------------------- | ----------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------- |
| `feature/prompt-multi-image-comfy` | `feature/comfyui-worker` | Реально обработать несколько Prompt-фото через ComfyUI | Prompt с 1/2/3/4/5 фото должен запускать ComfyUI отдельно для каждого input image | HEAD `Content-Length` для `/uploads` и `/outputs` | `ExtractUploadedFileNames()`, `RunSingleImageViaComfy()`, multi-image `RunGenerationViaComfy()` | `uploadedImageUrls[] → multiple ComfyUI prompts → multiple /outputs/*.png` | [Back to main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## ✅ Что сделано

| Категория | Файл / метод                       | Было                                                        | Стало                                          | Зачем                                              | Статус |
| --------- | ---------------------------------- | ----------------------------------------------------------- | ---------------------------------------------- | -------------------------------------------------- | ------ |
| Branch    | `feature/prompt-multi-image-comfy` | Prompt использовал только первое фото                       | Отдельная ветка multi-image Prompt             | Не ломать стабильный ComfyUI worker                | ✅      |
| HTTP      | `ServeUploadedFile()`              | HEAD мог отдавать `Content-Length: 0`                       | HEAD отдаёт реальный размер файла              | Корректнее для Android/Coil/curl                   | ✅      |
| HTTP      | `ServeOutputFile()`                | HEAD мог отдавать `Content-Length: 0`                       | HEAD отдаёт реальный размер файла              | Корректнее для output images                       | ✅      |
| Header    | `generation_service.h`             | один `RunGenerationViaComfy()` возвращал `optional<string>` | возвращает `vector<string>`                    | Можно вернуть несколько output URLs                | ✅      |
| Header    | `generation_service.h`             | `ExtractUploadedFileName()`                                 | `ExtractUploadedFileNames()`                   | Достаёт все uploaded image URLs                    | ✅      |
| Source    | `generation_service.cpp`           | только первое изображение                                   | массив input file names                        | Prompt обрабатывает все фото                       | ✅      |
| Source    | `RunSingleImageViaComfy()`         | не было отдельного метода                                   | один input → один ComfyUI prompt → один output | Переиспользуемая единица обработки                 | ✅      |
| Source    | `RunGenerationViaComfy()`          | один output                                                 | много outputs для `serverAction=prompt`        | Prompt становится multi-image                      | ✅      |
| Source    | `CreateGeneration()`               | один URL + дублирование                                     | принимает vector outputs                       | Backend response содержит несколько `/outputs/...` | ✅      |
| Android   | Prompt 2 фото                      | backend брал только первое                                  | backend запускает ComfyUI два раза             | Честный multi-image backend flow                   | ✅      |

---

## 🧱 Структура ветки

| Root                      | Source                                                                          | ComfyUI                               | Storage input   | Storage output   | API                                   |
| ------------------------- | ------------------------------------------------------------------------------- | ------------------------------------- | --------------- | ---------------- | ------------------------------------- |
| `~/mobile-assets-backend` | `src/generation_service.h`, `src/generation_service.cpp`, `src/api_handler.cpp` | `~/ComfyUI/input`, `~/ComfyUI/output` | `storage/input` | `storage/output` | `/generations`, `/outputs/{fileName}` |

---

## 🧭 Multi-image Prompt architecture

| Step | Component | Input                 | Action                       | Output                                  |
| ---- | --------- | --------------------- | ---------------------------- | --------------------------------------- |
| 1    | Android   | 1–5 images            | uploads images               | `/uploads/file1`, `/uploads/file2`, ... |
| 2    | Android   | Prompt request        | sends `uploadedImageUrls[]`  | JSON body                               |
| 3    | Backend   | `uploadedImageUrls[]` | `ExtractUploadedFileNames()` | `vector<string>`                        |
| 4    | Backend   | each file name        | `RunSingleImageViaComfy()`   | one ComfyUI prompt per image            |
| 5    | ComfyUI   | input image           | `LoadImage → SaveImage`      | output png                              |
| 6    | Backend   | ComfyUI output        | copy to `storage/output`     | `/outputs/file.png`                     |
| 7    | Response  | output URLs           | array result                 | Android Result Screen                   |

---

## 🔥 Поведение после ветки

| Scenario        | Было                                                         | Стало                                          |
| --------------- | ------------------------------------------------------------ | ---------------------------------------------- |
| Tool action     | 1 input image → 1 ComfyUI output duplicated by `outputCount` | без изменений                                  |
| Template action | 1 input image → 1 ComfyUI output duplicated by `outputCount` | без изменений                                  |
| Prompt 1 photo  | 1 input image → 1 output                                     | ✅                                              |
| Prompt 2 photos | использовалось только первое фото                            | 2 input images → 2 ComfyUI prompts → 2 outputs |
| Prompt 3 photos | использовалось только первое фото                            | 3 input images → 3 ComfyUI prompts → 3 outputs |
| Prompt 4 photos | использовалось только первое фото                            | 4 input images → 4 ComfyUI prompts → 4 outputs |
| Prompt 5 photos | использовалось только первое фото                            | 5 input images → 5 ComfyUI prompts → 5 outputs |

---

## 🖥 Terminal 1 — branch + build

| Шаг | Команда                                            | Ожидание                |
| --- | -------------------------------------------------- | ----------------------- |
| 1   | `cd ~/mobile-assets-backend`                       | backend repo            |
| 2   | `git checkout -b feature/prompt-multi-image-comfy` | новая ветка             |
| 3   | изменить `api_handler.cpp`                         | HEAD content-length fix |
| 4   | изменить `generation_service.h`                    | новые signatures        |
| 5   | изменить `generation_service.cpp`                  | multi-image logic       |
| 6   | `cd build`                                         | existing build dir      |
| 7   | `cmake --build .`                                  | build success           |

```bash
cd ~/mobile-assets-backend

git checkout -b feature/prompt-multi-image-comfy

cd build

cmake --build .
```

---

## 🖥 Terminal 2 — ComfyUI

| Шаг | Команда                                             | Ожидание            |
| --- | --------------------------------------------------- | ------------------- |
| 1   | `cd ~/ComfyUI`                                      | ComfyUI folder      |
| 2   | `source venv/bin/activate`                          | venv active         |
| 3   | `python main.py --listen 0.0.0.0 --port 8188 --cpu` | ComfyUI running     |
| 4   | Prompt 1 фото                                       | `got prompt` 1 раз  |
| 5   | Prompt 2 фото                                       | `got prompt` 2 раза |
| 6   | Prompt 3 фото                                       | `got prompt` 3 раза |

```bash
cd ~/ComfyUI

source venv/bin/activate

python main.py --listen 0.0.0.0 --port 8188 --cpu
```

---

## 🖥 Terminal 3 — Backend run

| Шаг | Команда                                              | Ожидание                              |
| --- | ---------------------------------------------------- | ------------------------------------- |
| 1   | `cd ~/mobile-assets-backend/build`                   | build folder                          |
| 2   | `export PUBLIC_BASE_URL="http://192.168.0.177:8080"` | public URL                            |
| 3   | `./bin/mobile_assets_backend`                        | backend running                       |
| 4   | Android Prompt 2 фото                                | `serverAction=prompt`, `imageCount=2` |
| 5   | Result                                               | несколько `/outputs/...png`           |

```bash
cd ~/mobile-assets-backend/build

export PUBLIC_BASE_URL="http://192.168.0.177:8080"

./bin/mobile_assets_backend
```

---

## 🖥 Terminal 4 — curl output check

| Проверка        | Команда                                         | Ожидание              |
| --------------- | ----------------------------------------------- | --------------------- |
| HEAD output     | `curl -I /outputs/file.png`                     | `Content-Length` не 0 |
| Download output | `curl -o /tmp/prompt_out.png /outputs/file.png` | файл скачан           |
| File size       | `ls -lh /tmp/prompt_out.png`                    | размер больше 0       |

```bash
curl -I http://localhost:8080/outputs/ИМЯ.png

curl -o /tmp/prompt_out.png http://localhost:8080/outputs/ИМЯ.png

ls -lh /tmp/prompt_out.png
```

---

## 📄 HEAD Content-Length fix

| Function              | Что исправить                           | Зачем                                            |
| --------------------- | --------------------------------------- | ------------------------------------------------ |
| `ServeUploadedFile()` | сохранить `body.size()` до очистки body | HEAD должен вернуть реальный размер файла        |
| `ServeOutputFile()`   | сохранить `body.size()` до очистки body | HEAD должен вернуть реальный размер output image |

```cpp
const std::size_t body_size = body.size();

http::response<http::string_body> response{http::status::ok, request.version()};

response.set(
    http::field::content_type,
    upload_service_.GetContentTypeByFileName(file_name)
);

response.set(http::field::cache_control, "no-cache");
response.set(http::field::connection, "close");
response.keep_alive(false);

if (request.method() == http::verb::head) {
    response.body() = "";
    response.content_length(body_size);
} else {
    response.body() = std::move(body);
    response.content_length(response.body().size());
}

return response;
```

Для `ServeOutputFile()` такой же блок, но content-type берётся так:

```cpp
output_service_.GetContentTypeByFileName(file_name)
```

---

## 📄 generation_service.h

| Что добавить / заменить            | Код                                |
| ---------------------------------- | ---------------------------------- |
| include                            | `#include <vector>`                |
| заменить `RunGenerationViaComfy`   | returns `std::vector<std::string>` |
| добавить `RunSingleImageViaComfy`  | one image → one output             |
| заменить `ExtractUploadedFileName` | `ExtractUploadedFileNames`         |

```cpp
#include <vector>
```

```cpp
std::vector<std::string> RunGenerationViaComfy(const json::object& request,
                                               const std::string& task_id,
                                               const std::string& server_action,
                                               int output_count);

std::optional<std::string> RunSingleImageViaComfy(const std::string& input_file_name,
                                                  const std::string& task_id,
                                                  const std::string& server_action,
                                                  int image_index);

std::vector<std::string> ExtractUploadedFileNames(const json::object& request) const;
```

---

## 📄 generation_service.cpp includes

| Include       | Зачем                            |
| ------------- | -------------------------------- |
| `<algorithm>` | `std::find` для удаления дублей  |
| `<vector>`    | список input files и output URLs |

```cpp
#include <algorithm>
#include <vector>
```

---

## 📄 ExtractFileNameFromUploadUrl

| Что делает                                 | Защита                      |
| ------------------------------------------ | --------------------------- |
| принимает `/uploads/file.png` или full URL | убирает query string        |
| ищет marker `/uploads/`                    | если нет marker → `nullopt` |
| достаёт file name                          | пустое имя запрещено        |
| проверяет `/` и `\`                        | path traversal запрещён     |

```cpp
namespace {

std::optional<std::string> ExtractFileNameFromUploadUrl(const std::string& raw_url) {
    std::string url = raw_url;

    const auto query_pos = url.find('?');

    if (query_pos != std::string::npos) {
        url = url.substr(0, query_pos);
    }

    const std::string marker = "/uploads/";
    const auto marker_pos = url.find(marker);

    if (marker_pos == std::string::npos) {
        return std::nullopt;
    }

    std::string file_name = url.substr(marker_pos + marker.size());

    if (file_name.empty()) {
        return std::nullopt;
    }

    if (file_name.find('/') != std::string::npos || file_name.find('\\') != std::string::npos) {
        return std::nullopt;
    }

    return file_name;
}

}  // namespace
```

---

## 📄 ExtractUploadedFileNames

| Source field        | Type           | Что делает              |
| ------------------- | -------------- | ----------------------- |
| `sourceImageUrl`    | string         | добавляет одиночный URL |
| `uploadedImageUrls` | array          | добавляет все URL       |
| duplicates          | same filename  | убирает дубли           |
| invalid URL         | no `/uploads/` | пропускает              |

```cpp
std::vector<std::string> GenerationService::ExtractUploadedFileNames(
    const json::object& request
) const {
    std::vector<std::string> result;

    auto single_it = request.find("sourceImageUrl");

    if (single_it != request.end() && single_it->value().is_string()) {
        auto file_name = ExtractFileNameFromUploadUrl(
            std::string(single_it->value().as_string())
        );

        if (file_name) {
            result.push_back(*file_name);
        }
    }

    auto uploaded_it = request.find("uploadedImageUrls");

    if (uploaded_it != request.end() && uploaded_it->value().is_array()) {
        for (const auto& item : uploaded_it->value().as_array()) {
            if (!item.is_string()) {
                continue;
            }

            auto file_name = ExtractFileNameFromUploadUrl(
                std::string(item.as_string())
            );

            if (!file_name) {
                continue;
            }

            const bool already_exists = std::find(
                result.begin(),
                result.end(),
                *file_name
            ) != result.end();

            if (!already_exists) {
                result.push_back(*file_name);
            }
        }
    }

    return result;
}
```

---

## 📄 RunSingleImageViaComfy

| Step | Action                                          |
| ---- | ----------------------------------------------- |
| 1    | create `comfy_input_dir_`                       |
| 2    | find input in backend `storage/input`           |
| 3    | copy input to `~/ComfyUI/input`                 |
| 4    | build unique output prefix with `image_index`   |
| 5    | build workflow                                  |
| 6    | queue prompt                                    |
| 7    | wait first output                               |
| 8    | copy ComfyUI output to backend `storage/output` |
| 9    | return public `/outputs/...` URL                |

```cpp
std::optional<std::string> GenerationService::RunSingleImageViaComfy(
    const std::string& input_file_name,
    const std::string& task_id,
    const std::string& server_action,
    int image_index
) {
    try {
        fs::create_directories(comfy_input_dir_);

        const fs::path backend_input_file = backend_input_dir_ / input_file_name;
        const fs::path comfy_input_file = comfy_input_dir_ / input_file_name;

        if (!fs::exists(backend_input_file)) {
            return std::nullopt;
        }

        fs::copy_file(
            backend_input_file,
            comfy_input_file,
            fs::copy_options::overwrite_existing
        );

        const std::string output_prefix =
            "pixo_" + server_action + "_" + task_id + "_" + std::to_string(image_index);

        json::object workflow = workflow_builder_.BuildWorkflow(
            server_action,
            input_file_name,
            output_prefix
        );

        auto prompt_id = comfy_client_.QueuePrompt(workflow);

        if (!prompt_id) {
            return std::nullopt;
        }

        auto comfy_output_file_name = comfy_client_.WaitForFirstOutputFile(*prompt_id);

        if (!comfy_output_file_name) {
            return std::nullopt;
        }

        const fs::path comfy_output_file = comfy_output_dir_ / *comfy_output_file_name;
        const fs::path saved_output_file = output_service_.SaveFromComfyOutput(comfy_output_file);

        return output_service_.GetPublicUrl(
            saved_output_file.filename().string()
        );

    } catch (...) {
        return std::nullopt;
    }
}
```

---

## 📄 RunGenerationViaComfy

| serverAction   | Input handling            | Output handling                        |
| -------------- | ------------------------- | -------------------------------------- |
| `prompt`       | all uploaded images       | one output per input                   |
| other actions  | first uploaded image only | one output duplicated by `outputCount` |
| no input files | returns empty vector      | fallback in CreateGeneration           |
| Comfy failure  | skip failed output        | fallback if no outputs                 |

```cpp
std::vector<std::string> GenerationService::RunGenerationViaComfy(
    const json::object& request,
    const std::string& task_id,
    const std::string& server_action,
    int output_count
) {
    std::vector<std::string> result_urls;

    const std::vector<std::string> input_file_names = ExtractUploadedFileNames(request);

    if (input_file_names.empty()) {
        return result_urls;
    }

    if (server_action == "prompt") {
        int image_index = 0;

        for (const std::string& input_file_name : input_file_names) {
            auto output_url = RunSingleImageViaComfy(
                input_file_name,
                task_id,
                server_action,
                image_index
            );

            if (output_url) {
                result_urls.push_back(*output_url);
            }

            ++image_index;
        }

        return result_urls;
    }

    auto output_url = RunSingleImageViaComfy(
        input_file_names.front(),
        task_id,
        server_action,
        0
    );

    if (output_url) {
        result_urls.push_back(*output_url);
    }

    while (!result_urls.empty() && static_cast<int>(result_urls.size()) < output_count) {
        result_urls.push_back(result_urls.front());
    }

    return result_urls;
}
```

---

## 📄 CreateGeneration result logic

| Было                     | Стало                     |
| ------------------------ | ------------------------- |
| один `optional<string>`  | `vector<string>`          |
| один output URL          | много output URLs         |
| prompt 2 фото → 1 output | prompt 2 фото → 2 outputs |
| fallback на input image  | fallback сохранён         |

```cpp
json::array result_urls;

std::vector<std::string> comfy_result_urls = RunGenerationViaComfy(
    request,
    task_id,
    server_action,
    output_count
);

if (!comfy_result_urls.empty()) {
    for (const std::string& url : comfy_result_urls) {
        result_urls.emplace_back(url);
    }
} else {
    result_urls.emplace_back(first_input_image_url);
}

while (static_cast<int>(result_urls.size()) < output_count) {
    result_urls.emplace_back(result_urls.front());
}
```

---

## 📱 Android test matrix

| Test             | Expected ComfyUI log | Expected backend result              |
| ---------------- | -------------------- | ------------------------------------ |
| Prompt 1 photo   | `got prompt` 1 раз   | 1 `/outputs/*.png`                   |
| Prompt 2 photos  | `got prompt` 2 раза  | 2 `/outputs/*.png`                   |
| Prompt 3 photos  | `got prompt` 3 раза  | 3 `/outputs/*.png`                   |
| Tool AI Enhancer | `got prompt` 1 раз   | 1 output duplicated by `outputCount` |
| Template cherry  | `got prompt` 1 раз   | 1 output duplicated by `outputCount` |
| Output HEAD      | no ComfyUI needed    | real `Content-Length`                |
| Output GET       | no ComfyUI needed    | image file downloads                 |

---

## ⚠️ Ошибки и решения

| Error                             | Где                       | Причина                                 | Решение                         |
| --------------------------------- | ------------------------- | --------------------------------------- | ------------------------------- |
| Prompt 2 фото даёт 1 output       | `CreateGeneration`        | остался старый `optional<string>` logic | заменить на `vector<string>`    |
| `std::find` compile error         | build                     | нет `<algorithm>`                       | добавить include                |
| `vector` compile error            | build                     | нет `<vector>`                          | добавить include                |
| output перезаписывается           | ComfyUI output            | одинаковый prefix                       | добавить `image_index` в prefix |
| `ExtractUploadedFileNames` пустой | request                   | нет `/uploads/` в URL                   | проверить Android request       |
| fallback на input image           | generation                | ComfyUI не смог                         | смотреть backend/ComfyUI logs   |
| HEAD Content-Length 0             | `/outputs` или `/uploads` | body cleared before size saved          | использовать `body_size`        |
| Prompt 5 фото медленно            | CPU ComfyUI               | 5 prompt подряд                         | нормально для CPU               |

---

## 🧾 Git

| Action | Command                                                          |
| ------ | ---------------------------------------------------------------- |
| status | `git status`                                                     |
| add    | `git add .`                                                      |
| commit | `git commit -m "Process multiple prompt images through ComfyUI"` |
| push   | `git push -u origin feature/prompt-multi-image-comfy`            |

```bash
cd ~/mobile-assets-backend

git status

git add .

git commit -m "Process multiple prompt images through ComfyUI"

git push -u origin feature/prompt-multi-image-comfy
```

---

## 🏁 Итог

| Возможность                                           | Статус |
| ----------------------------------------------------- | ------ |
| Prompt 1 image → 1 ComfyUI output                     | ✅      |
| Prompt 2 images → 2 ComfyUI outputs                   | ✅      |
| Prompt 3 images → 3 ComfyUI outputs                   | ✅      |
| Prompt 4 images → 4 ComfyUI outputs                   | ✅      |
| Prompt 5 images → 5 ComfyUI outputs                   | ✅      |
| Tools still use first image                           | ✅      |
| Templates still use first image                       | ✅      |
| HEAD content-length fixed                             | ✅      |
| Backend response now can contain multiple output URLs | ✅      |
| Real AI model still next stage                        | ⏳      |

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
