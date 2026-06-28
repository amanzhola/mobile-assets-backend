# feature/change-scene-runner

| Branch                        | Previous                     | Next                                                                                | Status     | Main Goal                                           | Problem Before                                                                | Main Change                                                            | Added Runner        | Added Workflow                 | Added Script                         | Updated Prompt Layer       | Result                                        | Back                                                                                  |
| ----------------------------- | ---------------------------- | ----------------------------------------------------------------------------------- | ---------- | --------------------------------------------------- | ----------------------------------------------------------------------------- | ---------------------------------------------------------------------- | ------------------- | ------------------------------ | ------------------------------------ | -------------------------- | --------------------------------------------- | ------------------------------------------------------------------------------------- |
| `feature/change-scene-runner` | `feature/glam-makeup-runner` | `feature/hair-studio-runner` / `feature/ghostface-runner` / `feature/ghibli-runner` | ✅ Finished | вынести `change_scene` из общего `ToolActionRunner` | old inpaint pipeline давал серый фон, грязные края и плохую реакцию на prompt | новый pipeline: cutout subject → generate background → composite final | `ChangeSceneRunner` | `change_scene_background.json` | `composite_subject_on_background.py` | `BuildChangeScenePrompt()` | нормальный scene replacement без старого фона | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Final Architecture

| Action         | Runner              | Prompt Builder                            | Subject Cutout                     | Background Generation          | Final Composite                      | Output                              |
| -------------- | ------------------- | ----------------------------------------- | ---------------------------------- | ------------------------------ | ------------------------------------ | ----------------------------------- |
| `change_scene` | `ChangeSceneRunner` | `PromptBuilder::BuildChangeScenePrompt()` | `remove_background.py transparent` | ComfyUI text-to-image workflow | `composite_subject_on_background.py` | `/outputs/pixo_change_scene_...png` |

```text
change_scene
↓
ChangeSceneRunner
↓
PromptBuilder::BuildChangeScenePrompt
↓
remove_background.py transparent
↓
ComfyUI generates new background
↓
composite_subject_on_background.py
↓
final output
```

---

## What Was Wrong Before

| Old Approach                     | Problem                                      | Result                      |
| -------------------------------- | -------------------------------------------- | --------------------------- |
| original image + background mask | SDXL tried to inpaint huge background area   | weak scene change           |
| `VAEEncodeForInpaint`            | not good for full background replacement     | gray / muddy background     |
| old background mask              | edges around person stayed dirty             | outline around hair/body    |
| prompt like `sunset beach`       | SDXL reacted weakly                          | scene did not really change |
| ToolActionRunner route           | `change_scene` was treated like generic tool | no dedicated logic          |

---

## Correct Solution

| Step | What Happens                          | Why                         |
| ---- | ------------------------------------- | --------------------------- |
| 1    | cut person from original image        | keep user/person intact     |
| 2    | generate new background separately    | do not fight old background |
| 3    | composite subject over new background | clean final image           |
| 4    | save final output                     | standard backend result URL |

---

## Files Added

| Type         | File                                               | Purpose                                                 |
| ------------ | -------------------------------------------------- | ------------------------------------------------------- |
| new runner   | `src/action_runners/change_scene_runner.h`         | declaration for Change Scene runner                     |
| new runner   | `src/action_runners/change_scene_runner.cpp`       | full Change Scene execution pipeline                    |
| new workflow | `workflows/change_scene_background.json`           | text-to-image background-only Comfy workflow            |
| new script   | `scripts/scene/composite_subject_on_background.py` | composite transparent subject over generated background |

---

## Files Updated

| File                                          | Change                                                            |
| --------------------------------------------- | ----------------------------------------------------------------- |
| `src/prompt/prompt_builder.h`                 | added `ChangeScenePromptResult` and `BuildChangeScenePrompt(...)` |
| `src/prompt/prompt_builder.cpp`               | builds background-only positive prompt                            |
| `src/comfy/workflow_builder.h`                | added `BuildChangeSceneBackgroundWorkflow(...)`                   |
| `src/comfy/workflow_builder.cpp`              | loads and fills `change_scene_background.json`                    |
| `src/generation/generation_action_router.h`   | added `ChangeSceneRunner` dependency                              |
| `src/generation/generation_action_router.cpp` | routes `serverAction=change_scene`                                |
| `src/generation_service.h`                    | added `change_scene_runner_` member                               |
| `src/generation_service.cpp`                  | constructs runner and passes it to router                         |
| `CMakeLists.txt`                              | added `src/action_runners/change_scene_runner.cpp`                |

