# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-27)

**Core value:** Player can find an active raid den, enter it, and complete a 3v1 Dynamax battle to catch a powerful (potentially Gigantamax) Pokémon.
**Current focus:** Phase 1 — Den Foundation

## Current Position

Phase: 1 of 7 (Den Foundation)
Plan: 2 of 3 in current phase
Status: In progress
Last activity: 2026-02-27 — Completed 01-02-PLAN.md

Progress: [██░░░░░░░░] ~10% (2/~20 plans estimated)

## Performance Metrics

**Velocity:**
- Total plans completed: 2
- Average duration: ~10 minutes
- Total execution time: ~20 minutes

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01 Den Foundation | 2 | ~20 min | ~10 min |

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

### Pending Todos

None yet.

### Blockers/Concerns

- [Phase 1 - RESOLVED]: Save block extension added `dynamaxDens[]` at end of SaveBlock2 — sizeof now 0xF7C (3964), 4 bytes below sector limit
- [Phase 3+]: OAM sprite budget: destroy lobby sprites before battle starts to stay under 64 sprites per screen
- [Phase 4]: Boss permanent Dynamax requires guarding `UndoDynamax` and Dynamax timer — unguarded, end-of-turn logic will break

## Session Continuity

Last session: 2026-02-27
Stopped at: Completed 01-02-PLAN.md — dynamaxDens in SaveBlock2
Resume file: None
