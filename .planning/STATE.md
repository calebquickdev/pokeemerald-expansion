# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-27)

**Core value:** Player can find an active raid den, enter it, and complete a 3v1 Dynamax battle to catch a powerful (potentially Gigantamax) Pokémon.
**Current focus:** Phase 3 — Battle Core

## Current Position

Phase: 3 of 7 (Battle Core)
Plan: 1 of ? in current phase
Status: In progress
Last activity: 2026-02-27 — Completed 03-01-PLAN.md

Progress: [████████░░] ~40% (8/~20 plans estimated)

## Performance Metrics

**Velocity:**
- Total plans completed: 8
- Average duration: ~9 minutes
- Total execution time: ~72 minutes

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01 Den Foundation | 3 | ~27 min | ~9 min |
| 02 Overworld Den Object | 4 | ~36 min | ~9 min |
| 03 Battle Core | 1 | ~9 min | ~9 min |

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
- [Phase 3, 03-01]: CreateBattleStartTask exported (was static) via battle_setup.h — needed by raid_den.c
- [Phase 3, 03-01]: Battler 3 holds B_POSITION_OPPONENT_RIGHT but GetBattlerSide returns B_SIDE_PLAYER for it during RAID battles

### Pending Todos

None.

### Blockers/Concerns

- [Phase 1 - RESOLVED]: Save block extension added `dynamaxDens[]` at end of SaveBlock2 — sizeof now 0xF7C (3964), 4 bytes below sector limit
- [Phase 2 - NOTE]: Pre-existing build error in `src/data/trainers.h` (MOVE_HIDDEN_POWER_ICE, AI_FLAG_DOUBLE undefined) unrelated to raid den work
- [Phase 3+]: OAM sprite budget: destroy lobby sprites before battle starts to stay under 64 sprites per screen
- [Phase 4]: Boss permanent Dynamax requires guarding `UndoDynamax` and Dynamax timer — unguarded, end-of-turn logic will break

## Session Continuity

Last session: 2026-02-27
Stopped at: Completed 03-01-PLAN.md — DoRaidBattle entry point and RAID controller branch
Resume file: None
