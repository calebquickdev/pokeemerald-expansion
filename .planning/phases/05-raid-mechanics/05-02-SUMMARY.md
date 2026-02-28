---
phase: 05
plan: 02
subsystem: battle
tags: [raid, shield, hp-threshold, battle_script_commands]
requires: ["05-01"]
provides: ["BATTLE-12", "BATTLE-13"]
affects: ["05-03"]
tech-stack:
  added: []
  patterns: ["multiply-based fraction comparison for threshold checks"]
key-files:
  created: []
  modified:
    - src/battle_script_commands.c
decisions:
  - "Nested if inside shieldPhase==0 branch handles single hit crossing both thresholds"
  - "Multiply comparison (hp*4<maxHp*3) avoids division on GBA"
metrics:
  duration: ~5 minutes
  completed: 2026-02-28
---

# Phase 5 Plan 02: HP-Threshold Shield Activation Summary

Inserted threshold check block into `Cmd_datahpupdate` in `src/battle_script_commands.c` satisfying BATTLE-12 (75%) and BATTLE-13 (50%).

## What Was Done

Added a guarded block immediately after the HP subtraction if/else in the `else` branch of `Cmd_datahpupdate`. The block is active only during raid battles (`BATTLE_TYPE_RAID`), only for the boss battler (`B_POSITION_OPPONENT_LEFT`), and only for direct move damage (not passive damage or no-effect moves).

When `shieldPhase == 0` and HP has fallen below 75%, `shieldHp` is set to `1 + gRaidCurrentStarRating` and `shieldPhase` advances to 1. A nested check handles a single massive hit that crosses the 50% threshold in the same instance, advancing directly to `shieldPhase = 2`. When `shieldPhase == 1` and HP has fallen below 50%, `shieldHp` and `shieldPhase` are updated to their final values.

## Deviations from Plan

None — plan executed exactly as written.
