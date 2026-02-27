---
phase: "01"
plan: "02"
subsystem: save-system
tags: [save-block, struct, persistence, gba]

dependency-graph:
  requires: ["01-01"]
  provides: ["saveblock2-den-storage"]
  affects: ["01-03", "phase-2-den-logic"]

tech-stack:
  added: []
  patterns: ["append-only SaveBlock2 extension"]

key-files:
  created: []
  modified:
    - include/global.h

decisions:
  - "Added dynamaxDens at the very end of SaveBlock2 (migration-safe; no existing offsets shift)"
  - "Used #include raid_den.h in global.h preamble rather than redefining struct DynamaxDen inline"

metrics:
  duration: "~7 minutes"
  completed: "2026-02-27"
---

# Phase 01 Plan 02: SaveBlock2 Den Storage Summary

**One-liner:** `dynamaxDens[20]` appended to `SaveBlock2` via `raid_den.h` include in `global.h`, bringing sizeof to 0xF7C (3964 bytes, within 3968-byte sector limit).

## What Was Done

### Task 1 — Add `#include "raid_den.h"` to global.h

Added `#include "raid_den.h"` immediately after `#include "config/save.h"` (line 21) in the preamble of `include/global.h`. This makes `struct DynamaxDen` and `MAX_DYNAMAX_DENS` visible to all translation units that include `global.h`.

### Task 2 — Append `dynamaxDens` field to `SaveBlock2`

Inserted `struct DynamaxDen dynamaxDens[MAX_DYNAMAX_DENS];` as the last field of `struct SaveBlock2`, immediately before the closing `};`. Updated the sizeof comment from `0xF2C` to `0xF7C` (3884 + 80 = 3964 bytes).

**Size math:**
- Previous: `sizeof(SaveBlock2) = 0xF2C = 3884` bytes
- `struct DynamaxDen` = 4 bytes (`u16 species` + `u8 isGmax` + `u8 _pad`)
- 20 dens × 4 bytes = 80 bytes added
- New: `0xF7C = 3964` bytes — 4 bytes below the 3968-byte sector limit

## Verification Results

- `grep dynamaxDens include/global.h` — found at line 573
- `grep raid_den.h include/global.h` — found at line 22
- Build via WSL (`make -j4`): compiled successfully; only pre-existing unrelated errors present (`MOVE_HIDDEN_POWER_ICE`, `AI_FLAG_DOUBLE` in `trainers.party`) — no errors in raid den or save-related code

## Deviations from Plan

None — plan executed exactly as written.

## Commits

| Hash | Message |
|------|---------|
| 1ff330a1bb | feat(01-02): add dynamaxDens array to SaveBlock2 |
