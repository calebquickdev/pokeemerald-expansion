---
phase: "02"
plan: "03"
subsystem: overworld-den
tags: [specials, header-declarations, object-events, flags, raid-den]
depends_on: ["01-01", "01-02", "01-03", "02-01"]
provides: ["SetupDynamaxDenObjects", "ActivateDynamaxDen", "OpenDenLobbyScreen specials", "sDenObjectTable", "FLAG_DAILY_DEN_RAIDED_0..3 script aliases"]
affects: ["02-04", "phase-3-lobby", "phase-6-lobby-screen"]
tech-stack:
  added: []
  patterns: ["den-object-table pattern (mapGroup/mapNum/localId -> denId)", "special function registration via def_special"]
key-files:
  created: []
  modified:
    - src/raid_den.c
    - include/raid_den.h
    - include/event_object_movement.h
    - include/constants/flags.h
    - data/specials.inc
decisions:
  - "OpenDenLobbyScreen is an intentional empty stub; Phase 6 will wire SetMainCallback2(CB2_DenLobbyScreen)"
  - "sDenObjectTable is a static const struct array; single LittlerootTown entry for Phase 2 testing"
  - "ObjectEventSetGraphicsIdByLocalIdAndMap parameter order is (localId, mapNum, mapGroup, graphicsId) — mapNum before mapGroup matches the definition at src/event_object_movement.c:2797"
metrics:
  duration: "~8 minutes"
  completed: "2026-02-27"
---

# Phase 02 Plan 03: C Specials and Header Declarations Summary

**One-liner:** Three raid-den specials implemented and registered, with den-object table, missing header declaration, and script-accessible flag aliases.

## What Was Done

### Task 1 — Header and specials registration

- **`include/event_object_movement.h`:** Added missing declaration for `ObjectEventSetGraphicsIdByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup, u16 graphicsId)` adjacent to `ObjectEventSetGraphicsId`, fixing a would-be implicit-declaration warning.
- **`include/raid_den.h`:** Added declarations for `SetupDynamaxDenObjects`, `ActivateDynamaxDen`, and `OpenDenLobbyScreen` after the existing `RollDynamaxDenPokemon`/`UpdateDynamaxDens` declarations.
- **`include/constants/flags.h`:** Expanded the Phase 1 comment into four `#define` aliases (`FLAG_DAILY_DEN_RAIDED_0..3`) mapping to `DAILY_FLAGS_START + 0x15..0x18`, enabling event scripts to reference den raided flags by name without using the C macro form.
- **`data/specials.inc`:** Appended `def_special SetupDynamaxDenObjects`, `def_special ActivateDynamaxDen`, and `def_special OpenDenLobbyScreen` at end of file.

### Task 2 — Implementation in `src/raid_den.c`

Added new includes (`event_data.h`, `event_object_movement.h`, `constants/event_objects.h`) and three functions:

- **`SetupDynamaxDenObjects`:** Reads `gSaveBlock1Ptr->location` to filter `sDenObjectTable` by current map, then sets each matching object event's graphics to `OBJ_EVENT_GFX_RAID_DEN_ACTIVE` or `OBJ_EVENT_GFX_RAID_DEN_INACTIVE` depending on `FLAG_DAILY_DEN_RAIDED(denId)`.
- **`ActivateDynamaxDen`:** Reads `gSpecialVar_0x8000` as den ID, clears the raided flag, re-rolls species via `RollDynamaxDenPokemon`, and stores result in `gSaveBlock2Ptr->dynamaxDens[denId]`.
- **`OpenDenLobbyScreen`:** Empty stub. Phase 6 will replace with `SetMainCallback2(CB2_DenLobbyScreen)`.

`sDenObjectTable` has one entry: `{ MAP_GROUP(LITTLEROOT_TOWN), MAP_NUM(LITTLEROOT_TOWN), 9, 0 }` — the test den for Phase 2.

## Verification Results

- `ObjectEventSetGraphicsIdByLocalIdAndMap` declared in `event_object_movement.h` ✓
- `FLAG_DAILY_DEN_RAIDED_0..3` defined in `constants/flags.h` ✓
- All 3 specials registered in `data/specials.inc` ✓
- All 3 function definitions present in `src/raid_den.c` ✓
- Build: `src/raid_den.c` compiled with zero errors and zero warnings. Pre-existing errors in `src/data/trainers.party` (`MOVE_HIDDEN_POWER_ICE`, `AI_FLAG_DOUBLE`) are unrelated to this plan and were present before execution.

## Deviations from Plan

None — plan executed exactly as written.

## Next Phase Readiness

Plan 02-04 can now proceed: it will add the LittlerootTown den NPC object event template (local ID 9) to `data/maps/LittlerootTown/map.json` and wire the `MAP_SCRIPT_ON_LOAD` to call `SetupDynamaxDenObjects`.
