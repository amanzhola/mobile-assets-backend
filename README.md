# feature/background-progress-cleanup

| Branch                                | Previous                           | Next                                | Main Goal                                            | Problem Before                                                             | Main Change                                                                | Updated Runner           | Updated Router           | Updated Core    | Build File        | Result                                                      | Back                                                                                  |
| ------------------------------------- | ---------------------------------- | ----------------------------------- | ---------------------------------------------------- | -------------------------------------------------------------------------- | -------------------------------------------------------------------------- | ------------------------ | ------------------------ | --------------- | ----------------- | ----------------------------------------------------------- | ------------------------------------------------------------------------------------- |
| `feature/background-progress-cleanup` | `feature/generation-action-router` | `feature/template-workflow-cleanup` | почистить progress/status flow для Remove Background | background removal мог завершаться без аккуратного progress/state handling | обновлён `RemoveBackgroundRunner` и routing через `GenerationActionRouter` | `RemoveBackgroundRunner` | `GenerationActionRouter` | generation flow | unchanged / minor | Remove Background стал чище возвращать результат и прогресс | [Main README](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

---

## Architecture

| Layer                    | До этой ветки                                                        | После этой ветки                                 | Что стало лучше               | Почему это важно                                         |
| ------------------------ | -------------------------------------------------------------------- | ------------------------------------------------ | ----------------------------- | -------------------------------------------------------- |
| `RemoveBackgroundRunner` | выполнял local background removal, но progress/state были неидеальны | аккуратнее возвращает result/progress            | меньше ошибок статуса         | Android Result Screen получает стабильный completed flow |
| `GenerationActionRouter` | уже маршрутизировал actions                                          | учитывает обновлённый background runner behavior | routing сохранился чистым     | remove_background не ломает общий flow                   |
| `GenerationService`      | task lifecycle уже отделялся от runners                              | получает более стабильный result от runner       | меньше special-case логики    | service не должен знать детали rembg                     |
| Android                  | ожидал стандартный task result                                       | получает обычный completed result                | поведение единообразное       | UI не требует отдельной логики                           |
| Local scripts            | `remove_background.py` делает обработку                              | runner корректно оборачивает результат           | local script остаётся простым | backend отвечает за task semantics                       |

---

## Files

| Type    | File                                              | Action   | Purpose                                   | Notes                         |
| ------- | ------------------------------------------------- | -------- | ----------------------------------------- | ----------------------------- |
| updated | `src/action_runners/remove_background_runner.h`   | modified | clarify remove background runner API      | local background action       |
| updated | `src/action_runners/remove_background_runner.cpp` | modified | cleanup progress/result handling          | white/transparent output flow |
| updated | `src/generation/generation_action_router.cpp`     | modified | keep/remove_background routing consistent | router-level integration      |

---

## Remove Background Flow

| Mode          | Input          | Local Script                              | Output                        | Result URL                               |
| ------------- | -------------- | ----------------------------------------- | ----------------------------- | ---------------------------------------- |
| `white`       | uploaded image | `scripts/background/remove_background.py` | PNG/RGB with white background | `/outputs/pixo_remove_background_...png` |
| `transparent` | uploaded image | `scripts/background/remove_background.py` | PNG/RGBA with alpha           | `/outputs/pixo_remove_background_...png` |

```text
source image
↓
GenerationActionRouter
↓
RemoveBackgroundRunner
↓
scripts/background/remove_background.py
↓
storage/output
↓
completed task
```

---

## Progress Cleanup

| Stage             | Before                                          | After                                         | Why It Matters            |
| ----------------- | ----------------------------------------------- | --------------------------------------------- | ------------------------- |
| task starts       | status could be less clear                      | status becomes processing through normal flow | Android can show progress |
| local script runs | result handling was tool-specific               | runner returns normal output URL              | unified result handling   |
| output saved      | task completion could need special handling     | task reaches completed with result URL        | standard API behavior     |
| error case        | failures could be noisy or unclear              | runner can return failure cleanly             | easier debugging          |
| transparent mode  | could be confused with white/background preview | treated as alpha output                       | correct product behavior  |

---

## Responsibility Split

| Component                | Responsibility                               | Should NOT do                  |
| ------------------------ | -------------------------------------------- | ------------------------------ |
| `GenerationService`      | own task lifecycle and persistence           | run `rembg` directly           |
| `GenerationActionRouter` | route `remove_background` to runner          | implement image processing     |
| `RemoveBackgroundRunner` | run background removal and return output URL | handle unrelated actions       |
| `remove_background.py`   | remove background and save requested mode    | know backend task status       |
| `OutputService`          | create/serve output paths                    | know background mode semantics |

---

## What This Branch Solves

| Problem                                                   | Fix                                      | Result                     |
| --------------------------------------------------------- | ---------------------------------------- | -------------------------- |
| Remove Background needed cleaner result/progress behavior | cleanup inside runner/router integration | more stable completed task |
| local script behavior was separate from task semantics    | runner wraps it consistently             | API stays standard         |
| white/transparent modes needed stable handling            | runner keeps both modes explicit         | correct outputs            |
| router already existed but background flow needed polish  | router integration updated               | clean action dispatch      |
| future local actions need same pattern                    | background runner becomes reference      | easier runner cleanup      |

---

## Validation

| Check                            | Expected                                          |
| -------------------------------- | ------------------------------------------------- |
| build                            | `cmake --build .` passes                          |
| `serverAction=remove_background` | routes through `GenerationActionRouter`           |
| white mode                       | returns white background output                   |
| transparent mode                 | returns alpha PNG output                          |
| task status                      | reaches `completed`                               |
| progress                         | reaches `100`                                     |
| resultImageUrls                  | contains `/outputs/pixo_remove_background_...png` |
| other actions                    | unchanged                                         |

---

## Test Requests

| Test                   | Request                                |
| ---------------------- | -------------------------------------- |
| White background       | `options.backgroundType = white`       |
| Transparent background | `options.backgroundType = transparent` |

```bash
RESPONSE=$(curl -s -X POST http://localhost:8080/generations \
-H "Content-Type: application/json" \
-d '{
  "serverAction": "remove_background",
  "toolType": "REMOVE_BACKGROUND",
  "sourceImageUrl": "http://192.168.0.177:8080/uploads/YOUR_TEST_IMAGE.jpg",
  "options": {
    "backgroundType": "transparent"
  },
  "outputCount": 1
}')

echo "$RESPONSE" | jq

TASK_ID=$(echo "$RESPONSE" | jq -r ".taskId")

watch -n 3 "curl -s http://localhost:8080/generations/$TASK_ID | jq"
```

---

## Expected Result

| Field                | Value                                                       |
| -------------------- | ----------------------------------------------------------- |
| `status`             | `completed`                                                 |
| `progressPercent`    | `100`                                                       |
| `resultImageUrls[0]` | `/outputs/pixo_remove_background_...png` or full public URL |
| output file          | exists in `storage/output`                                  |
| transparent output   | PNG with alpha                                              |
| white output         | subject composited onto white background                    |

---

## Commands

| Step     | Command                                                                                                                         |
| -------- | ------------------------------------------------------------------------------------------------------------------------------- |
| checkout | `git checkout feature/background-progress-cleanup`                                                                              |
| build    | `cd ~/mobile-assets-backend/build && cmake --build .`                                                                           |
| run      | `PUBLIC_BASE_URL="http://192.168.0.177:8080" COMFY_BASE_URL="https://YOUR-COMFY.trycloudflare.com" ./bin/mobile_assets_backend` |
| commit   | `git add . && git commit -m "Clean background removal progress handling"`                                                       |
| push     | `git push -u origin feature/background-progress-cleanup`                                                                        |

---

## Final State

| Item                                        | Status                              |
| ------------------------------------------- | ----------------------------------- |
| `RemoveBackgroundRunner`                    | cleaned                             |
| `GenerationActionRouter` background routing | updated                             |
| white background mode                       | preserved                           |
| transparent alpha mode                      | preserved                           |
| task completion                             | stable                              |
| progress result                             | stable                              |
| API behavior                                | unchanged                           |
| next branch                                 | `feature/template-workflow-cleanup` |

---

## Back

| Link        | URL                                                                                                                                              |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------ |
| Main README | [https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md](https://github.com/amanzhola/mobile-assets-backend/blob/main/README.md) |