---

## Removed / Moved To Unused

| Old File                                        | New Place / Status                           | Reason                                        |
| ----------------------------------------------- | -------------------------------------------- | --------------------------------------------- |
| `scripts/scene/create_background_mask.py`       | `scripts/unused/scene/`                      | no longer needed                              |
| `scripts/scene/apply_change_scene_composite.py` | `scripts/unused/scene/`                      | replaced by subject-over-background composite |
| `workflows/change_scene_inpaint.json`           | `workflows/unused_change_scene_inpaint.json` | old inpaint pipeline rejected                 |

---

## New Workflow

| Workflow                                 | Type                          | Uses                                                                                                 | Does NOT Use          |
| ---------------------------------------- | ----------------------------- | ---------------------------------------------------------------------------------------------------- | --------------------- |
| `workflows/change_scene_background.json` | text-to-image background only | `CheckpointLoaderSimple`, `EmptyLatentImage`, `CLIPTextEncode`, `KSampler`, `VAEDecode`, `SaveImage` | `VAEEncodeForInpaint` |

Important:

```text
There must be no VAEEncodeForInpaint in change_scene_background.json.
```

---

## Prompt Logic

| Layer                      | Output                                                                            |
| -------------------------- | --------------------------------------------------------------------------------- |
| user prompt                | `пляж на закате` / `sunset beach`                                                 |
| translator                 | English scene text                                                                |
| `BuildChangeScenePrompt()` | background-only generation prompt                                                 |
| positive prompt            | realistic background scene only, natural photo background, scene: Beach at sunset |
| negative prompt            | person, people, human, face, body, silhouette, portrait, character                |

---

## Background Prompt

```text
realistic background scene only
no people
no person
no human
natural photo background
scene: Beach at sunset
```

---

## Negative Prompt

```text
person, people, human, face, body, silhouette, portrait, character
```

---

## ChangeSceneRunner Pipeline

| Step      | Log                                   | Description                      |
| --------- | ------------------------------------- | -------------------------------- |
| start     | `[CHANGE_SCENE_RUNNER_START]`         | runner started                   |
| prompt    | `[CHANGE_SCENE_PROMPT_BUILT]`         | prompt prepared                  |
| cutout    | `[CHANGE_SCENE_SUBJECT_CUTOUT_START]` | remove background transparent    |
| workflow  | `[CHANGE_SCENE_WORKFLOW_JSON]`        | background workflow built        |
| queue     | `[COMFY_QUEUE_RESPONSE]`              | ComfyUI accepted prompt          |
| history   | `[COMFY_HISTORY_OUTPUT]`              | output detected                  |
| download  | `[COMFY_DOWNLOAD_OK]`                 | background downloaded            |
| composite | `[CHANGE_SCENE_COMPOSITE_START]`      | subject composited on background |
| success   | `[CHANGE_SCENE_SUCCESS]`              | final output ready               |

---

## Final Expected Log

```text
workflow=workflows/change_scene_background.json

[CHANGE_SCENE_RUNNER_START]
[PROMPT_TRANSLATED]
[CHANGE_SCENE_PROMPT_BUILT]
[CHANGE_SCENE_SUBJECT_CUTOUT_START]
[CHANGE_SCENE_WORKFLOW_JSON]
EmptyLatentImage
[COMFY_QUEUE_RESPONSE]
[COMFY_HISTORY_OUTPUT]
[COMFY_DOWNLOAD_OK]
[CHANGE_SCENE_COMPOSITE_START]
[CHANGE_SCENE_SUCCESS]
```

---

## Bad Logs

| Bad Log                                | Meaning                                       |
| -------------------------------------- | --------------------------------------------- |
| `VAEEncodeForInpaint`                  | old inpaint workflow is still used            |
| `workflow=workflows/tool_img2img.json` | request still goes through `ToolActionRunner` |
| `change_scene_inpaint.json`            | old workflow not removed from route           |
| gray background                        | old inpaint flow or weak background workflow  |
| old outline around person              | subject cutout/composite not used correctly   |

---

## Screenshots / README Assets

| Preview             | Path                                                 |
| ------------------- | ---------------------------------------------------- |
| Input               | `readme_assets/change_scene/input.jpg`               |
| Output sunset beach | `readme_assets/change_scene/output_sunset_beach.png` |

```md
## Visual Result

| Input | Output: Sunset Beach |
|---|---|
| ![](readme_assets/change_scene/input.jpg) | ![](readme_assets/change_scene/output_sunset_beach.png) |
```

---

## README Assets Tree

```text
readme_assets/
├── ai_enhancer/
│   ├── input.jpg
│   ├── output_1.png
│   └── output_2.png
├── change_scene/
│   ├── input.jpg
│   └── output_sunset_beach.png
├── glam_makeup/
│   ├── input.jpg
│   └── output_soft_pink_blush.png
├── remove_background/
│   ├── input.jpg
│   └── output.png
├── remove_objects/
│   ├── input.jpg
│   ├── output_auto.png
│   └── output_manual.png
├── skin_clean/
│   ├── input.jpg
│   └── output.png
└── smile/
    ├── input.jpg
    └── output.png
```

---

## Visual Result

| Input                                     | Output: Sunset Beach                                    |
| ----------------------------------------- | ------------------------------------------------------- |
| ![](readme_assets/change_scene/input.jpg) | ![](readme_assets/change_scene/output_sunset_beach.png) |

---

## ToolActionRunner Cleanup

| Removed From `IsToolAction` | Reason                         |
| --------------------------- | ------------------------------ |
| `change_scene`              | now has `ChangeSceneRunner`    |
| `glam_makeup`               | already has `GlamMakeupRunner` |
| `remove_background`         | has `RemoveBackgroundRunner`   |
| `skin_improve`              | has `SkinImproveRunner`        |
| `upscale_image`             | has `UpscaleRunner`            |
| `smile_edit`                | has `SmileEditRunner`          |

Remaining temporary generic tools:

```text
ghibli
ghostface
hair_studio
```

---

## Validation Commands

| Check          | Command                                                                   | Expected                |                          |
| -------------- | ------------------------------------------------------------------------- | ----------------------- | ------------------------ |
| references     | `grep -R "change_scene" -n src                                            | head -50`               | runner/router references |
| builder        | `grep -R "BuildChangeSceneBackgroundWorkflow" -n src`                     | method exists           |                          |
| workflow       | `grep -R "change_scene_background" -n src workflows`                      | route uses new workflow |                          |
| no old inpaint | `grep -R "VAEEncodeForInpaint" -n workflows/change_scene_background.json` | no output               |                          |
| build          | `cmake --build . --clean-first`                                           | success                 |                          |

---

## Build

```bash
cd ~/mobile-assets-backend/build
cmake --build . --clean-first
```

---

## Run

```bash
PUBLIC_BASE_URL="http://192.168.0.177:8080" \
COMFY_BASE_URL="https://YOUR_COMFY.trycloudflare.com" \
PROMPT_TRANSLATOR_BASE_URL="https://YOUR_TRANSLATOR.trycloudflare.com" \
./bin/mobile_assets_backend
```

---

## Test Request

```bash
RESPONSE=$(curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "serverAction": "change_scene",
  "toolType": "CHANGE_SCENE",
  "sourceImageUrl": "http://192.168.0.177:8080/uploads/YOUR_TEST_IMAGE.jpg",
  "prompt": "Beach at sunset",
  "outputCount": 1
}')

echo "$RESPONSE" | jq
TASK_ID=$(echo "$RESPONSE" | jq -r ".taskId")
watch -n 3 "curl -s http://localhost:8080/generations/$TASK_ID | jq"
```

---

## Expected Result

| Field                | Value                               |
| -------------------- | ----------------------------------- |
| `status`             | `completed`                         |
| `progressPercent`    | `100`                               |
| `resultImageUrls[0]` | `/outputs/pixo_change_scene_...png` |
| person               | preserved from original             |
| background           | newly generated                     |
| old background       | removed                             |
| gray background      | absent                              |
| dirty old outline    | reduced/removed                     |

---

## Final State

| Item                                                 | Status                                   |
| ---------------------------------------------------- | ---------------------------------------- |
| `ChangeSceneRunner`                                  | added                                    |
| `change_scene` removed from generic ToolActionRunner | ✅                                        |
| old inpaint workflow                                 | removed/unused                           |
| new background-only workflow                         | ✅                                        |
| subject cutout                                       | via existing `remove_background.py`      |
| final composite                                      | via `composite_subject_on_background.py` |
| PromptBuilder support                                | ✅                                        |
| WorkflowBuilder support                              | ✅                                        |
| README assets                                        | ✅                                        |
| next candidates                                      | `hair_studio`, `ghostface`, `ghibli`     |

---

## Commit

```bash
git add .
git commit -m "Extract change scene runner"
git push -u origin feature/change-scene-runner
```

---

## Back

| Link                                                                                  |
| ------------------------------------------------------------------------------------- |
| [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |
