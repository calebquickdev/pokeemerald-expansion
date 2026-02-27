---
phase: 03-battle-core
plan: "01"
subsystem: battle-entry
tags: [raid-battle, battle-setup, battle-controllers, dynamax-den]

dependency-graph:
  requires: ["02-04"]
  provides: ["DoRaidBattle entry point", "RAID controller layout", "GetBattlerSide RAID guard"]
  affects: ["03-02", "03-03"]

tech-stack:
  added: []
  patterns: ["BATTLE_TYPE_RAID flag driving 3v1 layout via InitSinglePlayerBtlControllers RAID branch"]

key-files:
  created: []
  modified:
    - include/raid_den.h
    - src/raid_den.c
    - include/battle_setup.h
    - src/battle_setup.c
    - data/specials.inc
    - src/battle_controllers.c
    - include/battle.h

decisions:
  - "Exported CreateBattleStartTask (was static) via battle_setup.h so raid_den.c can call it directly rather than duplicating the task creation logic"
  - "Battler 3 assigned B_POSITION_OPPONENT_RIGHT but GetBattlerSide returns B_SIDE_PLAYER for battler 3 during RAID — position table keeps symmetry with Double layout, guard handles all side-dependent logic"

metrics:
  duration: ~9 minutes
  completed: "2026-02-27"
---

# Phase 3 Plan 01: DoRaidBattle Entry Point Summary

DoRaidBattle entry point with BATTLE_TYPE_DOUBLE|BATTLE_TYPE_RAID flags, 3v1 controller layout (Player/Opponent/RaidAlly/RaidAlly), and GetBattlerSide guard for battler 3.

## Tasks Completed

| Task | Description | Commit |
|------|-------------|--------|
| 1 | Rename starRating, implement DoRaidBattle(), wire OpenDenLobbyScreen, register special | 86c1e95 |
| 2 | RAID branch in InitSinglePlayerBtlControllers, GetBattlerSide guard | 86c1e95 |

## What Was Built

### include/raid_den.h
- Renamed `DynamaxDen._pad` → `starRating` (struct size unchanged)
- Added `void DoRaidBattle(void)` declaration

### src/raid_den.c
- Added includes: `main.h`, `battle.h`, `battle_setup.h`, `battle_transition.h`, `overworld.h`, `script.h`, `constants/battle.h`
- `ActivateDynamaxDen` now sets `starRating = 1` as 1-star placeholder
- Added stub `SetupRaidBossParty(u8 denId)` (Phase 5 will fill this)
- `DoRaidBattle()` sets `BATTLE_TYPE_DOUBLE | BATTLE_TYPE_RAID`, calls `CreateBattleStartTask(B_TRANSITION_BLUR, 0)`, `ScriptContext_Stop()`
- `OpenDenLobbyScreen` now calls `DoRaidBattle()` directly (Phase 6 will restore real lobby)

### include/battle_setup.h + src/battle_setup.c
- `CreateBattleStartTask` de-static'd and declared in header so external callers can use it

### data/specials.inc
- Added `def_special DoRaidBattle` after `def_special OpenDenLobbyScreen`

### src/battle_controllers.c
- RAID branch inserted as first conditional in `InitSinglePlayerBtlControllers`:
  - `gBattlersCount = 4`
  - Positions: 0=PLAYER_LEFT, 1=OPPONENT_LEFT, 2=PLAYER_RIGHT, 3=OPPONENT_RIGHT
  - Controllers: Player(0), Opponent(1), RaidAlly(2), RaidAlly(3)
  - Party indexes: 0→0, 1→0, 2→3, 3→4

### include/battle.h
- `GetBattlerSide` patched: returns `B_SIDE_PLAYER` for battler 3 when `BATTLE_TYPE_RAID` is active

## Decisions Made

1. **Exported CreateBattleStartTask**: Was `static` in `battle_setup.c`. Made non-static and declared in `battle_setup.h` so `raid_den.c` can call it without duplicating the task-creation boilerplate.

2. **Position/side split**: Battler 3 holds `B_POSITION_OPPONENT_RIGHT` in the position table (keeps double-battle geometry consistent) but `GetBattlerSide` overrides the side to `B_SIDE_PLAYER` for battler 3 during RAID. This ensures all side-dependent battle logic (ally checks, targeting, etc.) sees battler 3 as a player-side participant.

## Deviations from Plan

### Auto-fixed Issues

**[Rule 3 - Blocking] Exported CreateBattleStartTask**

- **Found during:** Task 1
- **Issue:** `CreateBattleStartTask` was `static` in `battle_setup.c`, inaccessible from `raid_den.c`
- **Fix:** Removed `static` qualifier; added declaration to `battle_setup.h`
- **Files modified:** `src/battle_setup.c`, `include/battle_setup.h`
- **Commit:** 86c1e95

## Build Result

Full build succeeded. ROM size: 26,266,648 bytes (78.28% of 32MB).

## Next Phase Readiness

Phase 03-02 (`SetControllerToRaidAlly` implementation) and 03-03 (boss HP scaling) can proceed. The RAID branch in `InitSinglePlayerBtlControllers` calls `SetControllerToRaidAlly` which is declared in `battle_controllers.h` — the implementation stub from 03-02 must compile before the raid battle can reach the action selection phase.
