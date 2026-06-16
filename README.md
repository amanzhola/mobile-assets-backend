# 🔪 feature/ai-enhancer-ultrasharp

| Branch                           | Parent                             | Goal                         | Model               | Workflow                                          | Async | Progress          | Fallback                    | Local ComfyUI                          | Remote Ready            | Android    | Back                                                                                  |
| -------------------------------- | ---------------------------------- | ---------------------------- | ------------------- | ------------------------------------------------- | ----- | ----------------- | --------------------------- | -------------------------------------- | ----------------------- | ---------- | ------------------------------------------------------------------------------------- |
| `feature/ai-enhancer-ultrasharp` | `feature/real-ai-enhancer-upscale` | Сделать AI Enhancer заметнее | `4x-UltraSharp.pth` | `LoadImage → UltraSharp → ImageScale → SaveImage` | ✅     | `progressPercent` | no fake `/uploads` fallback | `COMFY_BASE_URL=http://localhost:8188` | later Kaggle/Cloudflare | no changes | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## ✅ Summary

| Area           | Before                              | After                                                   | Why                                             |
| -------------- | ----------------------------------- | ------------------------------------------------------- | ----------------------------------------------- |
| Enhancer model | `RealESRGAN_x2plus.pth`             | `4x-UltraSharp.pth`                                     | UltraSharp заметнее усиливает резкость и детали |
| Output size    | x2 upscale                          | x4 upscale + optional downscale                         | качество выше, размер контролируемый            |
| Task response  | `processing/completed`              | `processing/completed/error + progressPercent`          | Android видит прогресс                          |
| Error behavior | мог вернуть исходный `/uploads/...` | task becomes `error`, `resultImageUrls=[]`              | больше нет ложного успешного результата         |
| Local output   | пытался скачать через `/view`       | если файл уже есть в `~/ComfyUI/output`, берём напрямую | локальный WSL работает стабильно                |
| Remote output  | позже Kaggle                        | если локального файла нет, скачать через `/view`        | один код для local и remote                     |
| Android        | без изменений                       | без изменений                                           | backend совместим                               |

---

## 🧱 Structure

| Component       | Path                                                | Purpose                           |
| --------------- | --------------------------------------------------- | --------------------------------- |
| Branch          | `feature/ai-enhancer-ultrasharp`                    | sharper AI Enhancer               |
| Workflow        | `workflows/ai_enhancer.json`                        | UltraSharp workflow               |
| Model           | `~/ComfyUI/models/upscale_models/4x-UltraSharp.pth` | ComfyUI upscale model             |
| Backend service | `src/generation_service.h/.cpp`                     | progress, async, no fake fallback |
| Comfy client    | `src/comfy/comfy_client.*`                          | local/remote output support       |
| Main config     | `src/main.cpp`                                      | `COMFY_BASE_URL` support          |
| Output storage  | `storage/output`                                    | final result files                |

---

## 🖥 Terminal 1 — branch and model

| Step      | Command                                                                                                                                  |
| --------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| repo      | `cd ~/mobile-assets-backend`                                                                                                             |
| base      | `git checkout feature/real-ai-enhancer-upscale`                                                                                          |
| branch    | `git checkout -b feature/ai-enhancer-ultrasharp`                                                                                         |
| model dir | `mkdir -p ~/ComfyUI/models/upscale_models`                                                                                               |
| download  | `curl -L -o ~/ComfyUI/models/upscale_models/4x-UltraSharp.pth https://huggingface.co/lokCX/4x-Ultrasharp/resolve/main/4x-UltraSharp.pth` |
| check     | `ls -lh ~/ComfyUI/models/upscale_models/4x-UltraSharp.pth`                                                                               |

---

## 🖥 Terminal 2 — ComfyUI local

| Step   | Command                                             |
| ------ | --------------------------------------------------- |
| open   | `cd ~/ComfyUI`                                      |
| venv   | `source venv/bin/activate`                          |
| run    | `python main.py --listen 0.0.0.0 --port 8188 --cpu` |
| health | `curl http://localhost:8188/system_stats; echo`     |

---

## 🖥 Terminal 3 — backend local

| Step      | Command                                                                                                          |
| --------- | ---------------------------------------------------------------------------------------------------------------- |
| build dir | `cd ~/mobile-assets-backend/build`                                                                               |
| build     | `cmake --build .`                                                                                                |
| run       | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="http://localhost:8188" ./bin/mobile_assets_backend` |
| health    | `curl http://localhost:8080/comfy/health; echo`                                                                  |

Expected:

| Field       | Value                   |
| ----------- | ----------------------- |
| `available` | `true`                  |
| `url`       | `http://localhost:8188` |

---

## 🧪 Test flow

| Step         | Command / Check                                                                                                      | Expected          |
| ------------ | -------------------------------------------------------------------------------------------------------------------- | ----------------- |
| create task  | `POST /generations` with `serverAction=ai_enhancer`                                                                  | fast response     |
| save task id | `TASK_ID=$(echo "$RESPONSE" \| jq -r '.taskId')`                                                                     | real task id      |
| watch        | `watch -n 2 "curl -s http://localhost:8080/generations/$TASK_ID \| jq '{status, progressPercent, resultImageUrls}'"` | progress grows    |
| completed    | task status                                                                                                          | `completed`       |
| result       | `resultImageUrls[0]`                                                                                                 | `/outputs/...png` |
| download     | `curl -o /tmp/ultrasharp.png "$OUTPUT_URL"`                                                                          | image file        |
| inspect      | `file /tmp/ultrasharp.png` / `identify /tmp/ultrasharp.png`                                                          | enhanced image    |

---

## 📊 Progress percent

| Stage                      | progressPercent | Meaning                             |
| -------------------------- | --------------: | ----------------------------------- |
| task created               |             `0` | task stored                         |
| background started         |             `5` | worker started                      |
| input found                |            `10` | backend input exists                |
| copied/uploaded to ComfyUI |            `20` | ComfyUI input ready                 |
| prompt queued              |            `30` | ComfyUI accepted prompt             |
| waiting history            |            `40` | waiting for output                  |
| output filename found      |            `85` | ComfyUI produced output             |
| output source ready        |            `92` | local file or remote download ready |
| saved to backend output    |            `95` | copied to `storage/output`          |
| completed                  |           `100` | result ready                        |
| error                      |           `100` | failed without fake result          |

---

## 🧠 Important logic

| Case                           | Behavior                                                              |
| ------------------------------ | --------------------------------------------------------------------- |
| Local WSL ComfyUI              | output already exists in `~/ComfyUI/output`, backend uses it directly |
| Remote Kaggle/Cloudflare later | local output missing, backend downloads from ComfyUI `/view`          |
| ComfyUI returns no file        | task becomes `error`                                                  |
| No output                      | `resultImageUrls=[]`                                                  |
| No fake fallback               | backend no longer pretends `/uploads/...` is generated result         |
| Android polling                | reads `status`, `progressPercent`, `resultImageUrls`                  |

---

## 🧩 Workflow

| Node | Type                    | Purpose                           |
| ---- | ----------------------- | --------------------------------- |
| 1    | `LoadImage`             | load uploaded image               |
| 2    | `UpscaleModelLoader`    | load `4x-UltraSharp.pth`          |
| 3    | `ImageUpscaleWithModel` | run real upscale                  |
| 4    | `ImageScale`            | downscale to mobile-friendly size |
| 5    | `SaveImage`             | save result                       |

---

## ⚠️ Errors and fixes

| Error                            | Cause                                         | Fix                                                       |
| -------------------------------- | --------------------------------------------- | --------------------------------------------------------- |
| `TASK_ID` literal in curl        | placeholder not replaced                      | use real id: `/generations/mock_task_...`                 |
| model not found                  | wrong path/name                               | check `~/ComfyUI/models/upscale_models/4x-UltraSharp.pth` |
| task stuck processing            | ComfyUI still running or failed               | check ComfyUI terminal                                    |
| `status=error`                   | no output file                                | check workflow/model/logs                                 |
| no `[COMFY_DOWNLOAD_OK]` locally | local file already exists, no download needed | use local output path first                               |
| output too large                 | 4x upscale                                    | keep `ImageScale` downscale                               |
| Android timeout                  | sync generation                               | async flow fixes this                                     |
| fake result image                | fallback returned upload                      | no fallback to `/uploads`                                 |

---

## ✅ Expected result

| Check                       | Expected                                     |
| --------------------------- | -------------------------------------------- |
| `POST /generations`         | quick response with `processing`             |
| `GET /generations/{taskId}` | progress from `0` to `100`                   |
| completed task              | `resultImageUrls` contains `/outputs/...png` |
| failed task                 | `status=error`, empty result urls            |
| Android AI Enhancer         | result image visible                         |
| Other tools                 | unchanged                                    |

---

## 🧾 Git

| Action | Command                                                                                                                                                                       |
| ------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| status | `git status`                                                                                                                                                                  |
| add    | `git add src/api_handler.cpp src/comfy/comfy_client.cpp src/comfy/comfy_client.h src/generation_service.cpp src/generation_service.h src/main.cpp workflows/ai_enhancer.json` |
| commit | `git commit -m "Finalize UltraSharp enhancer with progress tracking"`                                                                                                         |
| push   | `git push -u origin feature/ai-enhancer-ultrasharp`                                                                                                                           |

---

## 🏁 Final state

| Feature                      | Status |
| ---------------------------- | ------ |
| UltraSharp AI Enhancer       | ✅      |
| Async task processing        | ✅      |
| `progressPercent`            | ✅      |
| Local ComfyUI output support | ✅      |
| Remote ComfyUI-ready logic   | ✅      |
| No fake upload fallback      | ✅      |
| Android unchanged            | ✅      |
| Ready base for Kaggle branch | ✅      |

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
