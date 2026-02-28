---
phase: 06-den-lobby-screen
plan: 02
subsystem: ui
tags: [gba, callback2, ewram, palette-fade, task-system, vblank]

requires:
  - phase: 06-01
    provides: RollDynamaxDenPokemon, OpenDenLobbyScreen stub in raid_den.c

provides:
  - CB2_DenLobbyScreen: full CB2 lifecycle (VBlank/Main callbacks + task loop)
  - sLobbyState: EWRAM struct grouping all lobby-local state
  - Task_LobbyFadeIn → Task_LobbyMain task chain
  - OpenDenLobbyScreen delegates to CB2_DenLobbyScreen via SetMainCallback2

affects:
  - 06-03 (boss sprite rendering plugs into MainCB2_Lobby task loop)
  - 06-04 (menu windows use sLobbyState.menuWindowId/infoWindowId)
  - 06-05 (exit path uses Task_LobbyFadeOut)

tech-stack:
  added: []
  patterns:
    - "CB2 lifecycle: SetVBlankCallback(NULL) → DMA clear VRAM/OAM/PLTT → ScanlineEffect_Stop → ResetTasks/SpriteData/PaletteFade → BeginNormalPaletteFade → SetVBlankCallback/SetMainCallback2 → CreateTask"
    - "Lobby state pattern: single static EWRAM_DATA struct survives CB2 re-entry from party screen"

key-files:
  created: []
  modified:
    - src/raid_den.c
    - include/raid_den.h

key-decisions:
  - "sLobbyState.returnedFromParty guards denId re-caching on CB2 re-entry (gSpecialVar_0x8000 only valid on first entry)"
  - "Task_LobbyFadeOut stub left intentionally minimal — caller sets exit callback before triggering fade"

patterns-established:
  - "CB2_DenLobbyScreen: models GBA screen init after src/diploma.c VBlank/Main pattern"

duration: ~15min
completed: 2026-02-28
---

# Phase 6 Plan 2: Den Lobby Screen CB2 Scaffold Summary

**CB2_DenLobbyScreen with VRAM/OAM/palette clear, EWRAM LobbyState struct, and Task_LobbyFadeIn→Task_LobbyMain task chain — lobby screen now fades in from black instead of launching battle directly**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-02-28T03:00:00Z
- **Completed:** 2026-02-28T03:30:00Z
- **Tasks:** 2/2
- **Files modified:** 2

## Accomplishments

- Defined `struct LobbyState` in EWRAM with all 7 lobby-local fields (denId, selectedSlot, bossSpriteId, iconSpriteId, menuWindowId, infoWindowId, returnedFromParty)
- Added 16 lobby-specific includes (gpu_regs, scanline_effect, task, sprite, palette, bg, window, text, menu, string_util, decompress, battle_gfx_sfx_util, pokemon_icon, party_menu, constants/rgb, constants/party_menu)
- Implemented full `CB2_DenLobbyScreen` with GBA display init, DMA clears, and palette fade
- `OpenDenLobbyScreen` now delegates to `CB2_DenLobbyScreen` via `SetMainCallback2` instead of calling `DoRaidBattle` directly
- `Task_LobbyFadeIn` → `Task_LobbyMain` chain in place; `Task_LobbyFadeOut` stub ready for exit paths

## Task Commits

1. **Task 1: Define EWRAM lobby state and add required includes** - `eb88a24d3a` (feat)
2. **Task 2: Implement CB2_DenLobbyScreen, callbacks, and fade task chain** - `1c710f300c` (feat)

## Files Created/Modified

- `src/raid_den.c` - Added includes, LobbyState EWRAM struct, VBlankCB_Lobby, MainCB2_Lobby, CB2_DenLobbyScreen, updated OpenDenLobbyScreen, Task_LobbyFadeIn/Main/FadeOut stubs
- `include/raid_den.h` - Added `CB2_DenLobbyScreen` declaration

## Decisions Made

- `sLobbyState.returnedFromParty` guards denId caching so `gSpecialVar_0x8000` is only read on first entry — re-entry from party screen preserves existing state
- `Task_LobbyFadeOut` body is intentionally minimal; whichever exit path triggers the fade is responsible for setting the next callback before calling `BeginNormalPaletteFade`

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

None — build clean with no errors or warnings.

## Next Phase Readiness

- `CB2_DenLobbyScreen` scaffolding complete; Plans 06-03/04/05 can plug into `MainCB2_Lobby` task loop
- `sLobbyState` struct fields (bossSpriteId, menuWindowId, etc.) ready for 06-03 and 06-04 to populate
- EWRAM consumption: 227244 B / 256 KB (86.69%) — 12 bytes added by LobbyState struct

---
*Phase: 06-den-lobby-screen*
*Completed: 2026-02-28*
