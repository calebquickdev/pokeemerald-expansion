---
phase: 05-raid-mechanics
plan: "01"
subsystem: battle-mechanics
tags: [raid, shield, struct, dynamax, battle-script]
requires: [04-dynamax-integration]
provides: [RaidData-shieldPhase, RaidData-respawnTimer4, gRaidCurrentStarRating, shield-damage-intercept]
affects: [05-02, 05-03, 05-04, 05-05]
tech-stack:
  added: []
  patterns: [EWRAM_DATA global, battle damage intercept block]
key-files:
  created: []
  modified:
    - include/battle.h
    - include/raid_den.h
    - src/raid_den.c
    - src/battle_script_commands.c
decisions:
  - "Shield intercept uses continue pattern (like Ice Face) rather than MOVE_RESULT_NO_EFFECT; zeros moveDamage while other flags remain"
  - "respawnTimer sized [4] for direct battler-index access; index 1 (boss) intentionally unused"
  - "gRaidCurrentStarRating set immediately after stars clamping, before species/level setup"
metrics:
  duration: ~9 min
  completed: 2026-02-28
---

# Phase 5 Plan 01: RaidData Struct Extension and Shield Intercept Summary

**One-liner:** Extended `struct RaidData` with `shieldPhase`/`respawnTimer[4]`, added `gRaidCurrentStarRating` EWRAM global, and implemented shield damage zeroing in `Cmd_adjustdamage`.

## What Was Done

**Task 1 — Struct extension and star rating global:**
- Added `u8 shieldPhase` to `struct RaidData` in `include/battle.h` (tracks which HP threshold has fired: 0/1/2)
- Resized `respawnTimer` from `[3]` to `[4]` for direct battler-index access
- Defined `EWRAM_DATA u8 gRaidCurrentStarRating = 0` in `src/raid_den.c`
- Set `gRaidCurrentStarRating = stars` in `SetupRaidBossParty` immediately after star clamping
- Added `extern u8 gRaidCurrentStarRating` to `include/raid_den.h`

**Task 2 — Shield damage intercept:**
- Added `#include "raid_den.h"` to `src/battle_script_commands.c`
- Inserted shield intercept block in `Cmd_adjustdamage`, between the Disguise check and the Ice Face check
- When `BATTLE_TYPE_RAID && battlerDef == B_POSITION_OPPONENT_LEFT && shieldHp > 0`:
  - Consumes 1 shield unit (2 for Max moves via `IsMaxMove`)
  - Zeros `moveDamage[battlerDef]`
  - Clears super-effective and not-very-effective result flags
  - Skips remaining damage processing via `continue`

## Commits

- `e65706e4aa` — feat(05-01): extend RaidData struct and add gRaidCurrentStarRating global
- `37d37e3c28` — feat(05-01): add raid shield damage intercept in Cmd_adjustdamage

## Deviations from Plan

None — plan executed exactly as written.

## Next Phase Readiness

All Phase 5 plans can proceed:
- `gBattleStruct->raid.shieldPhase` is available for shield-trigger logic (05-02)
- `gBattleStruct->raid.respawnTimer[battlerIdx]` is available for ally respawn (05-03/05-04)
- `gRaidCurrentStarRating` is accessible from any file including `raid_den.h`
