# feature/glam-makeup-runner

| Branch                       | Previous                    | Next                      | Status                                                                | Commit                              | Main Goal                                                                                         | Core Components                                                                                                                                  | Result                                                                         | Back                                                                                  |
| ---------------------------- | --------------------------- | ------------------------- | --------------------------------------------------------------------- | ----------------------------------- | ------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------- |
| `feature/glam-makeup-runner` | `feature/smile-edit-runner` | `feature/prompt-platform` | ✅ Finished · ✅ Compiles · ✅ Pushed · ✅ Production · ✅ Continued later | `Extract glam makeup action runner` | Convert Glam Makeup from a generic SDXL prompt into a dedicated intelligent face editing pipeline | `PromptTranslator` · `PromptBuilder` · `PromptNormalizer` · `PromptTemplates` · `FaceParser` · `FaceEditPlan` · `FaceMasks` · `GlamMakeupRunner` | Independent Glam Makeup architecture with local face editing and SDXL fallback | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

# Architecture Evolution

| Before                                                     | After                                                                                                                                  |
| ---------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------- |
| `GenerationService → ToolActionRunner → tool_img2img.json` | `GenerationService → GenerationActionRouter → GlamMakeupRunner → Prompt Platform → Face Edit Platform → Local Overlay / SDXL Fallback` |

```text
GenerationService
        │
        ▼
GenerationActionRouter
        │
        ▼
GlamMakeupRunner
        │
        ▼
PromptTranslator
        │
        ▼
PromptBuilder
        │
        ▼
FaceParser
        │
        ▼
FaceEditPlan
        │
        ▼
FaceMasks
        │
        ▼
Local Overlay
        │
        ├── Success → Output
        └── Unsupported → SDXL Fallback
```

---

# New Modules

| Module             | Files                                     | Responsibility                                  |
| ------------------ | ----------------------------------------- | ----------------------------------------------- |
| Prompt Platform    | `src/prompt/*`                            | translation, normalization, prompt construction |
| Face Edit Platform | `src/face_edit/*`                         | face regions, parsing, edit planning, masks     |
| Runner             | `src/action_runners/glam_makeup_runner.*` | execute Glam Makeup pipeline                    |

---

# Prompt Platform

| Component        | Responsibility      |
| ---------------- | ------------------- |
| PromptTranslator | RU → EN translation |
| PromptNormalizer | normalize text      |
| PromptTemplates  | style presets       |
| PromptBuilder    | build final prompt  |

---

# Translation Pipeline

| User Input           | Translation     |
| -------------------- | --------------- |
| нежно-розовые румяна | soft pink blush |
| зелёные губы         | green lips      |
| золотые тени         | gold eyeshadow  |
| зелёные брови        | green eyebrows  |

Pipeline

```text
User
↓
PromptTranslator
↓
FastAPI
↓
Qwen2.5-1.5B
↓
English prompt
```

---

# Prompt Builder

| Input           | Builder Produces      |
| --------------- | --------------------- |
| Style preset    | prompt                |
| translated text | prompt                |
| face edit plan  | prompt                |
| negative prompt | final workflow prompt |

---

# Presets

| Preset       | Example                                        |
| ------------ | ---------------------------------------------- |
| Natural Glow | natural glow makeup · subtle blush · nude lips |
| Rich Glam    | contour · strong lashes · satin lipstick       |
| Evening      | evening makeup · smoky eyes · refined lipstick |

---

# Face Edit Platform

| Component    | Responsibility                   |
| ------------ | -------------------------------- |
| FaceRegions  | supported regions                |
| FaceParser   | detect target area               |
| FaceEditPlan | structured edit plan             |
| FaceMasks    | generate editable region         |
| FaceEditor   | future local editing entry point |

---

# Supported Regions

| Region  | Supported |
| ------- | --------- |
| Lips    | ✅         |
| Cheeks  | ✅         |
| Eyes    | ✅         |
| Eyelids | ✅         |
| Skin    | ✅         |
| Hair    | planned   |

---

# Face Parser

| Prompt          | Parsed Region |
| --------------- | ------------- |
| soft pink blush | cheeks        |
| blue lips       | lips          |
| gold eyeshadow  | eyelids       |
| green eyebrows  | eyebrows      |

---

# Local Makeup Pipeline

```text
Prompt
↓
Translate
↓
Normalize
↓
Parse Region
↓
Create Mask
↓
Apply Overlay
↓
Output
```

---

# Why Masks Were Needed

| SDXL Only              | Local Masks                |
| ---------------------- | -------------------------- |
| may repaint whole face | edits only selected region |
| unstable lips          | lips only                  |
| may alter nose/hair    | face preserved             |
| inconsistent           | deterministic              |

---

# MediaPipe Issue

| Problem                       | Solution                         |
| ----------------------------- | -------------------------------- |
| `mediapipe.solutions` missing | support new MediaPipe API        |
| mask creation failed          | OpenCV fallback                  |
| pipeline stopped              | always generate approximate mask |

---

# Overlay Pipeline

Old

```text
Mask
↓
Remove Objects Inpaint
↓
SDXL repaint
```

New

```text
Mask
↓
Local Overlay
↓
Output
```

---

# Overlay Examples

| Region   | Local Effect             |
| -------- | ------------------------ |
| cheeks   | soft pink blush          |
| lips     | blue / green / nude tint |
| eyelids  | eyeshadow                |
| eyebrows | local coloring           |

---

# Runner Logic

| Step | Action                         |
| ---- | ------------------------------ |
| 1    | translate prompt               |
| 2    | normalize                      |
| 3    | detect region                  |
| 4    | build edit plan                |
| 5    | create mask                    |
| 6    | apply overlay                  |
| 7    | if unsupported → SDXL fallback |

---

# Upload Fix

| Problem                                                  | Fix                                                            |
| -------------------------------------------------------- | -------------------------------------------------------------- |
| UploadImage returned false although file already existed | ignore upload failure if image already copied into Comfy input |

---

# Files Added

| Directory            | Files                                                                                      |
| -------------------- | ------------------------------------------------------------------------------------------ |
| `src/action_runners` | `glam_makeup_runner.*`                                                                     |
| `src/prompt`         | `prompt_builder.*` · `prompt_normalizer.*` · `prompt_templates.*` · `prompt_translator.*`  |
| `src/face_edit`      | `face_regions.*` · `face_parser.*` · `face_masks.*` · `face_editor.*` · `face_edit_plan.*` |

---

# GenerationService After Refactor

| Before                      | After                  |
| --------------------------- | ---------------------- |
| contained Glam Makeup logic | only dispatches action |

---

# Architecture After Branch

```text
GenerationService
        │
        ▼
GenerationActionRouter
        ├── AiEnhancerRunner
        ├── RemoveBackgroundRunner
        ├── RemoveObjectsRunner
        ├── RemoveObjectsCleanupRunner
        ├── TemplateRunner
        ├── PromptRunner
        ├── UpscaleRunner
        ├── SkinImproveRunner
        ├── SmileEditRunner
        ├── GlamMakeupRunner
        └── ToolActionRunner (temporary)
```

---

# Achievements

| Achievement                                                     | Status |
| --------------------------------------------------------------- | ------ |
| Dedicated GlamMakeupRunner                                      | ✅      |
| Prompt Platform introduced                                      | ✅      |
| Face Edit Platform introduced                                   | ✅      |
| RU→EN translation                                               | ✅      |
| Prompt normalization                                            | ✅      |
| Face region parsing                                             | ✅      |
| Structured edit plan                                            | ✅      |
| Local overlay pipeline                                          | ✅      |
| SDXL fallback                                                   | ✅      |
| GenerationService simplified                                    | ✅      |
| Foundation for Hair Studio / Ghostface / Ghibli / Scene editing | ✅      |

---

# Next Branch

| Branch                    | Purpose                                                                                                                                                                  |
| ------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `feature/prompt-platform` | extend Prompt Platform into a reusable backend subsystem shared by Glam Makeup, Prompt generation, Hair Studio, Ghostface, Ghibli, Templates and future AI editing tools |

---

# Back

| Link                                                                                  |
| ------------------------------------------------------------------------------------- |
| [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |
