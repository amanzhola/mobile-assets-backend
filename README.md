# 🧩 feature/template-prompt-mapping

| Branch                            | Parent                 | Назначение                                        | Главная идея                                                                               | Что исправлено перед стартом                       | Что добавлено                                                      | API                 | Назад                                                                                         |
| --------------------------------- | ---------------------- | ------------------------------------------------- | ------------------------------------------------------------------------------------------ | -------------------------------------------------- | ------------------------------------------------------------------ | ------------------- | --------------------------------------------------------------------------------------------- |
| `feature/template-prompt-mapping` | `feature/task-storage` | Подстановка prompt по `templateId` внутри backend | Android может отправлять `prompt: null`, backend сам берёт prompt из `data/templates.json` | `storage/tasks.json` убран из Git как runtime-файл | `templates_file_`, `FindTemplatePrompt()`, template prompt mapping | `POST /generations` | [Back to main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## ✅ Что сделано

| Категория       | Файл / команда                    | Было                                         | Стало                                                                  | Зачем                                           | Статус |
| --------------- | --------------------------------- | -------------------------------------------- | ---------------------------------------------------------------------- | ----------------------------------------------- | ------ |
| Runtime storage | `storage/tasks.json`              | Попал в Git                                  | Убран через `git rm --cached`                                          | Runtime-файлы не должны храниться в репозитории | ✅      |
| Branch          | `feature/template-prompt-mapping` | `feature/task-storage`                       | Новая ветка от task-storage                                            | Продолжение persistence-логики                  | ✅      |
| Service         | `src/generation_service.h`        | Конструктор принимал только `storage_file`   | Конструктор принимает `storage_file` и `templates_file`                | Service знает путь к templates                  | ✅      |
| Service         | `src/generation_service.h`        | Не было `FindTemplatePrompt()`               | Добавлен private-метод                                                 | Поиск prompt по `templateId`                    | ✅      |
| Service         | `src/generation_service.cpp`      | prompt должен был прийти от Android          | prompt может быть найден backend-ом                                    | Android может отправить `prompt: null`          | ✅      |
| Main            | `src/main.cpp`                    | `GenerationService{"../storage/tasks.json"}` | `GenerationService{"../storage/tasks.json", "../data/templates.json"}` | Подключён файл шаблонов                         | ✅      |
| Error handling  | template unknown                  | Не было отдельной ошибки                     | `unknown_template_id`                                                  | Понятная ошибка при неправильном `templateId`   | ✅      |

---

## 🧱 Структура ветки

| Root                     | src                                                          | data                  | storage                           | API                 | Build                             |
| ------------------------ | ------------------------------------------------------------ | --------------------- | --------------------------------- | ------------------- | --------------------------------- |
| `mobile-assets-backend/` | `generation_service.h`, `generation_service.cpp`, `main.cpp` | `data/templates.json` | `storage/tasks.json` runtime-only | `POST /generations` | `build/bin/mobile_assets_backend` |

---

## 🧭 Архитектура template mapping

| Client  | Request                                                          | ApiHandler              | GenerationService              | Data source           | Response               |
| ------- | ---------------------------------------------------------------- | ----------------------- | ------------------------------ | --------------------- | ---------------------- |
| Android | `serverAction: template`, `templateId: cherry`, `prompt: null`   | передаёт JSON в service | `FindTemplatePrompt("cherry")` | `data/templates.json` | task JSON уже с prompt |
| Android | `serverAction: template`, `templateId: wrong_id`, `prompt: null` | передаёт JSON в service | prompt не найден               | `data/templates.json` | `unknown_template_id`  |
| Android | `serverAction: template`, `templateId: cherry`, `prompt: custom` | передаёт JSON в service | использует переданный prompt   | templates не нужен    | task JSON              |
| Android | `serverAction: ghibli`                                           | обычная tool generation | template mapping не включается | workflows             | task JSON              |

---

## 🔥 Поведение API

| Scenario                    | Input                                                       | Backend action                      | Output                |
| --------------------------- | ----------------------------------------------------------- | ----------------------------------- | --------------------- |
| Template with prompt        | `serverAction=template`, `prompt="..."`                     | использует prompt из request        | task created          |
| Template without prompt     | `serverAction=template`, `prompt=null`, `templateId=cherry` | ищет prompt в `data/templates.json` | task created          |
| Template without templateId | `serverAction=template`, `templateId=""`                    | ошибка                              | `missing_template_id` |
| Template unknown id         | `serverAction=template`, `templateId=unknown`               | prompt не найден                    | `unknown_template_id` |
| Tool action                 | `serverAction=ghibli`                                       | template lookup не нужен            | task created          |
| Prompt action               | `serverAction=prompt`                                       | требует prompt и 1–4 images         | task created          |

---

## 🖥 Terminal 1 — fix runtime storage + branch + build + run

| Шаг | Команда                                                                               | Что делает                                    |
| --- | ------------------------------------------------------------------------------------- | --------------------------------------------- |
| 1   | `cd ~/mobile-assets-backend`                                                          | перейти в проект                              |
| 2   | `git rm --cached storage/tasks.json`                                                  | убрать runtime-файл из Git, оставить локально |
| 3   | `git commit -m "Stop tracking runtime task storage"`                                  | зафиксировать cleanup                         |
| 4   | `git push`                                                                            | отправить cleanup                             |
| 5   | `git checkout feature/task-storage`                                                   | перейти на базовую ветку                      |
| 6   | `git checkout -b feature/template-prompt-mapping`                                     | создать новую ветку                           |
| 7   | изменить `generation_service.h/.cpp`, `main.cpp`                                      | добавить mapping                              |
| 8   | `rm -rf build && mkdir build && cd build`                                             | чистая сборка                                 |
| 9   | `conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11` | зависимости                                   |
| 10  | `cmake .. -DCMAKE_BUILD_TYPE=Debug`                                                   | configure                                     |
| 11  | `cmake --build .`                                                                     | build                                         |
| 12  | `./bin/mobile_assets_backend`                                                         | run                                           |

```bash
cd ~/mobile-assets-backend

git rm --cached storage/tasks.json

git commit -m "Stop tracking runtime task storage"

git push

git checkout feature/task-storage

git checkout -b feature/template-prompt-mapping

rm -rf build
mkdir build
cd build

conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11

cmake .. -DCMAKE_BUILD_TYPE=Debug

cmake --build .

./bin/mobile_assets_backend
```

---

## 🖥 Terminal 2 — проверка template без prompt

| Проверка                   | Команда                                                  | Ожидание                                   |
| -------------------------- | -------------------------------------------------------- | ------------------------------------------ |
| create template generation | `POST /generations` с `templateId=cherry`, `prompt=null` | task created                               |
| response prompt            | посмотреть поле `prompt`                                 | prompt подставлен из `data/templates.json` |
| storage                    | `cat storage/tasks.json`                                 | task сохранён с prompt                     |
| restart                    | остановить и запустить сервер                            | task сохранился                            |
| get task                   | `GET /generations/mock_task_1`                           | task доступен после restart                |

```bash
curl -X POST http://localhost:8080/generations \
  -H "Content-Type: application/json" \
  -d '{
    "toolType": "TEMPLATE",
    "serverAction": "template",
    "sourceImageUri": "content://media/external/file/1",
    "templateId": "cherry",
    "prompt": null,
    "options": {
      "templateId": "cherry"
    },
    "outputCount": 2
  }'; echo

cat ~/mobile-assets-backend/storage/tasks.json

curl http://localhost:8080/generations/mock_task_1
```

---

## 📄 Изменения в `src/generation_service.h`

| Что найти                                                         | Чем заменить / что добавить                                                                    |
| ----------------------------------------------------------------- | ---------------------------------------------------------------------------------------------- |
| `explicit GenerationService(std::filesystem::path storage_file);` | `GenerationService(std::filesystem::path storage_file, std::filesystem::path templates_file);` |
| private methods                                                   | добавить `std::string FindTemplatePrompt(const std::string& template_id) const;`               |
| private fields                                                    | добавить `std::filesystem::path templates_file_;`                                              |

```cpp
GenerationService(std::filesystem::path storage_file,
                  std::filesystem::path templates_file);
```

```cpp
std::string FindTemplatePrompt(const std::string& template_id) const;
```

```cpp
std::filesystem::path templates_file_;
```

---

## 📄 Изменения в `src/generation_service.cpp`

| Место                                | Изменение                                         | Зачем                                |
| ------------------------------------ | ------------------------------------------------- | ------------------------------------ |
| constructor                          | добавить `templates_file_`                        | хранить путь к templates             |
| `FindTemplatePrompt()`               | новый метод                                       | найти prompt по id                   |
| `CreateGeneration()`                 | `const std::string prompt` → `std::string prompt` | prompt можно заменить                |
| после проверки `missing_template_id` | добавить lookup prompt                            | если prompt пустой, взять из backend |
| unknown id                           | вернуть `unknown_template_id`                     | понятная ошибка                      |

---

## Constructor

```cpp
GenerationService::GenerationService(std::filesystem::path storage_file,
                                     std::filesystem::path templates_file)
    : storage_file_{std::move(storage_file)}
    , templates_file_{std::move(templates_file)} {
    LoadTasks();
}
```

---

## FindTemplatePrompt

```cpp
std::string GenerationService::FindTemplatePrompt(const std::string& template_id) const {
    std::ifstream input(templates_file_);

    if (!input.is_open()) {
        throw std::runtime_error("Failed to open templates file: " + templates_file_.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    json::value parsed = json::parse(buffer.str());

    if (!parsed.is_object()) {
        return {};
    }

    const json::object& root = parsed.as_object();

    auto templates_it = root.find("templates");

    if (templates_it == root.end() || !templates_it->value().is_array()) {
        return {};
    }

    for (const auto& item : templates_it->value().as_array()) {
        if (!item.is_object()) {
            continue;
        }

        const json::object& obj = item.as_object();

        const std::string id = ReadStringOrEmpty(obj, "id");

        if (id == template_id) {
            return ReadStringOrEmpty(obj, "prompt");
        }
    }

    return {};
}
```

---

## Prompt mapping в CreateGeneration

```cpp
std::string prompt = ReadStringOrEmpty(request, "prompt");
```

```cpp
if (server_action == "template" && template_id.empty()) {
    return MakeError("missing_template_id", "template action requires templateId");
}

if (server_action == "template" && prompt.empty()) {
    prompt = FindTemplatePrompt(template_id);

    if (prompt.empty()) {
        return MakeError("unknown_template_id", "Unknown templateId: " + template_id);
    }
}
```

---

## 📄 Изменения в `src/main.cpp`

| Было                                                   | Стало                                                                                      |
| ------------------------------------------------------ | ------------------------------------------------------------------------------------------ |
| `GenerationService{fs::path{"../storage/tasks.json"}}` | `GenerationService{fs::path{"../storage/tasks.json"}, fs::path{"../data/templates.json"}}` |

```cpp
generation::GenerationService generation_service{
    fs::path{"../storage/tasks.json"},
    fs::path{"../data/templates.json"}
};
```

---

## ⚠️ Ошибки и решения

| Error                           | Где                 | Причина                                      | Решение                                                      |
| ------------------------------- | ------------------- | -------------------------------------------- | ------------------------------------------------------------ |
| `storage/tasks.json` tracked    | Git                 | runtime-файл попал в Git                     | `git rm --cached storage/tasks.json`                         |
| `Failed to open templates file` | runtime             | неправильный путь `../data/templates.json`   | запускать из `build/`, проверить `ls ../data/templates.json` |
| `unknown_template_id`           | POST `/generations` | `templateId` нет в `templates.json`          | проверить id                                                 |
| `missing_template_id`           | POST `/generations` | `serverAction=template`, но нет `templateId` | передать `templateId`                                        |
| empty prompt                    | response            | template найден, но в JSON нет `prompt`      | проверить `data/templates.json`                              |
| compile error constructor       | build               | старый вызов `GenerationService`             | обновить `main.cpp`                                          |
| parse error templates           | startup/request     | битый `templates.json`                       | проверить JSON                                               |

---

## ✅ Итог

| Возможность                                      | Статус |
| ------------------------------------------------ | ------ |
| `templateId` приходит от Android                 | ✅      |
| `prompt` может быть `null`                       | ✅      |
| backend сам ищет prompt                          | ✅      |
| prompt берётся из `data/templates.json`          | ✅      |
| unknown template даёт ошибку                     | ✅      |
| tasks продолжают сохраняться                     | ✅      |
| `storage/tasks.json` больше не обязан быть в Git | ✅      |

---

## 🧾 Git

| Action | Command                                               |
| ------ | ----------------------------------------------------- |
| status | `git status`                                          |
| add    | `git add .`                                           |
| commit | `git commit -m "Map template ids to backend prompts"` |
| push   | `git push -u origin feature/template-prompt-mapping`  |

```bash
cd ~/mobile-assets-backend

git status

git add .

git commit -m "Map template ids to backend prompts"

git push -u origin feature/template-prompt-mapping
```

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
