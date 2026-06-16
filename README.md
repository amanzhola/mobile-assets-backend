# ✨ feature/real-ai-enhancer-upscale

| Branch | Parent | Model | Workflow | Async | Android | Result | Next | Back |
|----------|----------|----------|----------|----------|----------|----------|----------|----------|
| `feature/real-ai-enhancer-upscale` | `feature/prompt-multi-image-comfy` | `RealESRGAN_x2plus.pth` | Real ComfyUI AI Enhance | ✅ | No changes | `/outputs/...png` | `upscale_image.json` | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## ✅ Что сделано

| Блок            | Было                                             | Стало                                                                | Причина                                      | Статус |
| --------------- | ------------------------------------------------ | -------------------------------------------------------------------- | -------------------------------------------- | ------ |
| AI Enhancer     | `LoadImage → SaveImage` copy workflow            | `LoadImage → UpscaleModelLoader → ImageUpscaleWithModel → SaveImage` | Настоящий AI upscale/restoration             | ✅      |
| Model           | Не было модели                                   | `RealESRGAN_x2plus.pth`                                              | Первый лёгкий real AI workflow               | ✅      |
| ComfyUI         | Просто копировал input в output                  | Выполняет upscale model                                              | Проверка реального AI pipeline               | ✅      |
| Android         | Уже отправлял image + `serverAction=ai_enhancer` | Не меняется                                                          | Backend сам выбирает workflow                | ✅      |
| Backend sync    | `POST /generations` мог ждать долго              | Async generation                                                     | Android не ловит timeout                     | ✅      |
| Task status     | Сразу completed / ожидание внутри request        | Сначала `processing`, потом `completed`                              | Правильная модель долгой генерации           | ✅      |
| Storage         | `/outputs/...` уже есть                          | AI output сохраняется туда же                                        | Android Result Screen работает без изменений | ✅      |
| Other workflows | copy workflows                                   | остаются copy workflows                                              | Не ломаем Ghibli/Templates/Prompt            | ✅      |

---

## 🧱 Структура этапа

| Component          | Path                                                    | Назначение       | Изменение                        |
| ------------------ | ------------------------------------------------------- | ---------------- | -------------------------------- |
| Backend repo       | `~/mobile-assets-backend`                               | C++ backend      | новая ветка                      |
| ComfyUI            | `~/ComfyUI`                                             | AI worker        | модель в `models/upscale_models` |
| Model              | `~/ComfyUI/models/upscale_models/RealESRGAN_x2plus.pth` | upscale model    | скачать                          |
| Workflow           | `workflows/ai_enhancer.json`                            | ComfyUI workflow | заменить copy на RealESRGAN      |
| Generation service | `src/generation_service.h/.cpp`                         | task lifecycle   | async generation                 |
| Output             | `storage/output`                                        | generated images | real AI result                   |
| Android            | mobile app                                              | client           | без изменений                    |

---

## 🔁 Новый lifecycle генерации

| Step                                 | До async                            | После async                             |
| ------------------------------------ | ----------------------------------- | --------------------------------------- |
| Android вызывает `POST /generations` | Ждёт весь ComfyUI процесс           | Получает быстрый ответ                  |
| Backend создаёт task                 | Task мог завершаться внутри request | Task создаётся со статусом `processing` |
| ComfyUI                              | Выполнялся синхронно                | Выполняется в background thread         |
| Android polling                      | Не был критичен                     | Основной способ дождаться результата    |
| `GET /generations/{taskId}`          | Возвращал completed                 | Сначала `processing`, потом `completed` |
| `GET /generations/{taskId}/result`   | Result после генерации              | Result после background completion      |

---

## 🖥 Terminal 1 — branch + model

