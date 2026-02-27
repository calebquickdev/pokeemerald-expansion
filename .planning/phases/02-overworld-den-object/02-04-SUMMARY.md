---
phase: "02"
plan: "04"
subsystem: overworld-scripting
tags: [raid-den, map-scripting, object-events, littleroot-town]
dependency-graph:
  requires: ["02-01", "02-02", "02-03"]
  provides: ["LittlerootTown den interaction", "MAP_SCRIPT_ON_LOAD den setup"]
  affects: ["future den maps"]
tech-stack:
  added: []
  patterns: ["den object event placed with inactive GFX; SetupDynamaxDenObjects updates it on load"]
key-files:
  created: []
  modified:
    - data/maps/LittlerootTown/map.json
    - data/maps/LittlerootTown/scripts.inc
decisions:
  - "flag: 0 used on den object (no hide flag); graphic toggled by script not hide/show"
  - "MAP_SCRIPT_ON_LOAD placed before ON_TRANSITION per execution order requirements"
metrics:
  duration: "~3 minutes"
  completed: "2026-02-27"
---

# Phase 02 Plan 04: Den Interaction Script and Test Map Summary

**One-liner:** LittlerootTown Raid Den at (7,3) with full active/inactive/Wishing-Piece interaction script via MAP_SCRIPT_ON_LOAD.

## What Was Done

### Task 1 — map.json object event
Appended a 9th object event to `data/maps/LittlerootTown/map.json`:
- `graphics_id`: `OBJ_EVENT_GFX_RAID_DEN_INACTIVE` (default placed state; overwritten at runtime)
- Coordinates: `(7, 3)`, elevation 3
- `movement_type`: `MOVEMENT_TYPE_NONE`, range 0×0
- Script: `LittlerootTown_EventScript_RaidDen_0`
- `flag`: `"0"` (always visible; appearance controlled by script)

### Task 2 — scripts.inc additions

**MapScripts header:**
- Added `.set LOCALID_RAID_DEN_0, 9`
- Prepended `map_script MAP_SCRIPT_ON_LOAD, LittlerootTown_OnLoad` to `LittlerootTown_MapScripts`
- Added `LittlerootTown_OnLoad` handler: calls `special SetupDynamaxDenObjects` then `end`

**Den interaction script (`LittlerootTown_EventScript_RaidDen_0`):**
- `FLAG_DAILY_DEN_RAIDED_0` unset → active path: sets `VAR_0x8000=0`, calls `OpenDenLobbyScreen`
- `FLAG_DAILY_DEN_RAIDED_0` set → inactive path:
  - No Wishing Piece → shows "Seems it's a den for DYNAMAX Pokémon to appear..."
  - Has Wishing Piece → YESNO prompt → on YES: removes item, sets `VAR_0x8000=0`, calls `ActivateDynamaxDen` + `SetupDynamaxDenObjects`, shows confirmation
  - On NO → silent release

## Build State

Build exits with the pre-existing `trainers.party` errors (`MOVE_HIDDEN_POWER_ICE`, `MOVE_HIDDEN_POWER_FIRE`, `AI_FLAG_DOUBLE`). All files modified by this plan compiled cleanly:
- `mapjson` processed `LittlerootTown/map.json` without error
- `event_scripts.s` (including LittlerootTown scripts) assembled without error

## Deviations from Plan

None — plan executed exactly as written.
