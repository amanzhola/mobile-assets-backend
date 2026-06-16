# 🚀 feature/ai-enhancer-kaggle-comfy

| Branch                             | Parent                                                                | Цель                         | Backend                  | ComfyUI            | Connection                             | Main Result                   | Android            | Back                                                                                  |
| ---------------------------------- | --------------------------------------------------------------------- | ---------------------------- | ------------------------ | ------------------ | -------------------------------------- | ----------------------------- | ------------------ | ------------------------------------------------------------------------------------- |
| `feature/ai-enhancer-kaggle-comfy` | `feature/ai-enhancer-ultrasharp` / `feature/real-ai-enhancer-upscale` | AI Enhancer через Kaggle GPU | Local C++ backend in WSL | Kaggle ComfyUI GPU | Cloudflare Tunnel via `COMFY_BASE_URL` | `/outputs/...` real AI result | No Android changes | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## 🧱 Current state

| Area                | Current value                                                                                                                                                                   | Notes                                                |
| ------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------- |
| Current branch      | `feature/ai-enhancer-kaggle-comfy`                                                                                                                                              | рабочая ветка для Kaggle GPU ComfyUI                 |
| Modified files      | `src/api_handler.cpp`, `src/comfy/workflow_builder.cpp`, `src/comfy/workflow_builder.h`, `src/generation_service.cpp`, `src/generation_service.h`, `workflows/ai_enhancer.json` | изменения перед финальным commit                     |
| Main architecture   | Android → WSL backend → Cloudflare Tunnel → Kaggle ComfyUI → backend `storage/output` → Android Result                                                                          | backend остаётся локальным                           |
| Local ComfyUI mode  | `COMFY_BASE_URL=http://localhost:8188`                                                                                                                                          | для WSL/local testing                                |
| Kaggle ComfyUI mode | `COMFY_BASE_URL=https://xxxx.trycloudflare.com`                                                                                                                                 | для GPU testing                                      |
| Public backend URL  | `PUBLIC_BASE_URL=http://192.168.0.177:8080`                                                                                                                                     | URL, который Android видит в `/uploads` и `/outputs` |
| Main rule           | `/uploads/...` = original input, `/outputs/...` = AI result                                                                                                                     | Android должен показывать `/outputs/...`             |

---

## 🔥 Architecture flow

| Step | From    | To             | Action                           | Result                              |
| ---- | ------- | -------------- | -------------------------------- | ----------------------------------- |
| 1    | Android | WSL backend    | upload image                     | file saved to `storage/input`       |
| 2    | Android | WSL backend    | `POST /generations`              | task created as `processing`        |
| 3    | Backend | Kaggle ComfyUI | `/upload/image`                  | remote ComfyUI receives input image |
| 4    | Backend | Kaggle ComfyUI | `/prompt`                        | workflow queued                     |
| 5    | Backend | Kaggle ComfyUI | `/history/{prompt_id}`           | backend waits for output filename   |
| 6    | Backend | Kaggle ComfyUI | `/view?filename=...&type=output` | backend downloads result image      |
| 7    | Backend | Local storage  | save result                      | file saved to `storage/output`      |
| 8    | Android | Backend        | `GET /generations/{taskId}`      | status becomes `completed`          |
| 9    | Android | Backend        | `GET /outputs/{file}`            | Result Screen shows AI image        |

---

## ⚙️ Env variables

| Variable          | Example                          | Used for                        | Important                              |
| ----------------- | -------------------------------- | ------------------------------- | -------------------------------------- |
| `PUBLIC_BASE_URL` | `http://192.168.0.177:8080`      | public URLs returned to Android | must be reachable from phone           |
| `COMFY_BASE_URL`  | `https://xxxx.trycloudflare.com` | remote ComfyUI URL              | must return JSON from `/system_stats`  |
| Local fallback    | `http://localhost:8188`          | local ComfyUI                   | used only if `COMFY_BASE_URL` is empty |

---

## 🧪 Health checks

| Check                   | Command                                                    | Expected                                                    |
| ----------------------- | ---------------------------------------------------------- | ----------------------------------------------------------- |
| Backend health          | `curl http://localhost:8080/health; echo`                  | `{"status":"ok","service":"mobile_assets_backend"}`         |
| ComfyUI through backend | `curl http://localhost:8080/comfy/health; echo`            | `{"available":true,"url":"https://xxxx.trycloudflare.com"}` |
| Kaggle tunnel direct    | `curl "$COMFY_BASE_URL/system_stats" \| head -c 300; echo` | JSON starting with `{"system":`                             |
| Bad tunnel symptom      | direct curl returns `<!DOCTYPE html>` or HTTP 530          | tunnel is dead or wrong                                     |

---

## 📡 Main endpoints

| Endpoint                       | Method | Purpose               | Storage              | Success result                        |
| ------------------------------ | ------ | --------------------- | -------------------- | ------------------------------------- |
| `/images/upload`               | `POST` | upload original image | `storage/input`      | returns `/uploads/{file}`             |
| `/uploads/{filename}`          | `GET`  | serve original image  | `storage/input`      | original file                         |
| `/generations`                 | `POST` | create AI task        | `storage/tasks.json` | `processing`, `progressPercent: 0`    |
| `/generations/{taskId}`        | `GET`  | check task status     | task storage         | `processing`, `completed`, or `error` |
| `/generations/{taskId}/result` | `GET`  | get result URLs       | task storage         | `/outputs/...`                        |
| `/outputs/{filename}`          | `GET`  | serve generated image | `storage/output`     | AI result image                       |
| `/comfy/health`                | `GET`  | check ComfyUI         | remote/local ComfyUI | `available:true`                      |

---

## ✅ Fixes already made

| Problem                | Old behavior                                                          | Fixed behavior                                                       | Why it matters         |
| ---------------------- | --------------------------------------------------------------------- | -------------------------------------------------------------------- | ---------------------- |
| False success fallback | ComfyUI failed, backend returned `/uploads/original.jpg` as completed | ComfyUI failed → task becomes `error`, `resultImageUrls=[]`          | no fake AI result      |
| Progress stuck         | Android saw `0%` forever                                              | `progressPercent`: `0 → 5 → 10 → 20 → 30 → 40 → 85 → 92 → 95 → 100`  | Android progress works |
| Remote output location | backend searched local `~/ComfyUI/output`                             | backend downloads from Kaggle `/view`                                | remote mode works      |
| HTTPS tunnel           | old HTTP logic unstable                                               | curl-based calls for `/prompt`, `/history`, `/view`, `/upload/image` | Cloudflare works       |
| Hardcoded health URL   | `/comfy/health` showed `localhost:8188`                               | shows `comfy_client_.GetBaseUrl()`                                   | real diagnostics       |
| Wrong task output      | task A could receive task B file                                      | prefix check + unique seed + serialized jobs                         | correct result binding |
| Parallel ComfyUI race  | many jobs could overlap                                               | mutex serializes ComfyUI jobs                                        | safer on Kaggle        |

---

## 🔒 ComfyUI job serialization

| Item                 | Value                                                         | Meaning                             |
| -------------------- | ------------------------------------------------------------- | ----------------------------------- |
| Mutex                | `std::mutex comfy_generation_mutex_`                          | one ComfyUI job at a time           |
| Lock place           | `RunGenerationViaComfy`                                       | serializes actual ComfyUI execution |
| Android behavior     | many tasks can be created immediately                         | tasks wait internally               |
| Current status model | `processing`                                                  | later can add `queued`              |
| Why                  | avoid race, cache mismatch, wrong output prefix, GPU overload | safe production-test mode           |

---

## 🎛️ AI Enhancer modes

| Android mode    | Backend option     | Purpose                                    | Prompt direction                                            | Denoise |
| --------------- | ------------------ | ------------------------------------------ | ----------------------------------------------------------- | ------- |
| HD-улучшение    | `hd_enhance`       | universal quality/detail/sharpness restore | preserve exact person/clothes/composition, improve details  | `0.22`  |
| Ретушь портрета | `portrait_retouch` | face/skin portrait retouch                 | preserve identity, natural skin texture, clean portrait     | `0.18`  |
| Коррекция света | `light_fix`        | exposure/shadows/highlights                | preserve composition, improve brightness and balanced light | `0.16`  |
| Усиление цвета  | `color_boost`      | contrast/color/saturation                  | richer natural colors, better contrast                      | `0.17`  |

---

## 🧩 Workflow placeholders

| Placeholder           | Meaning                | Type rule          |
| --------------------- | ---------------------- | ------------------ |
| `{{input_image}}`     | ComfyUI input filename | string             |
| `{{output_prefix}}`   | unique output prefix   | string             |
| `{{positive_prompt}}` | mode-specific prompt   | string             |
| `{{negative_prompt}}` | anti-artifact prompt   | string             |
| `{{denoise}}`         | img2img denoise value  | number, not string |
| `{{seed}}`            | unique seed per task   | number, not string |

| Correct JSON style       | Wrong JSON style           |
| ------------------------ | -------------------------- |
| `"seed": {{seed}}`       | `"seed": "{{seed}}"`       |
| `"denoise": {{denoise}}` | `"denoise": "{{denoise}}"` |

---

## 🖥 Kaggle setup

| Step | Location          | Command / Action                               | Expected                   |
| ---- | ----------------- | ---------------------------------------------- | -------------------------- |
| 1    | Kaggle Notebook   | GPU enabled                                    | T4/P100/L4 visible         |
| 2    | `/kaggle/working` | clone ComfyUI                                  | `/kaggle/working/ComfyUI`  |
| 3    | ComfyUI           | install requirements                           | dependencies ready         |
| 4    | checkpoints       | download `Juggernaut-XL_Lightning.safetensors` | checkpoint exists          |
| 5    | ComfyUI           | run on `0.0.0.0:8188`                          | local Kaggle ComfyUI works |
| 6    | Kaggle            | run Cloudflare tunnel                          | public URL printed         |
| 7    | WSL               | test `COMFY_BASE_URL/system_stats`             | JSON response              |

---

## 🚀 Backend run for Kaggle

| Step  | Command                                                                                                                   |
| ----- | ------------------------------------------------------------------------------------------------------------------------- |
| Build | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                     |
| Run   | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://xxxx.trycloudflare.com" ./bin/mobile_assets_backend` |
| Check | `curl http://localhost:8080/comfy/health; echo`                                                                           |

---

## 🧪 Test one mode

| Step                    | Value                                                                        |
| ----------------------- | ---------------------------------------------------------------------------- |
| Mode                    | `hd_enhance`                                                                 |
| Endpoint                | `POST /generations`                                                          |
| Expected first response | `status=processing`, `progressPercent=0`                                     |
| Poll endpoint           | `GET /generations/{taskId}`                                                  |
| Expected final response | `status=completed`, `progressPercent=100`, `resultImageUrls[0]=/outputs/...` |

---

## 🧪 Test all 4 modes

| Mode               | Expected                                          |
| ------------------ | ------------------------------------------------- |
| `hd_enhance`       | stronger general enhancement                      |
| `portrait_retouch` | conservative portrait cleanup                     |
| `light_fix`        | lighting/exposure improvement                     |
| `color_boost`      | richer color and contrast                         |
| All modes          | result must be `/outputs/...`, not `/uploads/...` |

---

## 📥 Download result

| Step               | Command                                                                                           |
| ------------------ | ------------------------------------------------------------------------------------------------- |
| Extract output URL | `OUTPUT_URL=$(curl -s http://localhost:8080/generations/$TASK_ID \| jq -r '.resultImageUrls[0]')` |
| Download           | `curl -o /tmp/ai_enhancer_result.png "$OUTPUT_URL"`                                               |
| Check file         | `file /tmp/ai_enhancer_result.png && ls -lh /tmp/ai_enhancer_result.png`                          |
| Windows path       | `wslpath -w /tmp/ai_enhancer_result.png`                                                          |

---

## 📱 Android behavior

| Android step       | Backend expectation               | Correct result           |
| ------------------ | --------------------------------- | ------------------------ |
| AI Enhancer screen | sends `serverAction=ai_enhancer`  | task created             |
| Mode selected      | sends `options.enhanceMode`       | mode prompt selected     |
| Generate           | backend returns `processing` fast | Android starts polling   |
| Progress           | `progressPercent` updates         | UI progress moves        |
| Result             | backend returns `/outputs/...`    | Android shows AI image   |
| Warning            | if Android shows `/uploads/...`   | old fallback or old task |

---

## 🧨 Errors and meaning

| Error / Symptom                 | Meaning                                | Fix                                         |
| ------------------------------- | -------------------------------------- | ------------------------------------------- |
| `/comfy/health available:false` | ComfyUI unavailable                    | check Kaggle, tunnel, env URL               |
| HTML from tunnel                | Cloudflare URL not serving ComfyUI API | restart tunnel, get new URL                 |
| HTTP 530                        | tunnel died                            | restart Kaggle tunnel cell                  |
| `prompt_id not found`           | ComfyUI rejected workflow              | inspect queue response                      |
| `ckpt_name not in list`         | checkpoint missing                     | download checkpoint to `models/checkpoints` |
| `model_name not in []`          | upscale model missing                  | install required upscale model              |
| completed but empty result      | invalid state                          | should be `error`, not completed            |
| prefix mismatch                 | output belongs to another task         | prefix-check rejects it                     |
| output is original image        | false fallback returned `/uploads`     | fallback must not mark completed            |

