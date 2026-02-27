---
phase: "01"
plan: "03"
subsystem: raid-den
tags: [gba, save-data, clock, daily-events, species-stub]
requires: ["01-02"]
provides: ["UpdateDynamaxDens", "RollDynamaxDenPokemon"]
affects: ["future species pool logic", "den UI phases"]
tech-stack:
  added: []
  patterns: ["daily-event-hook", "stub-placeholder"]
key-files:
  created: ["src/raid_den.c"]
  modified: ["src/clock.c"]
decisions:
  - "RollDynamaxDenPokemon returns SPECIES_RALTS unconditionally as Phase 1 placeholder"
  - "daysSince accepted but suppressed with (void) for future multi-day skip logic"
  - "isGmax hardcoded to 0; Gigantamax deferred to later phase"
metrics:
  duration: "258s"
  completed: "2026-02-27"
---

# Phase 01 Plan 03: UpdateDynamaxDens Hook and Species Stub Summary

**One-liner:** Daily den species re-roll wired into clock via `UpdateDynamaxDens` stub writing `SPECIES_RALTS` to all `MAX_DYNAMAX_DENS` save slots.

## What Was Done

### Task 1 — Create `src/raid_den.c`

Implemented both functions declared in `include/raid_den.h`:

- `RollDynamaxDenPokemon(u8 denId)` — Phase 1 stub; ignores `denId` and returns `SPECIES_RALTS`
- `UpdateDynamaxDens(u16 daysSince)` — iterates over all `MAX_DYNAMAX_DENS` entries in `gSaveBlock2Ptr->dynamaxDens[]`, calling `RollDynamaxDenPokemon(i)` for species and setting `isGmax = 0`

### Task 2 — Hook into `UpdatePerDay()` in `src/clock.c`

Added `UpdateDynamaxDens(daysSince);` immediately after `ClearDailyFlags();` inside `UpdatePerDay()`. This ensures dens re-roll every midnight alongside all other daily events.

## Verification Results

```
src\clock.c:47:        UpdateDynamaxDens(daysSince);
src\raid_den.c:4:u16 RollDynamaxDenPokemon(u8 denId)
src\raid_den.c:10:void UpdateDynamaxDens(u16 daysSince)
src\raid_den.c:17:        gSaveBlock2Ptr->dynamaxDens[i].species = RollDynamaxDenPokemon(i);
```

Build (via WSL `arm-none-eabi-gcc`): `src/raid_den.c` and `src/clock.c` both compiled with zero errors and zero warnings. Pre-existing errors (`MOVE_HIDDEN_POWER_ICE`, `AI_FLAG_DOUBLE` in `trainers.party`) are unrelated to this plan.

## Commits

| Hash | Message |
|------|---------|
| `24892b8` | `feat(01-03): add RollDynamaxDenPokemon and UpdateDynamaxDens stubs` |
| `b399e44` | `feat(01-03): wire UpdateDynamaxDens into UpdatePerDay in clock.c` |

## Decisions Made

| Decision | Rationale |
|----------|-----------|
| `SPECIES_RALTS` as stub return value | Concrete constant verifiable in tests; real pool logic deferred to species-pool phase |
| `(void)daysSince` suppression | Keeps API stable for future multi-day logic without compiler warnings |
| `isGmax = 0` hardcoded | Gigantamax mechanic is a later-phase concern; prevents undefined struct field reads |

## Deviations from Plan

None — plan executed exactly as written.

## Next Phase Readiness

The daily re-roll pipeline is complete end-to-end at the stub level. Future phases can replace the body of `RollDynamaxDenPokemon` with a real species-pool lookup without touching `clock.c` or the save layout.
