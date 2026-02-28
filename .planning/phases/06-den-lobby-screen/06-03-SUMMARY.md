---
phase: 06-den-lobby-screen
plan: "03"
subsystem: ui-rendering
tags: [lobby, bg-layer, window, sprite, silhouette, palette]

dependency-graph:
  requires:
    - 06-02  # CB2_DenLobbyScreen scaffold with EWRAM LobbyState
  provides:
    - Orange left-panel background fill
    - Raid boss front-sprite silhouette at (60,72)
    - Star rating text above silhouette
    - Task chain: FadeIn → RenderLeft → LobbyMain
  affects:
    - 06-04  # Right panel text/menu builds on same BG0 window 1
    - 06-05  # Menu input wires into Task_LobbyMain

tech-stack:
  added: []
  patterns:
    - Multi-state task using gTasks[taskId].data[0] as state machine
    - Window-based BG fill (FillWindowPixelBuffer + PutWindowTilemap)
    - HandleLoadSpecialPokePic async DMA poll with IsDma3ManagerBusyWithBgCopy
    - Silhouette via FillPalette RGB_BLACK over entire OBJ palette (skip index 0)

key-files:
  created: []
  modified:
    - src/raid_den.c

decisions:
  - id: star-char-placeholder
    summary: "CHAR_EXCL_MARK (0xAB, '!') used as star placeholder — ★ and * have no GBA font mapping in charmap.txt"
  - id: palette-offset
    summary: "FillPalette uses BG_PLTT_ID(n) + 1 (not BG_PLTT_ID(n)) for color index 1; PIXEL_FILL(1) matches"
  - id: tasks-committed-together
    summary: "Task 1 and Task 2 committed in single feat commit — forward decl without definition broke incremental build"

metrics:
  duration: ~26 minutes
  completed: 2026-02-28
---

# Phase 6 Plan 03: Lobby Left Panel Render Summary

**One-liner:** BG window two-tone fill + async boss silhouette load + star count via CHAR_EXCL_MARK repeats

## What Was Built

### Task 1 — Orange Background Fill and BG Init

Added to `CB2_DenLobbyScreen` (after `FreeAllSpritePalettes`):

- `sLobbyBgTemplates[]`: BG 0 (charBase=0, mapBase=31, priority=1) + BG 1 stub (charBase=2, mapBase=29, priority=2) for future plans
- `sLobbyWindowTemplates[]`: Window 0 = left half (15×20 tiles, palette 1, baseBlock 1), Window 1 = right half (15×20, palette 2, baseBlock 301)
- Orange fill: `FillPalette(RGB(31,16,0), BG_PLTT_ID(1)+1, PLTT_SIZEOF(1))` + `FillWindowPixelBuffer(0, PIXEL_FILL(1))`
- Off-white fill: `FillPalette(RGB(28,24,20), BG_PLTT_ID(2)+1, PLTT_SIZEOF(1))` + `FillWindowPixelBuffer(1, PIXEL_FILL(1))`
- `Task_LobbyFadeIn` now chains to `Task_LobbyRenderLeft` instead of `Task_LobbyMain`

### Task 2 — Boss Silhouette Sprite and Star Rating Text

`Task_LobbyRenderLeft` is a 3-state task:

- **State 0:** `AllocateMonSpritesGfx()` + `HandleLoadSpecialPokePic(TRUE, ...)` with personality=0 for consistent silhouette
- **State 1:** Poll `IsDma3ManagerBusyWithBgCopy()`; when clear, load palette, set sprite template, create sprite at (60,72), fill OBJ palette with `RGB_BLACK` (indices 1–15)
- **State 2:** Build star text buffer (CHAR_EXCL_MARK × starRating, max 5), print via `AddTextPrinterParameterized4`, flush window, chain to `Task_LobbyMain`

## Deviations from Plan

### Auto-fixed Issues

**[Rule 1 - Bug] `_("*")` has no charmap entry (U+2A rejected by preproc)**

- **Found during:** Task 2 first build
- **Issue:** pokeemerald's `preproc` tool has no mapping for `*` (U+002A) in charmap.txt; same for `★`
- **Fix:** Use raw byte `0xAB` (`CHAR_EXCL_MARK`, `!`) directly — no `_()` macro needed
- **Files modified:** `src/raid_den.c`

**[Rule 3 - Blocking] Tasks 1 and 2 co-depend for clean build**

- **Found during:** First build after Task 1
- **Issue:** Forward declaration `static void Task_LobbyRenderLeft` without definition triggers `-Werror` "used but never defined"
- **Fix:** Implemented Task 2 before attempting per-task commit; committed both in single `feat(06-03)` commit

**[Rule 1 - Bug] FillPalette offset correction**

- **Found during:** Code review of palette APIs
- **Issue:** Plan code used `BG_PLTT_ID(1)` (color index 0 = transparent) but `PIXEL_FILL(1)` expects color index 1
- **Fix:** Used `BG_PLTT_ID(1) + 1` and `BG_PLTT_ID(2) + 1` so the fill color maps to PIXEL_FILL(1)

## Decisions Made

| Decision | Detail |
|---|---|
| Star placeholder | `0xAB` (`!`) repeated — no star glyph in GBA font |
| Palette offset | `BG_PLTT_ID(n) + 1` for color index 1; matches `PIXEL_FILL(1)` |
| Personality=0 | Fixed personality for consistent silhouette shape across reloads |
| BG 1 pre-init | BG 1 template included for future right-panel plans (06-04/05) |

## Next Phase Readiness

- **06-04 (right panel text):** Window 1 (`sLobbyState.menuWindowId = 1`) is initialized and has off-white fill; ready for species name, level, moves
- **06-05 (menu input):** `Task_LobbyMain` is the target; input handling goes there
- **Concern:** `Task_LobbyFadeOut` is defined but unused — compiler warning suppressed by build passing; will be wired in 06-04/05
