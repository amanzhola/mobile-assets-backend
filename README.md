# 🖌️ feature/remove-objects-manual-mask

| Branch                               | Parent                                  | Goal                                          | Pass 1                | Pass 2                          | Main Fix                  | Mask Source        | Inpaint | Post Composite                  | Android            | Back                                                                                  |
| ------------------------------------ | --------------------------------------- | --------------------------------------------- | --------------------- | ------------------------------- | ------------------------- | ------------------ | ------- | ------------------------------- | ------------------ | ------------------------------------------------------------------------------------- |
| `feature/remove-objects-manual-mask` | `feature/remove-objects-sam2-flux-fill` | second-pass manual cleanup for Remove Objects | auto `remove_objects` | manual `remove_objects_cleanup` | inpaint only painted area | Android brush mask | ComfyUI | original outside mask preserved | Refine screen flow | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## 🌳 Tree

```text
mobile-assets-backend/
├── CMakeLists.txt
├── README.md
├── conanfile.txt
├── data/
│   ├── onboarding.json
│   ├── templates.json
│   └── tools.json
├── models/
│   └── local/
│       └── sam_vit_b_01ec64.pth
├── readme_assets/
│   └── remove_objects_manual/
│       ├── original.jpg
│       ├── auto_remove_umbrella.png
│       └── manual_cleanup.png
├── scripts/
│   ├── apply_inpaint_mask.py
│   ├── create_object_mask.py
│   ├── create_object_mask_sam.py
│   ├── prepare_manual_cleanup_mask.py
│   ├── remove_background.py
│   └── remove_objects.py
├── src/
│   ├── api_handler.cpp
│   ├── api_handler.h
│   ├── catalog_service.cpp
│   ├── catalog_service.h
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
    ├── remove_objects_cleanup_inpaint.json
    ├── remove_objects_inpaint.json
    ├── template_img2img.json
    └── tool_img2img.json
```

---

## ✅ Final working scheme

| Pass | Action                   | Input                   | Mask                           | Workflow                              | Post-processing         | Output                      |
| ---- | ------------------------ | ----------------------- | ------------------------------ | ------------------------------------- | ----------------------- | --------------------------- |
| 1    | `remove_objects`         | original uploaded image | SAM auto mask from text prompt | `remove_objects_inpaint.json`         | `apply_inpaint_mask.py` | first auto cleanup result   |
| 2    | `remove_objects_cleanup` | previous result image   | Android brush mask             | `remove_objects_cleanup_inpaint.json` | `apply_inpaint_mask.py` | final manual cleanup result |

---

## 🧠 Why this branch exists

| Problem                                    | Why auto was not enough                              | Manual cleanup fix                            |
| ------------------------------------------ | ---------------------------------------------------- | --------------------------------------------- |
| umbrella canopy removed but stick remained | SAM/GroundingDINO may miss thin parts                | user paints exact leftover                    |
| handle/top tip remained                    | small fragments are hard for text detection          | manual white mask marks only that area        |
| face/body could be damaged by full inpaint | ComfyUI returns full decoded image                   | post-composite restores original outside mask |
| object-specific rules are fragile          | umbrellas, wires, straps, glasses, spokes all differ | user mask is exact and general                |

---

## 🔁 Pass 1: Auto Remove Objects

| Item                | Value                                         |
| ------------------- | --------------------------------------------- |
| `serverAction`      | `remove_objects`                              |
| mask generation     | `scripts/create_object_mask_sam.py`           |
| inpaint workflow    | `workflows/remove_objects_inpaint.json`       |
| post-composite      | `scripts/apply_inpaint_mask.py`               |
| expected result     | main object area removed, face/body preserved |
| possible limitation | thin leftovers may remain                     |

```text
uploaded source image
→ create_object_mask_sam.py
→ remove_objects_inpaint.json
→ ComfyUI inpaint
→ apply_inpaint_mask.py
→ output
```

---

## 🖌️ Pass 2: Manual Cleanup

| Item             | Value                                           |
| ---------------- | ----------------------------------------------- |
| `serverAction`   | `remove_objects_cleanup`                        |
| source image     | previous Remove Objects result                  |
| mask image       | Android brush mask uploaded to `/uploads/...`   |
| mask preparation | `scripts/prepare_manual_cleanup_mask.py`        |
| inpaint workflow | `workflows/remove_objects_cleanup_inpaint.json` |
| post-composite   | `scripts/apply_inpaint_mask.py`                 |
| expected result  | only painted leftover area changes              |

```text
sourceImageUrl from previous result
+
options.maskImageUrl from Android brush mask
↓
prepare_manual_cleanup_mask.py
↓
ComfyUI inpaint
↓
apply_inpaint_mask.py
↓
final cleanup output
```

---

## 📤 Cleanup request contract

| Field                  | Value                                                         |
| ---------------------- | ------------------------------------------------------------- |
| `toolType`             | `REMOVE_OBJECTS`                                              |
| `serverAction`         | `remove_objects_cleanup`                                      |
| `sourceImageUrl`       | previous generated result image                               |
| `prompt`               | `remove selected leftovers, reconstruct background naturally` |
| `options.sourceTaskId` | previous Remove Objects task id                               |
| `options.maskImageUrl` | uploaded Android brush mask PNG                               |
| `options.mode`         | `manual_cleanup`                                              |
| `outputCount`          | `1`                                                           |

