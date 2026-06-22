# 🧽 feature/remove-objects-sam2-flux-fill

| Branch                                  | Parent / Base                                                  | Goal                                       | Main Pipeline                                               | Best Fix                   | Current Status            | Product Decision                        | Next Branch                          | Android                         | Back                                                                                  |
| --------------------------------------- | -------------------------------------------------------------- | ------------------------------------------ | ----------------------------------------------------------- | -------------------------- | ------------------------- | --------------------------------------- | ------------------------------------ | ------------------------------- | ------------------------------------------------------------------------------------- |
| `feature/remove-objects-sam2-flux-fill` | after `feature/local-tool-runner` / Remove Objects experiments | лучший текущий auto Remove Objects backend | GroundingDINO + SAM mask → ComfyUI inpaint → post-composite | keep original outside mask | best automatic first pass | auto cleanup first, manual brush second | `feature/remove-objects-manual-mask` | Android unchanged for auto pass | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## 🌳 Tree

```text

🌳 Tree

mobile-assets-backend/
├── CMakeLists.txt
├── conanfile.txt
├── README.md
├── data/
│   ├── onboarding.json
│   ├── templates.json
│   └── tools.json
├── models/
│   └── local/
│       └── sam_vit_b_01ec64.pth
├── scripts/
│   ├── apply_inpaint_mask.py
│   ├── create_object_mask.py
│   ├── create_object_mask_sam.py
│   ├── remove_background.py
│   └── remove_objects.py
├── src/
│   ├── comfy/
│   │   ├── comfy_client.cpp
│   │   ├── comfy_client.h
│   │   ├── workflow_builder.cpp
│   │   └── workflow_builder.h
│   ├── generation/
│   │   ├── generation_json.cpp
│   │   ├── generation_json.h
│   │   ├── generation_task_store.cpp
│   │   ├── generation_task_store.h
│   │   ├── generation_template_workflow.cpp
│   │   ├── generation_template_workflow.h
│   │   ├── generation_tool_prompts.cpp
│   │   └── generation_tool_prompts.h
│   ├── local_tools/
│   │   ├── local_tool_runner.cpp
│   │   └── local_tool_runner.h
│   ├── api_handler.cpp
│   ├── api_handler.h
│   ├── catalog_service.cpp
│   ├── catalog_service.h
│   ├── generation_service.cpp
│   ├── generation_service.h
│   ├── http_server.cpp
│   ├── http_server.h
│   ├── main.cpp
│   ├── output_service.cpp
│   ├── output_service.h
│   ├── template_asset_service.cpp
│   ├── template_asset_service.h
│   ├── upload_service.cpp
│   └── upload_service.h
├── storage/
│   ├── input/
│   ├── output/
│   ├── tasks.json
│   └── template_cache/
└── workflows/
    ├── ai_enhancer.json
    ├── remove_objects_inpaint.json
    ├── template_img2img.json
    └── tool_img2img.json

```

---

## ✅ Current stable base

| Item                 | Value                                                                                                     |
| -------------------- | --------------------------------------------------------------------------------------------------------- |
| Working branch       | `feature/remove-objects-sam2-flux-fill`                                                                   |
| Current role         | best available backend version for automatic Remove Objects                                               |
| Main architecture    | source image → GroundingDINO + SAM mask → ComfyUI inpaint → post-composite original outside mask → output |
| Key fix              | only masked area comes from ComfyUI output                                                                |
| Preserved area       | everything outside mask is restored from original image                                                   |
| Main benefit         | avoids face/body/background distortion outside selected object area                                       |
| Remaining limitation | thin parts can remain if mask misses them                                                                 |

---

## 🔁 Current pipeline

| Step | Input                                | Action                                           | Output                                          |
| ---- | ------------------------------------ | ------------------------------------------------ | ----------------------------------------------- |
| 1    | source image                         | user chooses object prompt, for example `зонтик` | text target                                     |
| 2    | source image + text                  | GroundingDINO detects object region              | rough object box                                |
| 3    | object box                           | SAM generates mask                               | object mask                                     |
| 4    | source image + mask                  | ComfyUI inpaint workflow                         | full decoded inpaint image                      |
| 5    | original image + Comfy output + mask | `apply_inpaint_mask.py` post-composite           | original outside mask, inpaint only inside mask |
| 6    | final image                          | backend saves output                             | `/outputs/pixo_remove_objects_...png`           |

---

## 🧠 Important fix

| Problem                                          | Cause                                                                         | Fix                                                              | Result                     |
| ------------------------------------------------ | ----------------------------------------------------------------------------- | ---------------------------------------------------------------- | -------------------------- |
| face/person/body changed outside selected object | ComfyUI inpaint returns full decoded image and can modify pixels outside mask | `scripts/apply_inpaint_mask.py` composites original outside mask | only masked area changes   |
| background became inconsistent                   | full SDXL decoded image changed surrounding pixels                            | preserve original outside mask                                   | better visual consistency  |
| person distortion                                | SDXL altered face/body even outside object                                    | post-composite original outside mask                             | face/body preserved better |

---

## 📂 Main files

| File                                    | Purpose                                                        |
| --------------------------------------- | -------------------------------------------------------------- |
| `scripts/create_object_mask_sam.py`     | creates object mask using GroundingDINO/SAM pipeline           |
| `scripts/apply_inpaint_mask.py`         | keeps original outside mask and Comfy output only inside mask  |
| `workflows/remove_objects_inpaint.json` | ComfyUI inpaint workflow                                       |
| `src/local_tools/local_tool_runner.cpp` | local runner integration for object mask / inpaint preparation |
| `src/generation_service.cpp`            | generation routing and Remove Objects flow                     |
| `src/comfy/workflow_builder.cpp`        | builds workflow input for ComfyUI                              |
| `models/local/sam_vit_b_01ec64.pth`     | local SAM checkpoint                                           |

---

## 🧪 Previous branches and results

| Branch                                  | Purpose                                                                     | Result                                         | Problem                                                     | Conclusion                                       |
| --------------------------------------- | --------------------------------------------------------------------------- | ---------------------------------------------- | ----------------------------------------------------------- | ------------------------------------------------ |
| `feature/local-tool-runner`             | extract local remove background runner                                      | Remove Background white/transparent works well | not Remove Objects                                          | keep as stable local tools base                  |
| `feature/remove-objects-auto-mask`      | text-guided auto mask + local inpaint                                       | object removed                                 | removed area blurry/muddy, face/person could distort        | not production quality                           |
| `feature/remove-objects-comfy-inpaint`  | use ComfyUI inpaint instead of local blur                                   | umbrella removed                               | face/head damaged, background mismatch, thin parts remained | Comfy inpaint needs better mask + post-composite |
| `feature/remove-objects-sam2-flux-fill` | improve mask with GroundingDINO + SAM, Comfy inpaint, original outside mask | best current result                            | thin stick/handle/top tip may remain                        | keep as first-pass auto cleanup                  |

---

## ✅ Current branch result

| Test object     | Result                                            |
| --------------- | ------------------------------------------------- |
| Umbrella canopy | detected and removed                              |
| Removed area    | better than local blur/inpaint                    |
| Background      | reconstructed better than auto-mask local inpaint |
| Face/person     | preserved after post-composite fix                |
| Thin parts      | stick / handle / top tip may remain               |
| Current quality | best automatic Remove Objects backend branch      |

---

## ⚠️ Current limitation

| Limitation                                      | Why it happens                                                           | Product decision |
| ----------------------------------------------- | ------------------------------------------------------------------------ | ---------------- |
| umbrella stick may remain                       | SAM/GroundingDINO may detect only canopy                                 |                  |
| handle may remain                               | thin object parts are hard for automatic mask                            |                  |
| top tip may remain                              | tiny object fragments often missed                                       |                  |
| similar problem for wires/straps/glasses/spokes | automatic detection is not reliable for thin geometry                    |                  |
| do not add fragile object-specific rules        | umbrella-only rules will not generalize                                  |                  |
| correct solution                                | two-step Remove Objects: auto cleanup first, manual brush cleanup second |                  |

---

## ❌ Failed experiments

| Experiment                 | Attempt                                                  | Error / Problem                                                | Conclusion                                          |
| -------------------------- | -------------------------------------------------------- | -------------------------------------------------------------- | --------------------------------------------------- |
| IOPaint / LaMa             | `pip install -U iopaint`                                 | Pillow failed to build on Python 3.12                          | do not use IOPaint in current `.venv-tools`         |
| Florence2                  | `pip install transformers accelerate sentencepiece timm` | `Florence2LanguageConfig has no attribute forced_bos_token_id` | do not use Florence2 in current backend environment |
| special umbrella geometry  | auto-expand mask by object shape                         | fragile and object-specific                                    | do not implement                                    |
| local OpenCV-style inpaint | blur/fill masked area locally                            | muddy patch                                                    | not production quality                              |

---

## 🧰 Setup

| Step             | Command                                                                                                          |
| ---------------- | ---------------------------------------------------------------------------------------------------------------- |
| activate env     | `cd ~/mobile-assets-backend && source .venv-tools/bin/activate`                                                  |
| install packages | `pip install transformers segment-anything timm opencv-python pillow numpy torch torchvision`                    |
| create model dir | `mkdir -p models/local`                                                                                          |
| download SAM     | `wget -O models/local/sam_vit_b_01ec64.pth https://dl.fbaipublicfiles.com/segment_anything/sam_vit_b_01ec64.pth` |

---

## 🧪 Compare branch results