| Шаг | Команда                                                        | Результат         |
| --- | -------------------------------------------------------------- | ----------------- |
| 1   | `cd ~/mobile-assets-backend`                                   | перейти в backend |
| 2   | `git checkout feature/prompt-multi-image-comfy`                | базовая ветка     |
| 3   | `git pull`                                                     | обновиться        |
| 4   | `git checkout -b feature/real-ai-enhancer-upscale`             | новая ветка       |
| 5   | `mkdir -p ~/ComfyUI/models/upscale_models`                     | папка моделей     |
| 6   | `wget -O RealESRGAN_x2plus.pth ...`                            | скачать модель    |
| 7   | `ls -lh ~/ComfyUI/models/upscale_models/RealESRGAN_x2plus.pth` | проверить файл    |

```bash
cd ~/mobile-assets-backend
git checkout feature/prompt-multi-image-comfy
git pull
git checkout -b feature/real-ai-enhancer-upscale

mkdir -p ~/ComfyUI/models/upscale_models
cd ~/ComfyUI/models/upscale_models

wget -O RealESRGAN_x2plus.pth \
https://github.com/xinntao/Real-ESRGAN/releases/download/v0.2.1/RealESRGAN_x2plus.pth

ls -lh ~/ComfyUI/models/upscale_models/RealESRGAN_x2plus.pth
```

---

## 🖥 Terminal 2 — ComfyUI

| Шаг | Команда                                             | Ожидание         |
| --- | --------------------------------------------------- | ---------------- |
| 1   | `cd ~/ComfyUI`                                      | папка ComfyUI    |
| 2   | `source venv/bin/activate`                          | venv active      |
| 3   | `python main.py --listen 0.0.0.0 --port 8188 --cpu` | ComfyUI запущен  |
| 4   | `curl http://localhost:8188/system_stats`           | ComfyUI отвечает |

```bash
cd ~/ComfyUI
source venv/bin/activate
python main.py --listen 0.0.0.0 --port 8188 --cpu
```

---

## 🖥 Terminal 3 — Backend

| Шаг | Команда                                              | Ожидание                 |
| --- | ---------------------------------------------------- | ------------------------ |
| 1   | заменить `workflows/ai_enhancer.json`                | workflow стал RealESRGAN |
| 2   | заменить `generation_service.h/.cpp`                 | generation стала async   |
| 3   | `cd ~/mobile-assets-backend/build`                   | build folder             |
| 4   | `cmake --build .`                                    | сборка успешна           |
| 5   | `export PUBLIC_BASE_URL="http://192.168.0.177:8080"` | public URL               |
| 6   | `./bin/mobile_assets_backend`                        | backend запущен          |

```bash
cd ~/mobile-assets-backend/build
cmake --build .

export PUBLIC_BASE_URL="http://192.168.0.177:8080"
./bin/mobile_assets_backend
```

---

## 🧪 Terminal 4 — quick checks

| Проверка            | Команда                                   | Ожидание             |
| ------------------- | ----------------------------------------- | -------------------- |
| Comfy health        | `curl http://localhost:8080/comfy/health` | ok                   |
| RealESRGAN test     | `curl -X POST /comfy/test-prompt-result`  | outputUrl            |
| Download output     | `curl -o /tmp/enhanced.png /outputs/...`  | файл есть            |
| Size check          | Python PIL                                | output примерно 2x   |
| Android AI Enhancer | Generate                                  | Result image visible |

```bash
curl http://localhost:8080/comfy/health; echo

curl -X POST http://localhost:8080/comfy/test-prompt-result; echo
```

---

## 📌 Real AI Enhancer workflow

| Node | ComfyUI class           | Назначение                        |
| ---- | ----------------------- | --------------------------------- |
| 1    | `LoadImage`             | загрузить input image             |
| 2    | `UpscaleModelLoader`    | загрузить `RealESRGAN_x2plus.pth` |
| 3    | `ImageUpscaleWithModel` | применить модель к изображению    |
| 4    | `SaveImage`             | сохранить enhanced output         |

---

## ⚡ Почему нужна async generation

| Проблема                  | Причина                                     | Решение                                           |
| ------------------------- | ------------------------------------------- | ------------------------------------------------- |
| Android timeout           | RealESRGAN на CPU может работать 50+ секунд | `POST /generations` сразу возвращает `processing` |
| Долгий HTTP request       | ComfyUI upscale занимает время              | background thread                                 |
| Result не готов сразу     | AI ещё работает                             | Android polling через `GET /generations/{taskId}` |
| Нужно сохранить результат | Background task завершился позже            | `CompleteTaskWithResults()`                       |
| Если AI упал              | ComfyUI не вернул output                    | `FailTaskWithFallback()`                          |

---

## 🔄 Async flow

| Step | Что происходит                                |
| ---- | --------------------------------------------- |
| 1    | Android вызывает `POST /generations`          |
| 2    | Backend создаёт task со статусом `processing` |
| 3    | Backend сразу отвечает Android                |
| 4    | В фоне запускается ComfyUI workflow           |
| 5    | ComfyUI создаёт enhanced image                |
| 6    | Backend копирует output в `storage/output`    |
| 7    | Task становится `completed`                   |
| 8    | Android polling получает готовый result       |

---

## 📱 Android проверка

| Экран       | Действие                   | Ожидание                           |
| ----------- | -------------------------- | ---------------------------------- |
| AI Enhancer | выбрать фото               | upload OK                          |
| AI Enhancer | Generate                   | быстрый ответ `processing`         |
| Result      | polling                    | ждёт completed                     |
| Result      | image                      | показывает `/outputs/...png`       |
| Other tools | Ghibli/Ghostface/Templates | не сломаны, copy workflow          |
| Prompt      | multi-image                | не сломан, copy workflow per image |

---

## ⚠️ Ошибки и решения

| Ошибка                           | Где             | Причина                             | Решение                                              |
| -------------------------------- | --------------- | ----------------------------------- | ---------------------------------------------------- |
| `model not found`                | ComfyUI         | модель не в `models/upscale_models` | проверить имя `RealESRGAN_x2plus.pth`                |
| ComfyUI не видит модель          | ComfyUI         | модель добавили после запуска       | перезапустить ComfyUI                                |
| Backend timeout                  | Android         | generation ещё sync                 | заменить `generation_service.h/.cpp` на async версию |
| Task висит `processing`          | backend         | ComfyUI не вернул output            | смотреть logs ComfyUI                                |
| Output не 2x                     | workflow        | всё ещё copy workflow               | проверить `ai_enhancer.json`                         |
| OOM / killed                     | ComfyUI CPU/RAM | фото слишком большое                | уменьшить Android upload max side до 640–768         |
| Android не показывает result     | URL             | PUBLIC_BASE_URL неправильный        | проверить IP и `/outputs/...`                        |
| `include "generation_service.h"` | C++ source      | пропущен `#`                        | должно быть `#include "generation_service.h"`        |

---

## 🧾 Git

| Action            | Command                                                       |
| ----------------- | ------------------------------------------------------------- |
| status            | `git status`                                                  |
| add workflow      | `git add workflows/ai_enhancer.json`                          |
| add async service | `git add src/generation_service.h src/generation_service.cpp` |
| commit            | `git commit -m "Run ComfyUI generations asynchronously"`      |
| push              | `git push -u origin feature/real-ai-enhancer-upscale`         |

```bash
cd ~/mobile-assets-backend

git status

git add src/generation_service.h src/generation_service.cpp workflows/ai_enhancer.json

git commit -m "Run ComfyUI generations asynchronously"

git push -u origin feature/real-ai-enhancer-upscale
```

---

## 🏁 Итог

| Возможность                               | Статус |
| ----------------------------------------- | ------ |
| AI Enhancer стал real AI upscale          | ✅      |
| Используется `RealESRGAN_x2plus.pth`      | ✅      |
| Android не меняется                       | ✅      |
| Backend запускает ComfyUI workflow        | ✅      |
| Generation стала async                    | ✅      |
| Android получает `processing` быстро      | ✅      |
| Result появляется после polling           | ✅      |
| Other workflows не сломаны                | ✅      |
| Следующий кандидат — `upscale_image.json` | ⏭️     |

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