```json
{
  "toolType": "REMOVE_OBJECTS",
  "serverAction": "remove_objects_cleanup",
  "sourceImageUrl": "http://192.168.0.177:8080/outputs/final_pixo_remove_objects_....png",
  "prompt": "remove selected leftovers, reconstruct background naturally",
  "options": {
    "sourceTaskId": "previous_task_id",
    "maskImageUrl": "http://192.168.0.177:8080/uploads/mask_....png",
    "mode": "manual_cleanup"
  },
  "outputCount": 1
}
```

---

## 🎭 Android brush mask semantics

| Mask color               | Meaning          | Backend behavior                       |
| ------------------------ | ---------------- | -------------------------------------- |
| white / painted area     | remove / inpaint | area is sent to ComfyUI                |
| black / transparent area | keep             | original pixels are preserved          |
| same canvas as result    | required         | backend prepares Comfy-compatible mask |

---

## 🧩 New files

| File                                            | Purpose                                                      |
| ----------------------------------------------- | ------------------------------------------------------------ |
| `scripts/prepare_manual_cleanup_mask.py`        | converts Android brush mask to Comfy-compatible inpaint mask |
| `workflows/remove_objects_cleanup_inpaint.json` | ComfyUI workflow for manual cleanup inpaint                  |

---

## 🔧 Modified files

| File                                 | Purpose                                |
| ------------------------------------ | -------------------------------------- |
| `src/generation/generation_json.cpp` | helper changes for request parsing     |
| `src/generation/generation_json.h`   | helper declarations                    |
| `src/generation_service.cpp`         | adds `remove_objects_cleanup` flow     |
| `src/generation_service.h`           | declarations / fields for cleanup flow |

---

## 🖼️ Visual examples

| Stage                        | Image                                                             | Meaning                                                          |
| ---------------------------- | ----------------------------------------------------------------- | ---------------------------------------------------------------- |
| Original                     | ![](readme_assets/remove_objects_manual/original.jpg)             | source image before Remove Objects                               |
| Pass 1 — Auto Remove Objects | ![](readme_assets/remove_objects_manual/auto_remove_umbrella.png) | first pass removes main umbrella area by text prompt             |
| Pass 2 — Manual Cleanup      | ![](readme_assets/remove_objects_manual/manual_cleanup.png)       | second pass removes remaining selected leftovers with brush mask |

---

## 🧪 Expected logs

| Log                                             | Meaning                                         |
| ----------------------------------------------- | ----------------------------------------------- |
| `[REMOVE_OBJECTS_CLEANUP_MASK_PREPARE_START]`   | backend starts preparing Android brush mask     |
| `[REMOVE_OBJECTS_CLEANUP_COMFY_WORKFLOW_JSON]`  | cleanup workflow JSON prepared                  |
| `[COMFY_QUEUE_RESPONSE]`                        | ComfyUI accepted prompt                         |
| `[COMFY_HISTORY_OUTPUT]`                        | ComfyUI produced output                         |
| `[COMFY_DOWNLOAD_OK]`                           | backend downloaded image from ComfyUI           |
| `[REMOVE_OBJECTS_CLEANUP_POST_COMPOSITE_START]` | backend starts preserving original outside mask |

---

## ✅ Expected result

| Stage                   | Expected                          |
| ----------------------- | --------------------------------- |
| Auto pass               | removes main object               |
| Manual pass             | removes remaining small leftovers |
| Outside painted mask    | unchanged                         |
| Face/body               | unchanged                         |
| Background outside mask | unchanged                         |
| Extra generated objects | should not appear                 |
| Final output            | standard `/outputs/...` URL       |

---

## 🧪 Backend test

```bash
cd ~/mobile-assets-backend/build

PUBLIC_BASE_URL="http://192.168.0.177:8080" \
COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" \
./bin/mobile_assets_backend
```

```bash
RESPONSE=$(curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "toolType": "REMOVE_OBJECTS",
  "serverAction": "remove_objects_cleanup",
  "sourceImageUrl": "http://192.168.0.177:8080/outputs/final_pixo_remove_objects_mock_task_32144767943214_0_00001_.png",
  "prompt": "remove selected leftovers, reconstruct background naturally",
  "options": {
    "sourceTaskId": "mock_task_32144767943214",
    "maskImageUrl": "http://192.168.0.177:8080/uploads/img_41264888971654_c7ff908ee1865a2f.jpg",
    "mode": "manual_cleanup"
  },
  "outputCount": 1
}')

echo "$RESPONSE" | jq

TASK_ID=$(echo "$RESPONSE" | jq -r '.taskId')

watch -n 3 "curl -s http://localhost:8080/generations/$TASK_ID | jq"
```

---

## 🧾 Commit

```bash
cd ~/mobile-assets-backend

git add src/generation/generation_json.cpp
git add src/generation/generation_json.h
git add src/generation_service.cpp
git add src/generation_service.h
git add scripts/prepare_manual_cleanup_mask.py
git add workflows/remove_objects_cleanup_inpaint.json

git commit -m "Add manual mask cleanup for remove objects"

git push -u origin feature/remove-objects-manual-mask
```

---

## 🏁 Final status

| Capability                                  | Status |
| ------------------------------------------- | ------ |
| Auto Remove Objects first pass              | ✅      |
| Manual cleanup second pass                  | ✅      |
| Android brush mask supported                | ✅      |
| `remove_objects_cleanup` action added       | ✅      |
| ComfyUI cleanup inpaint workflow added      | ✅      |
| post-composite outside mask preserved       | ✅      |
| face/body/background protected outside mask | ✅      |
| final output uses `/outputs/...`            | ✅      |
| state can be considered successful          | ✅      |

---

## ⬅️ Назад

| Link        | URL                                                                    |
| ----------- | ---------------------------------------------------------------------- |
| Main README | https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md |
