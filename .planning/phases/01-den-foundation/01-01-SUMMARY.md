---
phase: "01"
plan: "01"
subsystem: raid-den
tags: [flags, daily-flags, raid-den, header]
depends_on: []
provides: [FLAG_DAILY_DEN_RAIDED, raid_den.h, MAX_DYNAMAX_DENS]
affects: ["01-02", "01-03"]
tech-stack:
  added: []
  patterns: [flag-reservation, header-declaration]
key-files:
  created: ["include/raid_den.h"]
  modified: ["include/constants/flags.h"]
decisions:
  - "Used DAILY_FLAGS_START + 0x15 as base offset (FLAG_UNUSED_0x935) for 20 contiguous den flags"
  - "Placed all raid den declarations in a single include/raid_den.h header"
metrics:
  duration: "~3 minutes"
  completed: "2026-02-27"
---

# Phase 01 Plan 01: Daily Den Raided Flags Summary

**One-liner:** Reserved 20 daily flags (0x935–0x948) for raid den raided state via `FLAG_DAILY_DEN_RAIDED(denId)` macro in new `include/raid_den.h` header.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Reserve daily flags 0x935–0x948 in flags.h | 2581947d | include/constants/flags.h |
| 2 | Create include/raid_den.h | 09f51720 | include/raid_den.h |

## Decisions Made

1. **Flag range selection:** Flags 0x935–0x948 (offsets 0x15–0x28 from `DAILY_FLAGS_START`) were already unused and form a contiguous block of exactly 20, matching `MAX_DYNAMAX_DENS`. `ClearDailyFlags()` resets them at midnight with no extra code.

2. **Single header pattern:** All raid den types, constants, and function forward declarations go in `include/raid_den.h`. Future source files include this one header for all raid den access.

## Deviations from Plan

None — plan executed exactly as written.

## Build Notes

The project has pre-existing build errors in `src/data/trainers.party` (`MOVE_HIDDEN_POWER_ICE`, `MOVE_HIDDEN_POWER_FIRE`, `AI_FLAG_DOUBLE` undeclared) that are unrelated to this plan. These failures exist independently of the flag and header changes introduced here.

## Next Phase Readiness

- `01-02` and `01-03` can include `raid_den.h` and use `FLAG_DAILY_DEN_RAIDED`, `MAX_DYNAMAX_DENS`, and the `DynamaxDen` struct immediately.
- Function bodies for `UpdateDynamaxDens` and `RollDynamaxDenPokemon` are forward-declared and ready to be implemented.