| Step        | Command                                                                                                                         |      |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------- | ---- |
| checkout    | `git switch feature/remove-objects-sam2-flux-fill`                                                                              |      |
| build       | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                           |      |
| run backend | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend` |      |
| create task | use `POST /generations` with `serverAction=remove_objects`                                                                      |      |
| poll task   | `watch -n 3 "curl -s http://localhost:8080/generations/$TASK_ID                                                                 | jq"` |
| download    | `curl -o /tmp/remove_objects_result.png "$OUTPUT_URL"`                                                                          |      |
| compare     | check removed object, blur, face, body, thin leftovers, background consistency                                                  |      |

---

## 🧪 Remove Objects curl test

```bash
RESPONSE=$(curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "serverAction": "remove_objects",
  "toolType": "REMOVE_OBJECTS",
  "sourceImageUrl": "http://192.168.0.177:8080/uploads/YOUR_TEST_IMAGE.jpg",
  "prompt": "зонтик",
  "outputCount": 1
}')

echo "$RESPONSE" | jq

TASK_ID=$(echo "$RESPONSE" | jq -r ".taskId")

watch -n 3 "curl -s http://localhost:8080/generations/$TASK_ID | jq"
```

---

## 📥 Download result

```bash
OUTPUT_URL=$(curl -s http://localhost:8080/generations/$TASK_ID | jq -r ".resultImageUrls[0]")

echo "$OUTPUT_URL"

curl -o /tmp/remove_objects_result.png "$OUTPUT_URL"
```

---

## 👀 Visual checklist

| Question                   | Expected for current branch               |
| -------------------------- | ----------------------------------------- |
| Was the object removed?    | canopy/object main part should be removed |
| Is removed area blurry?    | should be cleaner than local inpaint      |
| Did face change?           | should be preserved                       |
| Did body/clothes change?   | should be preserved outside mask          |
| Did thin leftovers remain? | possible                                  |
| Is background consistent?  | mostly consistent                         |
| Is it production-perfect?  | not yet; needs manual cleanup second pass |

---

## 🧭 Product decision

| Decision                                                                      | Reason                         |
| ----------------------------------------------------------------------------- | ------------------------------ |
| keep `feature/remove-objects-sam2-flux-fill` as current best automatic branch | best first-pass Remove Objects |
| stop trying to fully guess every thin part automatically                      | fragile and non-general        |
| add manual cleanup                                                            | gives user precise control     |
| only inpaint painted area                                                     | protects face/body/background  |
| do not fake Remove Objects                                                    | mask-based inpaint is required |

---

## 🔜 Planned next branch

| Branch                               | Purpose                                        |
| ------------------------------------ | ---------------------------------------------- |
| `feature/remove-objects-manual-mask` | second-pass manual brush cleanup for leftovers |

```bash
git switch feature/remove-objects-sam2-flux-fill
git switch -c feature/remove-objects-manual-mask
```

---

## 🎯 Manual cleanup target flow

| Step | UX / Backend                                   |
| ---- | ---------------------------------------------- |
| 1    | Remove Objects auto generation                 |
| 2    | Result Screen                                  |
| 3    | user taps Recreate                             |
| 4    | Manual Refine Screen opens                     |
| 5    | user draws mask over leftover artifact         |
| 6    | Android uploads mask PNG                       |
| 7    | Android sends `remove_objects_cleanup` request |
| 8    | backend inpaints only selected mask area       |
| 9    | backend post-composites original outside mask  |
| 10   | result opens as normal                         |

---

## 📱 Android manual cleanup route

| Item              | Value                                                                               |
| ----------------- | ----------------------------------------------------------------------------------- |
| Tool              | only `ToolType.REMOVE_OBJECTS`                                                      |
| Recreate behavior | opens refine screen instead of normal regenerate                                    |
| Route             | `ResultDestination → AppRoute.RemoveObjectsRefine → RemoveObjectsRefineDestination` |
| Screen            | `PixoRemoveObjectsRefineScreen`                                                     |
| Purpose           | draw exact mask over remaining object parts                                         |

---

## 📱 Android files added for manual cleanup

| File                                                                                         |
| -------------------------------------------------------------------------------------------- |
| `app/src/main/java/com/company/pixo/core/navigation/tools/RemoveObjectsRefineDestination.kt` |
| `app/src/main/java/com/company/pixo/feature/editor/PixoRemoveObjectsRefineRoute.kt`          |
| `app/src/main/java/com/company/pixo/feature/editor/PixoRemoveObjectsRefineScreen.kt`         |
| `app/src/main/res/drawable/ic_baseline_add_task_24.xml`                                      |
| `app/src/main/res/drawable/ic_baseline_brush_24.xml`                                         |

---

## 📱 Android files modified for manual cleanup

| File                                                                      |
| ------------------------------------------------------------------------- |
| `app/src/main/java/com/company/pixo/core/navigation/AppNavHost.kt`        |
| `app/src/main/java/com/company/pixo/core/navigation/AppRoute.kt`          |
| `app/src/main/java/com/company/pixo/core/navigation/ResultDestination.kt` |

---

## 🖌️ Manual refine screen features

| Feature                        |
| ------------------------------ |
| fullscreen result image        |
| back button                    |
| brush toggle                   |
| undo last stroke               |
| zoom + / -                     |
| move image when brush is off   |
| draw mask when brush is on     |
| check button to submit cleanup |
| save state on rotation         |
| mask PNG export                |
| upload mask to backend         |

---

## 📤 Backend cleanup request contract

| Field                  | Value                                                         |
| ---------------------- | ------------------------------------------------------------- |
| `toolType`             | `REMOVE_OBJECTS`                                              |
| `backendType`          | `REMOVE_OBJECTS`                                              |
| `serverAction`         | `remove_objects_cleanup`                                      |
| `sourceImageUrl`       | current generated result image                                |
| `sourceImageUri`       | `null`                                                        |
| `prompt`               | `remove selected leftovers, reconstruct background naturally` |
| `templateId`           | `null`                                                        |
| `options.sourceTaskId` | previous Remove Objects task id                               |
| `options.maskImageUrl` | uploaded brush mask PNG                                       |
| `options.mode`         | `manual_cleanup`                                              |
| `historyIdentity`      | `REMOVE_OBJECTS`                                              |

---

## 📄 Example cleanup request

```json
{
  "toolType": "REMOVE_OBJECTS",
  "backendType": "REMOVE_OBJECTS",
  "serverAction": "remove_objects_cleanup",
  "sourceImageUrl": "http://192.168.0.177:8080/outputs/pixo_remove_objects_....png",
  "sourceImageUri": null,
  "prompt": "remove selected leftovers, reconstruct background naturally",
  "templateId": null,
  "options": {
    "sourceTaskId": "previous_generation_task_id",
    "maskImageUrl": "http://192.168.0.177:8080/uploads/mask_....png",
    "mode": "manual_cleanup"
  },
  "historyIdentity": "REMOVE_OBJECTS"
}
```

---

## 🎭 Mask format

| Mask color | Meaning             |
| ---------- | ------------------- |
| black      | keep / do not touch |
| white      | remove / inpaint    |

| Mask rule  | Requirement                            |
| ---------- | -------------------------------------- |
| canvas     | must match visible result image canvas |
| white area | only this area should be inpainted     |
| black area | must remain unchanged                  |
| backend    | resize/align mask if needed            |

---

## 🧠 Backend cleanup behavior to add

| Step | Action                                                |
| ---- | ----------------------------------------------------- |
| 1    | validate `sourceImageUrl`                             |
| 2    | validate `options.maskImageUrl`                       |
| 3    | resolve/download source image                         |
| 4    | resolve/download mask image                           |
| 5    | convert mask to Comfy-compatible alpha mask if needed |
| 6    | run existing remove objects inpaint workflow          |
| 7    | apply post-composite so only mask area changes        |
| 8    | save output                                           |
| 9    | return normal generation result                       |

---

## 🧪 Manual cleanup test scenario

| Step | Expected                                       |
| ---- | ---------------------------------------------- |
| 1    | Open History                                   |
| 2    | Open existing Remove Objects result            |
| 3    | Tap Recreate                                   |
| 4    | Refine screen opens                            |
| 5    | Enable brush                                   |
| 6    | Draw mask over remaining artifact              |
| 7    | Tap check                                      |
| 8    | Android uploads mask PNG                       |
| 9    | Android sends `remove_objects_cleanup` request |
| 10   | Backend inpaints only selected area            |
| 11   | App opens Generation / Result as normal        |

---

## 🖥️ Expected backend log for manual cleanup

```text
serverAction=remove_objects_cleanup
toolType=REMOVE_OBJECTS
sourceImageUrl=<previous result image>
maskImageUrl=<uploaded mask png>
mode=manual_cleanup
```

---

## 🏁 Current expected result

| Branch                                  | Expected result                                                                           |
| --------------------------------------- | ----------------------------------------------------------------------------------------- |
| `feature/remove-objects-sam2-flux-fill` | canopy removed, background mostly clean, face preserved, some thin leftovers may remain   |
| `feature/remove-objects-manual-mask`    | auto result first, manual brush cleanup removes leftovers, only painted mask area changes |

---

## 🧾 Git

| Step   | Command                                                     |
| ------ | ----------------------------------------------------------- |
| status | `git status`                                                |
| add    | `git add .`                                                 |
| commit | `git commit -m "Add SAM based Remove Objects inpaint flow"` |
| push   | `git push -u origin feature/remove-objects-sam2-flux-fill`  |

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
