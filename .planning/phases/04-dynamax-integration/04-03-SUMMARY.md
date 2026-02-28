---
phase: 04-dynamax-integration
plan: 03
subsystem: battle
tags: [dynamax, sprites, ally, icon, RaidAllyHandleLoadMonSprite]
requires: ["04-01", "04-02"]
provides: ["RaidAllyHandleLoadMonSprite two-sprite", "RaidAllyHandleFaintAnimation", "ActivateDynamax icon swap", "UndoDynamax icon restore"]
affects: []
key-files:
  modified:
    - src/battle_controller_raid_ally.c
    - src/battle_dynamax.c
key-decisions:
  - "Load front sprite pixels with HandleLoadSpecialPokePic(TRUE) after BattleLoadMonSpriteGfx for palette"
  - "Icon at GetBattlerSpriteCoord positions; invisible full-sprite for Dynamax"
duration: ~2 hours
completed: 2026-02-27
---

# Phase 4 Plan 03: Ally Sprite Swap Summary

RaidAllyHandleLoadMonSprite two-sprite setup (invisible front + visible icon) with ActivateDynamax/UndoDynamax swap hooks

## What Was Built

- `RaidAllyHandleLoadMonSprite`: loads front sprite pixels (invisible in `gBattlerSpriteIds[battler]`) + icon sprite (stored in `raid.allyIconSpriteId[battler-2]`)
- `RaidAllyHandleFaintAnimation`: frees icon sprite before delegating to `BtlController_HandleFaintAnimation`
- Dispatch table updated: `CONTROLLER_FAINTANIMATION` → `RaidAllyHandleFaintAnimation`
- `ActivateDynamax`: RAID ally guard frees icon, sets sentinel to MAX_SPRITES, unhides full sprite
- `UndoDynamax`: RAID ally guard hides full sprite, creates new icon, stores new sprite ID

## Commits

| Task | Commit | Description |
|------|--------|-------------|
| Task 1 | bb33419896 | feat(04-03): override RaidAllyHandleLoadMonSprite with two-sprite setup |
| Task 2 | 5783eccaa3 | feat(04-03): add sprite swap to ActivateDynamax and UndoDynamax |
| Metadata | (this commit) | docs(04-03): complete ally sprite swap plan |

## Deviations from Plan

None

## Next Phase Readiness

Phase 4 complete, ready for Phase 5 (Raid Mechanics)
