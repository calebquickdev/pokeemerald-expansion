---
phase: 04
plan: 02
subsystem: battle-gimmick
tags: [dynamax, raid, rotation, battle-main, CanDynamax]

dependency-graph:
  requires: ["04-01"]
  provides: ["dynamax-rotation-cycle", "raid-candynamax-gate"]
  affects: ["04-03"]

tech-stack:
  added: []
  patterns: ["rotation-state-machine", "raid-guard-bypass"]

key-files:
  created: []
  modified:
    - src/raid_den.c
    - include/raid_den.h
    - src/battle_main.c
    - src/battle_dynamax.c

decisions:
  - "TryAdvanceRaidRotation placed before DoRaidBattle in raid_den.c; required battle_gimmick.h include for GetActiveGimmick"
  - "raid_den.h added to battle_main.c includes; hook placed immediately before AssignUsableGimmicks() in BattleTurnPassed()"
  - "CanDynamax RAID guard uses short-circuit: !(BATTLE_TYPE_RAID && B_SIDE_PLAYER) skips both HasTrainerUsedGimmick and ShouldTrainerBattlerUseGimmick"

metrics:
  duration: ~8 minutes
  completed: 2026-02-27
---

# Phase 4 Plan 02: Dynamax Rotation Cycle Summary

Dynamax rotation state machine and CanDynamax RAID gate implemented; `dynamaxEnergy` now cycles 0→2→3→0 each turn (skipping when active), and only the eligible battler may Dynamax per turn.

## Tasks Completed

| # | Task | Commit | Files |
|---|------|--------|-------|
| 1 | TryAdvanceRaidRotation + BattleTurnPassed hook | 052add36ab | src/raid_den.c, include/raid_den.h, src/battle_main.c |
| 2 | Patch CanDynamax — RAID bypass + rotation gate | 500694674f | src/battle_dynamax.c |

## What Was Built

**TryAdvanceRaidRotation (src/raid_den.c)**

Advances `gBattleStruct->raid.dynamaxEnergy` through the rotation 0→2→3→0. Guards against advancing while any player-side battler (0, 2, 3) is currently Dynamaxed — the rotation only ticks when no one is transformed.

**BattleTurnPassed hook (src/battle_main.c)**

`#include "raid_den.h"` added. `TryAdvanceRaidRotation()` called immediately before `AssignUsableGimmicks()` in `BattleTurnPassed()`, gated on `BATTLE_TYPE_RAID`.

**CanDynamax patches (src/battle_dynamax.c)**

1. `HasTrainerUsedGimmick` and `ShouldTrainerBattlerUseGimmick` wrapped in a `!(BATTLE_TYPE_RAID && B_SIDE_PLAYER)` guard. This prevents `SetGimmickAsActivated(battler2)` from permanently blocking battler 3, since the engine marks both partners as activated in RAID battles.
2. Commented-out rotation stub replaced with two live checks: fainted guard (`IsBattlerAlive`) and `dynamaxEnergy != battler` rotation gate.

## Decisions Made

| Decision | Rationale |
|----------|-----------|
| Added `#include "battle_gimmick.h"` to raid_den.c | `GetActiveGimmick` declared there; not transitively available via existing includes |
| Fainted guard placed before rotation gate | Prevents dead battlers from holding the rotation when `dynamaxEnergy` points at them |
| RAID guard uses short-circuit `&&` | Single condition covers both `HasTrainerUsedGimmick` and `ShouldTrainerBattlerUseGimmick` skips cleanly |

## Deviations from Plan

**1. [Rule 3 - Blocking] Added `#include "battle_gimmick.h"` to src/raid_den.c**

- **Found during:** Task 1 (pre-build analysis)
- **Issue:** `GetActiveGimmick` is declared in `battle_gimmick.h`; not available via the existing includes in `raid_den.c`
- **Fix:** Added `#include "battle_gimmick.h"` after `#include "battle.h"`
- **Files modified:** src/raid_den.c

## Verification

- `make -j4` exit code 0 — build clean, ROM generated

## Next Phase Readiness

Plan 04-03 (ally icon sprite swap) can proceed. `allyIconSpriteId[2]` in `struct RaidData` is zeroed at battle start; 04-03 needs to populate it and swap sprites when `dynamaxEnergy` changes.
