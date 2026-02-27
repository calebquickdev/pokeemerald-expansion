---
phase: 03-battle-core
plan: "03"
subsystem: battle-raid
tags: [raid, dynamax, party-setup, boss-hp, permanent-dynamax]
requires: ["03-01", "03-02"]
provides: ["SetupRaidBossParty", "UndoDynamax-guard", "permanent-dynamax-boss"]
affects: ["03-04", "03-05"]
tech-stack:
  added: []
  patterns: [star-level-range-randomization, hp-multiplier-override, infinite-dynamax-turns]
key-files:
  created: []
  modified:
    - src/raid_den.c
    - src/battle_dynamax.c
    - src/battle_controllers.c
decisions:
  - "Boss HP set via SetMonData overwrite (not CreateMon param) — only way to exceed stat-calculated max"
  - "dynamaxTurns[1]=0xFF chosen as sentinel for infinite turns (255 rounds >> any realistic battle)"
  - "SetActiveGimmick+SetGimmickAsActivated called at layout init, not ActivateDynamax, because gBattleMons not yet populated"
metrics:
  duration: ~8 minutes
  completed: "2026-02-27"
---

# Phase 3 Plan 03: Boss Party Setup, HP×3, Permanent Dynamax Summary

**One-liner:** Star-range level randomization, HP×3 override, and infinite-turn GIMMICK_DYNAMAX activated at controller init.

## What Was Built

### Task 1 — SetupRaidBossParty() in src/raid_den.c

Replaced the empty stub with a real implementation. The function:

- Reads `species` and `starRating` from `gSaveBlock2Ptr->dynamaxDens[denId]`
- Clamps `stars` to 1–5; substitutes `SPECIES_ZIGZAGOON` for uninitialized species
- Picks a random level in `[sStarLevelMin[stars], sStarLevelMax[stars]]` using `Random()`
- Calls `ZeroEnemyPartyMons()`, then `CreateMon` for the boss at `gEnemyParty[0]`
- Triples the boss's HP by reading `MON_DATA_MAX_HP`, multiplying by 3, then writing back both `MON_DATA_MAX_HP` and `MON_DATA_HP`
- Creates CPU ally Sceptile at `gPlayerParty[3]` and Blaziken at `gPlayerParty[4]`, both at boss level

Added `#include "pokemon.h"` and `#include "random.h"` (were missing); removed the now-unnecessary forward declaration.

### Task 2 — UndoDynamax guard in src/battle_dynamax.c

Inserted early-return guard as the first statement:

```c
if ((gBattleTypeFlags & BATTLE_TYPE_RAID) && GetBattlerSide(battler) == B_SIDE_OPPONENT)
    return;
```

Prevents the end-of-turn Dynamax expiry logic from reverting boss HP or clearing its Dynamax state.

### Task 2 (cont.) — Permanent Dynamax activation in src/battle_controllers.c

Inside the `BATTLE_TYPE_RAID` branch of `InitSinglePlayerBtlControllers`, after the `BufferBattlePartyCurrentOrderBySide` calls:

```c
SetActiveGimmick(1, GIMMICK_DYNAMAX);
SetGimmickAsActivated(1, GIMMICK_DYNAMAX);
gBattleStruct->dynamax.dynamaxTurns[1] = 0xFF;
```

Added `#include "battle_gimmick.h"` to expose `SetActiveGimmick`/`SetGimmickAsActivated`/`GIMMICK_DYNAMAX`.

## Verification

- Full `make -j4` build succeeded with no errors or warnings in modified files
- ROM `pokeemerald.gba` produced

## Decisions Made

| Decision | Rationale |
|---|---|
| HP override via SetMonData post-CreateMon | CreateMon derives HP from stats; only a post-creation write can exceed the stat-calculated ceiling |
| dynamaxTurns[1] = 0xFF | 255 is a safe "never expires" sentinel; standard Dynamax is 3 turns |
| SetActiveGimmick at init, not ActivateDynamax | gBattleMons[1] unpopulated at InitSinglePlayerBtlControllers time; safe path uses state flags only |

## Deviations from Plan

None — plan executed exactly as written.

## Next Phase Readiness

Plan 03-03 satisfies BATTLE-03, BATTLE-04, and BATTLE-05. The boss is now created with correct level range, tripled HP, and permanent Dynamax. Subsequent plans can build shield logic (BATTLE-06) and catch mechanics on top of this foundation.
