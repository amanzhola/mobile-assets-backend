# feature/smile-edit-runner

| Branch                      | Previous                      | Next                         | Status                                                         | Commit                             | Main Goal                                     | Problem Before                                   | Main Change                                            | Added Runner      | Updated Router           | Updated Core           | Workflow                       | Result                                           | Back                                                                                  |
| --------------------------- | ----------------------------- | ---------------------------- | -------------------------------------------------------------- | ---------------------------------- | --------------------------------------------- | ------------------------------------------------ | ------------------------------------------------------ | ----------------- | ------------------------ | ---------------------- | ------------------------------ | ------------------------------------------------ | ------------------------------------------------------------------------------------- |
| `feature/smile-edit-runner` | `feature/skin-improve-runner` | `feature/glam-makeup-runner` | ✅ Finished / ✅ Compiles / ✅ Pushed / ✅ Production architecture | `Extract smile edit action runner` | extract Smile Edit into its own Action Runner | Smile Edit lived inside generic ToolActionRunner | dedicated `SmileEditRunner` using AdvancedLivePortrait | `SmileEditRunner` | `GenerationActionRouter` | `generation_service.*` | `smile_edit_liveportrait.json` | Smile Edit became an independent action pipeline | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

# Goal

| Before                                                     | After                                                                                 | Result                                            |
| ---------------------------------------------------------- | ------------------------------------------------------------------------------------- | ------------------------------------------------- |
| `GenerationService → ToolActionRunner → tool_img2img.json` | `GenerationService → GenerationActionRouter → SmileEditRunner → AdvancedLivePortrait` | Smile Edit became a dedicated production pipeline |

---

# Development History

| Stage   | Solution                              | Result                                         | Decision   |
| ------- | ------------------------------------- | ---------------------------------------------- | ---------- |
| Stage 1 | Local OpenCV smile editing            | changed lips/nose instead of facial expression | ❌ rejected |
| Stage 2 | InsightFace                           | identity + landmarks only                      | ❌ rejected |
| Stage 3 | AdvancedLivePortrait ExpressionEditor | realistic smile editing                        | ✅ selected |

---

# Files Added

| Type | File                                       | Purpose             |
| ---- | ------------------------------------------ | ------------------- |
| new  | `src/action_runners/smile_edit_runner.h`   | runner declaration  |
| new  | `src/action_runners/smile_edit_runner.cpp` | Smile Edit pipeline |

---

# Files Updated

| File                                          | Purpose                            |
| --------------------------------------------- | ---------------------------------- |
| `src/generation_service.h`                    | inject runner                      |
| `src/generation_service.cpp`                  | create runner / pass to router     |
| `src/generation/generation_action_router.h`   | add SmileEditRunner dependency     |
| `src/generation/generation_action_router.cpp` | dispatch `serverAction=smile_edit` |
| `src/comfy/workflow_builder.h`                | add Smile workflow builder         |
| `src/comfy/workflow_builder.cpp`              | implement workflow                 |
| `src/comfy/comfy_client.h`                    | support image download by type     |
| `src/comfy/comfy_client.cpp`                  | implement `DownloadImageByType()`  |
| `CMakeLists.txt`                              | add runner                         |

---

# Architecture

| Layer             | Before                | After                   |
| ----------------- | --------------------- | ----------------------- |
| GenerationService | contained Smile logic | coordinator only        |
| Router            | no Smile runner       | dispatches Smile action |
| ToolActionRunner  | executed Smile        | no longer responsible   |
| SmileEditRunner   | did not exist         | owns complete pipeline  |

```text
GenerationService
↓
GenerationActionRouter
↓
SmileEditRunner
↓
AdvancedLivePortrait
↓
ExpressionEditor
↓
SaveImage
↓
Download
↓
OutputService
```

---

# Runner Responsibilities

| Responsibility      |
| ------------------- |
| read request        |
| resolve input image |
| convert smile level |
| build workflow      |
| upload image        |
| queue prompt        |
| wait completion     |
| download output     |
| save output         |
| return public URL   |

---

# Workflow

| Step             | Component             |
| ---------------- | --------------------- |
| LoadImage        | input image           |
| ExpressionEditor | apply smile parameter |
| SaveImage        | generate preview      |

Workflow file:

```text
workflows/smile_edit_liveportrait.json
```

Builder:

```cpp
BuildSmileEditWorkflow(...)
```

---

# Smile Level Mapping

| Android | ExpressionEditor |
| ------: | ---------------: |
|       0 |             0.00 |
|       1 |             0.35 |
|       2 |             0.60 |
|       3 |             0.95 |
|       4 |             1.20 |

---

# Why AdvancedLivePortrait

| Feature           | Supported |
| ----------------- | --------- |
| facial muscles    | ✅         |
| lips              | ✅         |
| cheeks            | ✅         |
| smile angle       | ✅         |
| preserve identity | ✅         |

---

# Problems Solved

## Problem 1

| Issue                                | Fix                                        |
| ------------------------------------ | ------------------------------------------ |
| backend searched `pixo_smile_edit_*` | fallback to `expression_edit_preview*.png` |

---

## Problem 2

| Issue                    | Fix                                      |
| ------------------------ | ---------------------------------------- |
| preview stored in `temp` | added `DownloadImageByType(output/temp)` |

---

## Problem 3

| Issue                    | Fix                                 |
| ------------------------ | ----------------------------------- |
| `fs::path` compile error | use `std::filesystem::path` / alias |

---

## Problem 4

| Issue                    | Fix                                  |
| ------------------------ | ------------------------------------ |
| const qualifier mismatch | both download methods became `const` |

---

# Logs

| Log                                       |
| ----------------------------------------- |
| `[SMILE_EDIT_LIVEPORTRAIT_START]`         |
| `[SMILE_EDIT_LIVEPORTRAIT_WORKFLOW_JSON]` |
| `[SMILE_EDIT_SUCCESS]`                    |
| `[SMILE_EDIT_EXCEPTION]`                  |

---

# Runner Knows

| Knows            |
| ---------------- |
| LivePortrait     |
| ExpressionEditor |
| workflow         |
| queue            |
| download         |
| output           |
| progress         |
| logs             |

---

# Runner Does NOT Know

| Does NOT Know     |
| ----------------- |
| HTTP              |
| API               |
| Catalog           |
| Task storage      |
| GenerationService |

---

# GenerationService Now Knows

```text
Action
↓
GenerationActionRouter
↓
SmileEditRunner
```

Nothing more.

---

# API Compatibility

| Item            | Status       |
| --------------- | ------------ |
| Android request | unchanged    |
| response        | unchanged    |
| serverAction    | `smile_edit` |
| frontend        | unchanged    |

---

# Result

| Before                     | After                          |
| -------------------------- | ------------------------------ |
| generic Tool Action        | dedicated Smile pipeline       |
| generic SDXL tool workflow | AdvancedLivePortrait           |
| shared execution           | independent runner             |
| no preview handling        | preview fallback implemented   |
| output-only download       | output/temp download supported |

---

# Validation

| Check            | Expected                       |
| ---------------- | ------------------------------ |
| build            | passes                         |
| workflow         | `smile_edit_liveportrait.json` |
| preview download | works                          |
| output URL       | returned                       |
| router           | dispatches Smile runner        |
| frontend         | unchanged                      |

---

# Architecture After Branch

```text
GenerationService
↓
GenerationActionRouter
├── RemoveBackgroundRunner
├── RemoveObjectsRunner
├── RemoveObjectsCleanupRunner
├── TemplateRunner
├── PromptRunner
├── AiEnhancerRunner
├── SkinImproveRunner
├── UpscaleRunner
├── SmileEditRunner
└── ToolActionRunner (temporary)
```

---

# What This Branch Achieved

| Achievement                           | Status |
| ------------------------------------- | ------ |
| Smile extracted from ToolActionRunner | ✅      |
| dedicated SmileEditRunner             | ✅      |
| AdvancedLivePortrait integration      | ✅      |
| ExpressionEditor support              | ✅      |
| preview fallback                      | ✅      |
| temp image download                   | ✅      |
| GenerationService simplified          | ✅      |
| production architecture               | ✅      |

---

# Commands

| Step     | Command                                                              |
| -------- | -------------------------------------------------------------------- |
| checkout | `git checkout feature/smile-edit-runner`                             |
| build    | `cd build && cmake --build .`                                        |
| run      | `PUBLIC_BASE_URL=... COMFY_BASE_URL=... ./bin/mobile_assets_backend` |
| commit   | `git commit -m "Extract smile edit action runner"`                   |
| push     | `git push -u origin feature/smile-edit-runner`                       |

---

# Back

| Link                                                                                  |
| ------------------------------------------------------------------------------------- |
| [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |
