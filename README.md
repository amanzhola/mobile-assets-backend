# 🎨 feature/tools-sdxl-actions

---

| Branch                       | Parent                             | Цель                                              | AI Enhancer                                        | Tools                      | Workflow                      | Main Backend Change                                                                  | Test Result                           | Android            | Back                                                                                  |
| ---------------------------- | ---------------------------------- | ------------------------------------------------- | -------------------------------------------------- | -------------------------- | ----------------------------- | ------------------------------------------------------------------------------------ | ------------------------------------- | ------------------ | ------------------------------------------------------------------------------------- |
| `feature/tools-sdxl-actions` | `feature/ai-enhancer-kaggle-comfy` | Перевести 10 Tools на общий SDXL img2img pipeline | Отдельный `workflows/ai_enhancer.json`, не трогать | 10 tool actions через SDXL | `workflows/tool_img2img.json` | `BuildToolWorkflow`, `IsToolAction`, `BuildToolPositivePrompt`, `ResolveToolDenoise` | all completed, `progressPercent: 100` | No Android changes | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## ✅ Current state

| Area            | Status       | Details                                                                                                                                                   |
| --------------- | ------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| AI Enhancer     | ✅ unchanged  | uses dedicated `workflows/ai_enhancer.json` through `BuildAiEnhancerWorkflow()`                                                                           |
| Tool workflow   | ✅ added      | common `workflows/tool_img2img.json`                                                                                                                      |
| Supported tools | ✅ 10 tools   | `ghibli`, `ghostface`, `glam_makeup`, `remove_objects`, `remove_background`, `skin_improve`, `upscale_image`, `change_scene`, `hair_studio`, `smile_edit` |
| Android params  | ✅ supported  | backend reads `prompt`, `makeupStyle`, `backgroundMode`, `hairstyle`, `length`, `color`, `intensity`                                                      |
| Seed            | ✅ unique     | generated from `steady_clock`                                                                                                                             |
| Denoise         | ✅ per tool   | each tool has own denoise value                                                                                                                           |
| Output safety   | ✅ fixed      | prefix mismatch protection + fallback search by prefix                                                                                                    |
| ComfyUI jobs    | ✅ serialized | `std::mutex comfy_generation_mutex_`                                                                                                                      |
| Validation      | ✅ tested     | all 10 tools completed successfully                                                                                                                       |

---

## 🧱 Main files

| File                             | Purpose                                                                       |
| -------------------------------- | ----------------------------------------------------------------------------- |
| `workflows/tool_img2img.json`    | shared SDXL img2img workflow for tool actions                                 |
| `src/comfy/workflow_builder.h`   | declares `BuildToolWorkflow()`                                                |
| `src/comfy/workflow_builder.cpp` | injects prompt, negative prompt, denoise, seed, input image, output prefix    |
| `src/generation_service.h`       | declares tool routing helpers                                                 |
| `src/generation_service.cpp`     | detects tool actions, builds prompts, resolves denoise, runs ComfyUI workflow |

---

## 🧩 Workflow routing

| Action type   | Workflow                      | Builder method              | Notes                         |
| ------------- | ----------------------------- | --------------------------- | ----------------------------- |
| AI Enhancer   | `workflows/ai_enhancer.json`  | `BuildAiEnhancerWorkflow()` | separate and unchanged        |
| Tool actions  | `workflows/tool_img2img.json` | `BuildToolWorkflow()`       | shared SDXL img2img workflow  |
| Other actions | `workflows/<action>.json`     | `BuildWorkflow()`           | fallback for non-tool actions |

---

## 🛠 Supported tool actions

| Tool                | Uses Android input                                     | Prompt behavior                                                      | Denoise |
| ------------------- | ------------------------------------------------------ | -------------------------------------------------------------------- | ------- |
| `ghibli`            | `prompt`                                               | Studio Ghibli anime style, preserve same person, cinematic, detailed | `0.55`  |
| `ghostface`         | `prompt`                                               | Ghost Face horror style, dark atmosphere, preserve composition       | `0.50`  |
| `glam_makeup`       | `options.makeupStyle`, `prompt`                        | professional makeup, beauty portrait, natural skin texture           | `0.28`  |
| `remove_objects`    | `prompt`                                               | remove described object, fill area naturally                         | `0.45`  |
| `remove_background` | `options.backgroundMode`                               | white studio background or transparent background intent             | `0.20`  |
| `skin_improve`      | default                                                | smooth skin, reduce blemishes, preserve natural texture              | `0.18`  |
| `upscale_image`     | default                                                | restore sharpness, clarity, fine details                             | `0.15`  |
| `change_scene`      | `prompt`                                               | change background to user text, preserve same person                 | `0.55`  |
| `hair_studio`       | `options.hairstyle`, `options.length`, `options.color` | realistic hair transformation, preserve same face                    | `0.30`  |
| `smile_edit`        | `options.intensity`                                    | natural smile edit, realistic teeth, preserve same person            | `0.22`  |

---

## 🔢 Numeric placeholder rule

| Placeholder | Correct                  | Wrong                      | Why                              |
| ----------- | ------------------------ | -------------------------- | -------------------------------- |
| `seed`      | `"seed": {{seed}}`       | `"seed": "{{seed}}"`       | ComfyUI needs number, not string |
| `denoise`   | `"denoise": {{denoise}}` | `"denoise": "{{denoise}}"` | ComfyUI needs number, not string |

---

## 🧠 Shared negative prompt

| Purpose          | Value                                                                                                            |
| ---------------- | ---------------------------------------------------------------------------------------------------------------- |
| Reduce artifacts | `low quality, blurry, distorted face, different person, bad anatomy, artifacts, watermark, text, ugly, deformed` |

---

## 🔒 Output safety

| Problem                                  | Fix                                              |
| ---------------------------------------- | ------------------------------------------------ |
| ComfyUI could return another task output | backend validates expected output prefix         |
| Prefix mismatch                          | logs `[COMFY_OUTPUT_PREFIX_MISMATCH]`            |
| Wrong output from history                | rejected with `std::nullopt`                     |
| Missing expected output                  | backend searches newest ComfyUI output by prefix |
| Multiple simultaneous Android tasks      | ComfyUI execution serialized by mutex            |

---

## 🧪 Validation results

| Tool                | Expected output prefix       | Final status |
| ------------------- | ---------------------------- | ------------ |
| `ghibli`            | `pixo_ghibli_...`            | ✅ completed  |
| `ghostface`         | `pixo_ghostface_...`         | ✅ completed  |
| `glam_makeup`       | `pixo_glam_makeup_...`       | ✅ completed  |
| `remove_objects`    | `pixo_remove_objects_...`    | ✅ completed  |
| `remove_background` | `pixo_remove_background_...` | ✅ completed  |
| `skin_improve`      | `pixo_skin_improve_...`      | ✅ completed  |
| `upscale_image`     | `pixo_upscale_image_...`     | ✅ completed  |
| `change_scene`      | `pixo_change_scene_...`      | ✅ completed  |
| `hair_studio`       | `pixo_hair_studio_...`       | ✅ completed  |
| `smile_edit`        | `pixo_smile_edit_...`        | ✅ completed  |

---

## 📊 Expected task result

| Field             | Expected                          |
| ----------------- | --------------------------------- |
| `status`          | `completed`                       |
| `progressPercent` | `100`                             |
| `resultImageUrls` | valid `/outputs/...` image URL    |
| output ownership  | own task prefix, not another task |
| Android change    | none                              |

---

## ❌ Not implemented yet

| Feature                        | Status          |
| ------------------------------ | --------------- |
| `queued` state                 | not implemented |
| dedicated worker thread        | not implemented |
| multiple GPU workers           | not implemented |
| priority queue                 | not implemented |
| cancellation                   | not implemented |
| retry policy                   | not implemented |
| persistent queue after restart | not implemented |
| websocket progress             | not implemented |
| batching                       | not implemented |

---

## 🧾 Git

| Step   | Command                                               |
| ------ | ----------------------------------------------------- |
| status | `git status`                                          |
| add    | `git add .`                                           |
| commit | `git commit -m "Add SDXL workflows for tool actions"` |
| push   | `git push -u origin feature/tools-sdxl-actions`       |

---

## 🏁 Final result

| Capability                                   | Status |
| -------------------------------------------- | ------ |
| AI Enhancer remains separate                 | ✅      |
| 10 tools use shared SDXL img2img workflow    | ✅      |
| Android parameters are understood by backend | ✅      |
| seed and denoise are numeric                 | ✅      |
| unique seed generation works                 | ✅      |
| ComfyUI jobs are serialized                  | ✅      |
| output prefix safety is enabled              | ✅      |
| all 10 tools tested successfully             | ✅      |
| current state is stable                      | ✅      |

---

## ⬅️ Назад

| Link        | URL                                                                                                                                              |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------ |
| Main README | [https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

