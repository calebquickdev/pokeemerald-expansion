---
phase: 05-raid-mechanics
plan: "03"
subsystem: battle-engine
tags: [raid, respawn, faint-handling, battle-util, battle-main]

dependency-graph:
  requires: ["05-01", "05-04"]
  provides: ["ally-respawn-cycle", "all-fainted-flee-trigger"]
  affects: ["05-05", "05-06"]

tech-stack:
  added: []
  patterns: ["faint-intercept-before-normal-flow", "decrement-timer-on-turn-end"]

key-files:
  created: []
  modified:
    - src/battle_util.c
    - src/battle_main.c
    - src/raid_den.c
    - include/raid_den.h

decisions:
  - "Faint intercept inserted before Nuzlocke block in case 4 so RAID and Nuzlocke paths remain independent"
  - "respawnTimer starts at 2: first decrement leaves ally absent for one full turn, second decrement at 0 restores them"
  - "All-fainted check compares timers[0], [2], [3] > 0 (boss at index 1 is never written)"

metrics:
  duration: "~13 minutes"
  completed: "2026-02-28"
---

# Phase 5 Plan 03: Ally Respawn Cycle Summary

**One-liner:** 2-turn respawn timer via faint intercept in HandleFaintedMonActions + TryRaidAllyRespawn() on turn end

## What Was Done

Implemented BATTLE-11: when a player-side ally faints in a raid battle, the normal switch-in prompt is suppressed and a 2-turn respawn timer is set instead. After two turns, the ally is restored to full HP and re-enters the battle automatically.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Faint intercept + TryRaidAllyRespawn call | 215615a4fc | src/battle_util.c, src/battle_main.c |
| 2 | TryRaidAllyRespawn implementation + header declaration | 215615a4fc | src/raid_den.c, include/raid_den.h |

## Key Implementation Details

**`src/battle_util.c` — HandleFaintedMonActions case 4:**
- Before the Nuzlocke block, check `BATTLE_TYPE_RAID && B_SIDE_PLAYER`
- Set `respawnTimer[battler] = 2`, set absent bit, advance `faintedActionsState = 5`, return TRUE
- All-fainted guard: if timers[0] > 0 && timers[2] > 0 && timers[3] > 0, trigger `BattleScript_RaidStormExpired` with `B_OUTCOME_PLAYER_TELEPORTED`

**`src/battle_main.c` — HandleEndTurn_ContinueBattle:**
- Expanded the `BATTLE_TYPE_RAID` block to call `TryRaidAllyRespawn()` after `TryAdvanceRaidRotation()`

**`src/raid_den.c` — TryRaidAllyRespawn:**
- Iterates battlers {0, 2, 3}; skips timer == 0
- Decrements non-zero timers; at 0: restores `hp = maxHP`, clears absent bit

## Deviations from Plan

None — plan executed exactly as written.
