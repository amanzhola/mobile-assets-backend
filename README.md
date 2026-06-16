# 🚀 feature/ai-enhancer-kaggle-comfy

| Branch                             | Parent                                                                | Цель                                | Главная схема                                                                                              | Где backend   | Где ComfyUI         | Как соединяются                               | Главный фикс                                                   | Android             | Назад                                                                                 |
| ---------------------------------- | --------------------------------------------------------------------- | ----------------------------------- | ---------------------------------------------------------------------------------------------------------- | ------------- | ------------------- | --------------------------------------------- | -------------------------------------------------------------- | ------------------- | ------------------------------------------------------------------------------------- |
| `feature/ai-enhancer-kaggle-comfy` | `feature/real-ai-enhancer-upscale` / `feature/ai-enhancer-ultrasharp` | Запускать AI Enhancer на Kaggle GPU | Android → локальный C++ backend → Cloudflare Tunnel → Kaggle ComfyUI GPU → backend output → Android Result | WSL / ноутбук | Kaggle Notebook GPU | `COMFY_BASE_URL=https://...trycloudflare.com` | backend upload-ит input image в remote ComfyUI `/upload/image` | Android не меняется | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## ✅ Что сделано

| Блок             | Было                               | Стало                                             | Почему важно                                       | Статус |
| ---------------- | ---------------------------------- | ------------------------------------------------- | -------------------------------------------------- | ------ |
| ComfyUI location | локально / CPU                     | Kaggle GPU                                        | быстрее и реальнее для AI workflow                 | ✅      |
| Backend location | WSL                                | WSL                                               | backend остаётся локальным                         | ✅      |
| Android base URL | локальный backend                  | без изменений                                     | Android не надо переделывать                       | ✅      |
| Comfy URL        | `http://localhost:8188`            | `COMFY_BASE_URL` из env                           | можно менять tunnel URL без правки кода            | ✅      |
| `/comfy/health`  | показывал hardcoded localhost      | показывает `comfy_client_.GetBaseUrl()`           | видно реальный Kaggle URL                          | ✅      |
| Upload input     | локальный `copy_file`              | remote upload в Kaggle `/upload/image`            | Kaggle не видит файлы WSL напрямую                 | ✅      |
| Queue prompt     | локальный HTTP                     | HTTPS через Cloudflare                            | backend ставит workflow в очередь Kaggle ComfyUI   | ✅      |
| History          | локальный `/history`               | remote `/history/{prompt_id}`                     | backend ждёт результат Kaggle                      | ✅      |
| Output           | backend искал файл локально        | скачивает output через ComfyUI `/view`            | результат переносится из Kaggle в `storage/output` | ✅      |
| Fallback         | мог вернуть исходник как completed | task становится error, если ComfyUI не дал output | больше нет ложного успеха                          | ✅      |
| Kaggle setup     | вручную много шагов                | 2 ячейки после Factory Reset                      | быстрый повторяемый запуск                         | ✅      |

---

## 🧱 Итоговая архитектура

| Layer             | Location          | URL / Path                     | Role                            | Notes                              |
| ----------------- | ----------------- | ------------------------------ | ------------------------------- | ---------------------------------- |
| Android           | phone / emulator  | `http://192.168.0.177:8080`    | client                          | ходит только в локальный backend   |
| C++ backend       | WSL / ноутбук     | `~/mobile-assets-backend`      | API gateway                     | принимает upload/generation/result |
| Cloudflare Tunnel | Kaggle            | `https://...trycloudflare.com` | bridge                          | внешний URL к Kaggle ComfyUI       |
| ComfyUI           | Kaggle GPU        | `/kaggle/working/ComfyUI`      | AI worker                       | выполняет workflow                 |
| Input upload      | backend → Kaggle  | `/upload/image`                | отправка image в remote ComfyUI | обязательно для remote mode        |
| Prompt queue      | backend → Kaggle  | `/prompt`                      | запуск workflow                 | через `ComfyClient`                |
| History polling   | backend → Kaggle  | `/history/{prompt_id}`         | ожидание результата             | получает filename                  |
| Output download   | backend ← Kaggle  | `/view?...`                    | скачать result image            | сохраняется в `storage/output`     |
| Android Result    | Android ← backend | `/outputs/...png`              | показ результата                | backend отдаёт локальный output    |

---

## 🖥 Kaggle Notebook — 2 ячейки после Factory Reset

| Cell   | Назначение | Что делает                                                                     | Результат                                       |
| ------ | ---------- | ------------------------------------------------------------------------------ | ----------------------------------------------- |
| Cell 1 | Setup      | clone ComfyUI, install requirements, download checkpoint, download cloudflared | `SETUP DONE`                                    |
| Cell 2 | Run        | запускает ComfyUI в фоне, запускает Cloudflare tunnel, печатает public URL     | `COMFY_PUBLIC_URL=https://...trycloudflare.com` |

---

## 🖥 WSL Backend запуск

| Step               | Command / Value                                 | Expected                                   |
| ------------------ | ----------------------------------------------- | ------------------------------------------ |
| backend path       | `cd ~/mobile-assets-backend/build`              | build folder                               |
| public backend URL | `PUBLIC_BASE_URL="http://192.168.0.177:8080"`   | Android получает правильные `/outputs/...` |
| remote ComfyUI URL | `COMFY_BASE_URL="https://...trycloudflare.com"` | backend ходит в Kaggle                     |
| run backend        | `./bin/mobile_assets_backend`                   | backend started                            |
| health check       | `curl http://localhost:8080/comfy/health`       | `available:true` + Kaggle URL              |

---

## 🧪 Проверки

| Проверка                | Команда / действие                               | Ожидание                           | Если ошибка                  |
| ----------------------- | ------------------------------------------------ | ---------------------------------- | ---------------------------- |
| Kaggle local ComfyUI    | `curl 127.0.0.1:8188/system_stats` inside Kaggle | JSON начинается с `{"system":...}` | ComfyUI не запущен           |
| Cloudflare URL from WSL | `curl "$COMFY_BASE_URL/system_stats"`            | JSON, не HTML                      | tunnel плохой / умер         |
| Backend health          | `curl localhost:8080/comfy/health`               | `available:true`                   | env не дошёл или tunnel умер |
| Android AI Enhancer     | Generate                                         | task → processing → completed      | смотреть backend logs        |
| Backend output          | `curl -I /outputs/file.png`                      | `200 OK`                           | output не скачался           |
| Kaggle logs             | Notebook log                                     | `got prompt`, `Prompt executed`    | workflow/model issue         |

---

## ⚠️ Главные ошибки и исправления

| Ошибка                               | Симптом                                                            | Причина                                 | Исправление                                                                               |
| ------------------------------------ | ------------------------------------------------------------------ | --------------------------------------- | ----------------------------------------------------------------------------------------- |
| Backend всё ещё показывает localhost | `/comfy/health` → `url:"http://localhost:8188"`                    | код не читает env или health hardcoded  | `COMFY_BASE_URL` в `main.cpp`, `GetBaseUrl()` в `ComfyClient`, health берёт URL из client |
| Cloudflare отдаёт HTML               | `curl "$COMFY_BASE_URL/system_stats"` показывает `<!DOCTYPE html>` | tunnel не ведёт к ComfyUI API           | перезапустить Kaggle ComfyUI + tunnel, взять новый URL                                    |
| HTTP 530                             | Cloudflare error                                                   | tunnel умер / Kaggle cell остановлена   | перезапустить Cell 2                                                                      |
| LoadImage cannot find image          | ComfyUI error                                                      | Kaggle не видит файл из WSL             | backend должен вызвать `/upload/image` перед `/prompt`                                    |
| History empty `{}`                   | backend ждёт output, но history пустая                             | prompt ещё не готов или wrong prompt id | polling / проверить queue response                                                        |
| Backend искал output локально        | result не найден                                                   | output лежит на Kaggle, не в WSL        | скачать через ComfyUI `/view`                                                             |
| Completed с исходником               | Android видел input как result                                     | ложный fallback                         | убрать fallback на success, делать task `error`                                           |
| QueuePrompt не работает через HTTPS  | no prompt id                                                       | старый local-only HTTP client           | curl-based HTTPS call / поддержка remote URL                                              |
| Kaggle после Factory Reset пустой    | нет ComfyUI/model/cloudflared                                      | reset очищает `/kaggle/working`         | запускать 2 setup cells заново                                                            |

---

## 🔄 Правильный порядок веток

| Order | Branch                             | Meaning                                         | Status               |
| ----- | ---------------------------------- | ----------------------------------------------- | -------------------- |
| 1     | `feature/prompt-multi-image-comfy` | multi-image prompt через ComfyUI copy pipeline  | база                 |
| 2     | `feature/real-ai-enhancer-upscale` | real AI enhancer через local RealESRGAN / async | качество локально    |
| 3     | `feature/ai-enhancer-ultrasharp`   | эксперимент качества UltraSharp                 | experiment           |
| 4     | `feature/ai-enhancer-kaggle-comfy` | remote Kaggle GPU infrastructure                | текущая рабочая база |

---

## 🧠 Почему эта ветка важнее локальной

| Локальный режим                   | Kaggle режим                          |
| --------------------------------- | ------------------------------------- |
| backend и ComfyUI на одной машине | backend локально, ComfyUI remote      |
| `copy_file` работает              | `copy_file` недостаточно              |
| output можно искать локально      | output надо скачивать через `/view`   |
| CPU медленно                      | GPU быстрее                           |
| проще отладка                     | ближе к real AI backend architecture  |
| подходит для copy workflow        | подходит для SDXL / heavier workflows |

---

## 🎯 Что продолжать именно в этой ветке

| Следующий шаг   | Что менять                   | Почему                         |
| --------------- | ---------------------------- | ------------------------------ |
| Denoise tests   | `workflows/ai_enhancer.json` | найти баланс качества          |
| `denoise=0.18`  | workflow parameter           | сильнее сохраняет оригинал     |
| `denoise=0.22`  | workflow parameter           | умеренный enhancer             |
| `denoise=0.26`  | workflow parameter           | больше улучшений               |
| `denoise=0.30`  | workflow parameter           | может менять лицо/шапку/детали |
| Negative prompt | workflow prompt              | уменьшить артефакты            |
| Steps / sampler | workflow                     | скорость/качество              |
| Better model    | checkpoint                   | качество результата            |
| Face restore    | отдельный workflow node      | позже, если нужно              |

---

## 📌 Ключевые env variables

| Variable          | Example                         | Used by             | Meaning                 |
| ----------------- | ------------------------------- | ------------------- | ----------------------- |
| `PUBLIC_BASE_URL` | `http://192.168.0.177:8080`     | backend output URLs | URL backend для Android |
| `COMFY_BASE_URL`  | `https://abc.trycloudflare.com` | `ComfyClient`       | remote ComfyUI URL      |
| none              | default `http://localhost:8188` | local fallback      | локальный ComfyUI режим |

---

## 🧾 Git

| Action        | Command                                                                                                                   |
| ------------- | ------------------------------------------------------------------------------------------------------------------------- |
| create branch | `git checkout -b feature/ai-enhancer-kaggle-comfy`                                                                        |
| check status  | `git status`                                                                                                              |
| add files     | `git add src/main.cpp src/comfy/comfy_client.h src/comfy/comfy_client.cpp src/generation_service.cpp src/api_handler.cpp` |
| commit        | `git commit -m "Support remote ComfyUI over Kaggle tunnel"`                                                               |
| push          | `git push -u origin feature/ai-enhancer-kaggle-comfy`                                                                     |

---

## 🏁 Итог

| Возможность                                      | Статус |
| ------------------------------------------------ | ------ |
| Android работает с локальным backend             | ✅      |
| Backend ходит в Kaggle ComfyUI через Cloudflare  | ✅      |
| `COMFY_BASE_URL` управляет remote URL            | ✅      |
| `/comfy/health` показывает реальный URL          | ✅      |
| Input image загружается в Kaggle `/upload/image` | ✅      |
| Prompt ставится в remote ComfyUI                 | ✅      |
| History читается с remote ComfyUI                | ✅      |
| Output скачивается из Kaggle через `/view`       | ✅      |
| Result сохраняется в backend `storage/output`    | ✅      |
| Android получает `/outputs/...`                  | ✅      |
| Ложный fallback убран                            | ✅      |
| Текущая ветка — база для дальнейшего качества    | ✅      |

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
