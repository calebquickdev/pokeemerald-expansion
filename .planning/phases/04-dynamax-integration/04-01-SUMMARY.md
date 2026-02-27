---
phase: 04-dynamax-integration
plan: "01"
subsystem: battle-structs
tags: [dynamax, raid, battle-struct, RaidData]
requires: [03-03]
provides: [struct-RaidData, BattleStruct.raid]
affects: [04-02, 04-03]
tech-stack:
  added: []
  patterns: [struct-embedding, sentinel-value]
key-files:
  created: []
  modified:
    - include/battle.h
    - src/battle_controllers.c
decisions:
  - "struct RaidData defined after struct BattleGimmickData (declaration order preserves binary layout)"
  - "allyIconSpriteId sentinel is MAX_SPRITES (64) — not SPRITE_NONE (0xFF) — matching sprite array boundary"
  - "MAX_SPRITES available transitively via battle.h -> sprite.h; no new include needed in battle_controllers.c"
metrics:
  duration: "6 minutes"
  completed: "2026-02-27"
---

# Phase 4 Plan 01: struct RaidData Foundation Summary

**One-liner:** Defined `struct RaidData` with `dynamaxEnergy`, `allyIconSpriteId[2]`, `shieldHp`, and `respawnTimer[3]`; embedded as `BattleStruct.raid`; zeroed at RAID battle start.

## Tasks Completed

| # | Task | Commit | Files |
|---|------|--------|-------|
| 1 | Define struct RaidData and embed in BattleStruct | 773ed3b238 | include/battle.h |
| 2 | Initialize raid fields in InitSinglePlayerBtlControllers RAID branch | 2cb8bfa2b9 | src/battle_controllers.c |

## Changes Made

### include/battle.h

- Inserted `struct RaidData` (8 bytes) immediately after `struct BattleGimmickData` closing brace (line 588)
- Added `struct RaidData raid;` field to `BattleStruct` immediately after `struct BattleGimmickData gimmick`

### src/battle_controllers.c

- In `InitSinglePlayerBtlControllers`, within the `BATTLE_TYPE_RAID` branch, after the existing `gBattleStruct->dynamax.dynamaxTurns[1] = 0xFF` line:
  - `dynamaxEnergy = 0` (player battler 0 eligible to Dynamax on turn 1)
  - `allyIconSpriteId[0] = allyIconSpriteId[1] = MAX_SPRITES` (sentinel: sprites not yet created)
  - `shieldHp = 0`, `memset(respawnTimer, 0, ...)` (Phase 5 fields zeroed for safety)

## Decisions Made

| Decision | Rationale |
|----------|-----------|
| `allyIconSpriteId` sentinel is `MAX_SPRITES` (64) | Matches the valid sprite index boundary; `gSprites` is indexed 0–63, so 64 unambiguously means "no sprite" |
| No new `#include` in `battle_controllers.c` | `MAX_SPRITES` is transitively available via `battle.h` → `sprite.h` (confirmed line 1059) |
| `shieldHp` and `respawnTimer` zeroed this phase | Phase 5 will write the real values; zeroing now prevents undefined behavior if any code checks them early |

## Deviations from Plan

None — plan executed exactly as written.

## Verification

Build completed successfully:
- `make -j4` exit code 0
- `pokeemerald.gba` produced
- EWRAM: 86.68%, IWRAM: 86.45%, ROM: 78.28%

## Next Phase Readiness

Plan 04-02 (Dynamax rotation) can now read/write `gBattleStruct->raid.dynamaxEnergy`.
Plan 04-03 (ally icon sprites) can now use `gBattleStruct->raid.allyIconSpriteId[]` with `MAX_SPRITES` as the "unset" sentinel.
The previously commented-out stub in `battle_dynamax.c:116` (`gBattleStruct->raid.dynamaxEnergy`) now resolves without error.
