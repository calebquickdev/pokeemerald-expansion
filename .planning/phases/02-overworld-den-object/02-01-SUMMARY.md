---
phase: "02"
plan: "01"
subsystem: items
tags: [item, wishing-piece, field-use, overworld]
depends_on: []
provides: ["ITEM_WISHING_PIECE shows USE button in overworld bag"]
affects: ["02-02", "02-03", "02-04"]
tech-stack:
  added: []
  patterns: []
key-files:
  created: []
  modified: ["src/data/items.h"]
decisions:
  - "Keep fieldUseFunc as CannotUse; actual den activation comes from interaction scripts, not bag USE"
metrics:
  duration: "~8 minutes"
  completed: "2026-02-27"
---

# Phase 02 Plan 01: Wishing Piece Item Type Summary

**One-liner:** Changed `[ITEM_WISHING_PIECE].type` from `ITEM_USE_BAG_MENU` to `ITEM_USE_FIELD` so the item shows a USE button in the overworld bag.

## What Was Done

Single field change in `src/data/items.h` line 3574:

```c
.type = ITEM_USE_FIELD,   // was ITEM_USE_BAG_MENU
```

`fieldUseFunc` remains `ItemUseOutOfBattle_CannotUse`. The USE button will show "Can't use that here." when selected from the bag standalone — the real activation path goes through den interaction scripts via `checkitem`/`removeitem`.

## Verification

- `build/modern/src/item.o` compiled with exit code 0 (items.h is clean)
- Full build failure in `src/data/trainers.party` is pre-existing (unrelated `trainers.h` modification in working tree)

## Decisions Made

| Decision | Rationale |
|----------|-----------|
| Keep `CannotUse` as fieldUseFunc | Den activation is script-driven, not bag-driven; standalone use returning "Can't use that here." is acceptable behavior |

## Deviations from Plan

None — plan executed exactly as written.

## Next Phase Readiness

Plan 02-02 can proceed. The Wishing Piece now appears as a field-usable item, ready for the den interaction script to consume it.
