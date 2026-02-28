---
phase: 06-den-lobby-screen
plan: 01
subsystem: pokemon-species-selection
tags: [raid-den, species-pool, bst, gmax, dynamax, c]

# Dependency graph
requires:
  - phase: 01-den-foundation
    provides: dynamaxDens[] struct in SaveBlock2 (species, isGmax, starRating fields)
provides:
  - BstToStarRating static helper mapping BST to 1-5 star rating
  - RollDynamaxDenPokemon: real base-form-only species selection with 10% GMAX upgrade
  - All three DynamaxDen fields written atomically by RollDynamaxDenPokemon
affects:
  - 06-den-lobby-screen plans 02+ (lobby screen reads species/starRating/isGmax from dynamaxDens[denId])

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Two-pass species iteration for random selection without heap allocation (count then pick)
    - Alternate-form exclusion via GET_BASE_SPECIES_ID(s)==s guard
    - GMAX upgrade as post-selection pass over isGigantamax entries with matching base form

key-files:
  created: []
  modified:
    - src/raid_den.c

key-decisions:
  - "RollDynamaxDenPokemon writes all three den fields (species, isGmax, starRating) directly — callers need zero field management"
  - "Two-pass loop (count→target→pick) avoids malloc; acceptable since this runs once per den activation"
  - "GetTotalBaseStat==0 used as 'species not real' sentinel (placeholder/invalid entries)"

patterns-established:
  - "Static BST threshold helper: BstToStarRating(u32 bst) -> u8, thresholds at 299/460/494/549"
  - "GMAX upgrade: Random()%10==0, then linear scan for isGigantamax entry with matching GET_BASE_SPECIES_ID"

# Metrics
duration: 9min
completed: 2026-02-27
---

# Phase 6 Plan 1: RollDynamaxDenPokemon Species Pool Summary

**Two-pass base-form-only species pool with BST-driven 1–5 star rating and 10% GMAX upgrade, writing all three den fields atomically**

## Performance

- **Duration:** ~9 min
- **Started:** 2026-02-27T21:51:23Z
- **Completed:** 2026-02-27T22:00:00Z
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments

- Replaced SPECIES_RALTS stub with a real two-pass species-pool picker that excludes alternate forms and GMAX entries
- Added `BstToStarRating` with BST thresholds ≤299→1, ≤460→2, ≤494→3, ≤549→4, else 5
- Applied 10% GMAX upgrade: post-selection scan promotes to matching Gigantamax form, forces starRating=5, isGmax=1
- Unified den field writes inside `RollDynamaxDenPokemon`; removed all manual field assignments from both callers

## Task Commits

1. **Tasks 1 & 2: Add BstToStarRating, real RollDynamaxDenPokemon, update callers** - `1ffbf8b084` (feat)

## Files Created/Modified

- `src/raid_den.c` — Added `battle_ai_util.h` include; replaced stub `RollDynamaxDenPokemon` with full implementation plus `BstToStarRating`; simplified `UpdateDynamaxDens` and `ActivateDynamaxDen` callers

## Decisions Made

- `RollDynamaxDenPokemon` owns all three field writes (species, isGmax, starRating) so callers are trivially simple
- `GetTotalBaseStat(species)==0` guards against placeholder/egg species entries that have no base stats
- `GET_BASE_SPECIES_ID(species)!=species` filters out Alolan, Galarian, and other alternate forms from the base pool

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- `gSaveBlock2Ptr->dynamaxDens[denId].starRating`, `.species`, and `.isGmax` are now correctly populated on every den activation
- Lobby screen (Plans 06-02+) can read these fields without any additional setup
- No blockers

---
*Phase: 06-den-lobby-screen*
*Completed: 2026-02-27*
