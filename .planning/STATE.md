# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-27)

**Core value:** Player can find an active raid den, enter it, and complete a 3v1 Dynamax battle to catch a powerful (potentially Gigantamax) Pokémon.
**Current focus:** Phase 6 — Den Lobby Screen

## Current Position

Phase: 5 of 7 (Raid Mechanics)
Plan: 4 of 4 in current phase
Status: Phase complete
Last activity: 2026-02-28 — Completed 05-03-PLAN.md

Progress: [██████████░] ~74% (17/~23 plans estimated)

## Performance Metrics

**Velocity:**
- Total plans completed: 13
- Average duration: ~9 minutes
- Total execution time: ~117 minutes

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01 Den Foundation | 3 | ~27 min | ~9 min |
| 02 Overworld Den Object | 4 | ~36 min | ~9 min |
| 03 Battle Core | 3 | ~27 min | ~9 min |
| 04 Dynamax Integration | 3 | ~27 min | ~9 min |

*Updated after each plan completion*

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- [Pre-planning]: 3v1 layout (player + 2 CPU allies + boss = 4 battlers); do NOT extend MAX_BATTLERS_COUNT
- [Pre-planning]: Boss HP × 3 (adjusted from × 4 when ally count reduced 3→2)
- [Pre-planning]: GMAX catch delivers base form + Gigantamax Factor (matches Sword/Shield behavior)
- [Pre-planning]: Species pool is base-form only; GMAX forms are the sole exception
- [Pre-planning]: Shields trigger at 75% and 50% HP (two separate events)
- [Phase 2]: Wishing Piece uses ITEM_USE_FIELD with CannotUse fieldUseFunc; activation is script-driven (not bag-use)
- [Phase 2]: Den object is always present in map; graphics switch via SetupDynamaxDenObjects on MAP_SCRIPT_ON_LOAD
- [Phase 2]: Test den placed in LittlerootTown at (7,3), local ID 9, den ID 0
- [Phase 3]: CreateBattleStartTask was de-static'd in battle_setup.c and declared in battle_setup.h for external use
- [Phase 3]: Battler 3 uses B_POSITION_OPPONENT_RIGHT; GetBattlerSide guard in battle.h returns B_SIDE_PLAYER for battler 3 during RAID
- [Phase 3]: Allies placed in gPlayerParty[3] (Sceptile) and gPlayerParty[4] (Blaziken) — test save must use ≤2 party slots
- [Phase 4]: Skip HasTrainerUsedGimmick for RAID player-side in CanDynamax — prevents cross-contamination blocking battler 3
- [Phase 4]: Icon sprite at GetBattlerSpriteCoord positions; invisible full-sprite for Dynamax swap
- [Phase 5 - 05-01]: Shield intercept uses continue pattern (like Ice Face); zeros moveDamage while leaving other flags; no MOVE_RESULT_NO_EFFECT
- [Phase 5 - 05-01]: respawnTimer[4] indexed directly by battler; index 1 (boss) intentionally unused
- [Phase 5 - 05-04]: printfromtable requires const u16[] of STRINGIDs, not pointer array; storm messages use dedicated IDs 731/732; STRINGID_STORM_HURLED_OUT_OF_DEN=733; BATTLESTRINGS_COUNT=734

### Pending Todos

None.

### Blockers/Concerns

- [Phase 1 - RESOLVED]: Save block extension added `dynamaxDens[]` at end of SaveBlock2 — sizeof now 0xF7C (3964), 4 bytes below sector limit
- [Phase 2 - NOTE]: Pre-existing build error in `src/data/trainers.h` (MOVE_HIDDEN_POWER_ICE, AI_FLAG_DOUBLE undefined) unrelated to raid den work
- [Phase 3 - NOTE]: gPlayerParty slots 3 and 4 overwritten by SetupRaidBossParty — test saves must have ≤2 party mons (Phase 6 fix)
- [Phase 5 - RESOLVED]: Shield damage intercept built on RaidData.shieldHp and RaidData.respawnTimer[4] — complete in 05-01

## Session Continuity

Last session: 2026-02-28
Stopped at: Completed 05-03-PLAN.md — Ally respawn cycle: faint intercept, TryRaidAllyRespawn(), all-fainted flee trigger
Resume file: None
