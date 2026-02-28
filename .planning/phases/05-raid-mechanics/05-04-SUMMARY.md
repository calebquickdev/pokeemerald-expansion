---
phase: 05-raid-mechanics
plan: "04"
subsystem: battle-end-turn
tags: [raid, storm, battle-scripts, string-ids, end-turn]
dependency-graph:
  requires: ["05-01"]
  provides: ["ENDTURN_RAID_STORM", "TryRaidStormTick", "BattleScript_RaidStormMessage", "BattleScript_RaidStormExpired"]
  affects: ["05-03"]
tech-stack:
  added: []
  patterns: ["printfromtable with u16 string ID table"]
key-files:
  created: []
  modified:
    - src/battle_util.c
    - src/raid_den.c
    - include/raid_den.h
    - include/battle_scripts.h
    - data/battle_scripts_1.s
    - src/battle_message.c
    - include/battle_message.h
    - include/constants/battle_string_ids.h
decisions:
  - "printfromtable requires const u16[] of STRINGID values, not const u8* pointer array — added two dedicated STRINGIDs (731, 732) for storm messages alongside STRINGID_STORM_HURLED_OUT_OF_DEN (733); BATTLESTRINGS_COUNT=734"
metrics:
  duration: "~7 minutes"
  completed: "2026-02-28"
---

# Phase 5 Plan 04: Raid Storm End-Turn Wiring Summary

**One-liner:** 10-turn raid storm counter wired into `DoFieldEndTurnEffects` via `ENDTURN_RAID_STORM` + `TryRaidStormTick()`, with `printfromtable`-based storm messages and `B_OUTCOME_PLAYER_TELEPORTED` on expiry.

## What Was Built

### Task 1: ENDTURN_RAID_STORM + TryRaidStormTick()

- Added `ENDTURN_RAID_STORM` to the field end-turn enum in `src/battle_util.c`, immediately before `ENDTURN_FIELD_COUNT`
- Added corresponding `case ENDTURN_RAID_STORM:` in `DoFieldEndTurnEffects()` that gates on `BATTLE_TYPE_RAID` and calls `TryRaidStormTick()`
- Added `#include "raid_den.h"` to `src/battle_util.c`
- Implemented `TryRaidStormTick()` in `src/raid_den.c`: if `gBattleTurnCounter >= 10`, sets `gBattleOutcome = B_OUTCOME_PLAYER_TELEPORTED` and executes `BattleScript_RaidStormExpired`; otherwise sets `MULTISTRING_CHOOSER = (counter == 9 ? 1 : 0)` and executes `BattleScript_RaidStormMessage`
- Declared `bool32 TryRaidStormTick(void)` in `include/raid_den.h`

### Task 2: Battle Scripts and Message Strings

- Added `STRINGID_STORM_GROWING_STRONGER (731)`, `STRINGID_STORM_GROWING_UNBEARABLE (732)`, `STRINGID_STORM_HURLED_OUT_OF_DEN (733)`; `BATTLESTRINGS_COUNT = 734`
- Added all three string entries to `gBattleStringsTable[]` in `src/battle_message.c`
- Added `const u16 gRaidStormStringIds[]` table with the two turn-message string IDs
- Declared `gRaidStormStringIds` in `include/battle_message.h`
- Added `BattleScript_RaidStormMessage` (uses `printfromtable`) and `BattleScript_RaidStormExpired` (uses `printstring`) to `data/battle_scripts_1.s`
- Declared both scripts as `extern const u8[]` in `include/battle_scripts.h`

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] printfromtable type mismatch**

- **Found during:** Task 2
- **Issue:** Plan specified `const u8 *const gRaidStormStringIds[]` (pointer array), but `Cmd_printfromtable` takes `const u16 *ptr` and uses each element as a STRINGID to pass to `PrepareStringBattle()`. A pointer array would produce wrong behavior or a crash.
- **Fix:** Used `const u16 gRaidStormStringIds[]` with dedicated `STRINGID_STORM_GROWING_STRONGER` and `STRINGID_STORM_GROWING_UNBEARABLE` constants, and added the string text directly to `gBattleStringsTable`. This required allocating two extra string IDs (731, 732), shifting `STRINGID_STORM_HURLED_OUT_OF_DEN` to 733 and `BATTLESTRINGS_COUNT` to 734.
- **Files modified:** `include/constants/battle_string_ids.h`, `src/battle_message.c`

## Build Result

Full build passed (`exit_code: 0`). ROM size: ~26.3 MB / 32 MB.

## Next Phase Readiness

- `BattleScript_RaidStormExpired` is now defined — Plan 05-03 can reference it for the all-allies-fainted edge case.
- No blockers for remaining Phase 5 plans.