---

## 🌿 Branch history

| #  | Branch                             | Purpose                         |
| -- | ---------------------------------- | ------------------------------- |
| 1  | `main` / `mobile-assets-backend`   | базовый C++ backend             |
| 2  | `feature/catalog-api`              | `/tools`, `/templates`          |
| 3  | `feature/image-upload`             | image upload to `storage/input` |
| 4  | `feature/task-storage`             | tasks persistence               |
| 5  | `template-prompt-mapping`          | `templateId → prompt`           |
| 6  | `feature/serve-uploads`            | serve `/uploads`                |
| 7  | `feature/local-mock-results`       | stable Android mock result      |
| 8  | `feature/comfyui-worker`           | first ComfyUI integration       |
| 9  | `feature/prompt-multi-image-comfy` | prompt multi-image pipeline     |
| 10 | `feature/real-ai-enhancer-upscale` | first real AI enhancer          |
| 11 | `feature/ai-enhancer-ultrasharp`   | local UltraSharp experiment     |
| 12 | `feature/ai-enhancer-kaggle-comfy` | Kaggle GPU remote ComfyUI       |

---

## 📌 Current modified files before commit

| File                             | Purpose                                                             |
| -------------------------------- | ------------------------------------------------------------------- |
| `src/api_handler.cpp`            | health/result/response handling                                     |
| `src/comfy/workflow_builder.cpp` | placeholder replacement                                             |
| `src/comfy/workflow_builder.h`   | workflow builder API                                                |
| `src/generation_service.cpp`     | async AI generation, progress, mode handling, ComfyUI serialization |
| `src/generation_service.h`       | task fields, mutex, method declarations                             |
| `workflows/ai_enhancer.json`     | SDXL/Juggernaut AI Enhancer workflow                                |

---

## 🧾 Commit

| Step   | Command                                                                                                                                                                  |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| status | `git status`                                                                                                                                                             |
| add    | `git add src/api_handler.cpp src/comfy/workflow_builder.cpp src/comfy/workflow_builder.h src/generation_service.cpp src/generation_service.h workflows/ai_enhancer.json` |
| commit | `git commit -m "Add Kaggle AI enhancer modes and serialized ComfyUI jobs"`                                                                                               |
| push   | `git push -u origin feature/ai-enhancer-kaggle-comfy`                                                                                                                    |

---

## ⚠️ Current known limitation

| Issue                                                 | Current rule                                               |
| ----------------------------------------------------- | ---------------------------------------------------------- |
| SDXL/Juggernaut may change details if denoise is high | keep denoise low                                           |
| Best old-photo range                                  | `0.18–0.22`                                                |
| Portrait-safe range                                   | `0.16–0.18`                                                |
| Stronger but riskier                                  | `0.28+`                                                    |
| Next quality work                                     | tune prompt, negative prompt, denoise, steps, cfg, sampler |

---

## 🔜 Next steps

| Priority | Step                                                                          | Why                                                     |
| -------- | ----------------------------------------------------------------------------- | ------------------------------------------------------- |
| 1        | visually compare `hd_enhance`, `portrait_retouch`, `light_fix`, `color_boost` | choose best mode settings                               |
| 2        | add status `queued`                                                           | current mutex makes tasks wait while still `processing` |
| 3        | separate workflows per mode                                                   | better quality per use case                             |
| 4        | portrait workflow with face restoration                                       | better face quality                                     |
| 5        | stable GPU deployment                                                         | Cloudflare tunnel is temporary                          |
| 6        | production options                                                            | Yandex Cloud GPU, RunPod, Vast.ai, Modal, dedicated API |

---

## 🏁 Final result

| Capability                                        | Status |
| ------------------------------------------------- | ------ |
| Android uses local backend                        | ✅      |
| Backend uses Kaggle ComfyUI GPU                   | ✅      |
| ComfyUI URL configurable through `COMFY_BASE_URL` | ✅      |
| Remote input upload works through `/upload/image` | ✅      |
| Queue prompt works through HTTPS tunnel           | ✅      |
| History polling works                             | ✅      |
| Output download through `/view` works             | ✅      |
| Result saved to `storage/output`                  | ✅      |
| Android receives `/outputs/...`                   | ✅      |
| No false completed fallback to original image     | ✅      |
| Progress percent works                            | ✅      |
| 4 AI Enhancer modes supported                     | ✅      |
| ComfyUI jobs serialized safely                    | ✅      |

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
