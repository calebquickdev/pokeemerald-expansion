# Research Summary: Dynamax Raid Dens

**Project:** pokeemerald-expansion  
**Domain:** GBA ROM hack — Dynamax Raid Dens (4v1 battle format)  
**Researched:** 2025-02-27  
**Confidence:** HIGH (codebase-derived)

---

## Key Stack Findings

- **Battle format:** Use `BATTLE_TYPE_RAID` with `BATTLE_TYPE_DOUBLE`. Model as **3v1** (1 player + 2 CPU allies vs 1 boss) to fit within `MAX_BATTLERS_COUNT = 4`; avoid extending to 5 battlers without a full engine refactor.
- **Controller pattern:** Add `SetControllerToRaidAlly` by cloning `SetControllerToPlayerPartner`; register it in `InitSinglePlayerBtlControllers` for ally battlers. No bag, run, or catch; AI chooses moves; Dynamax rotation via `gBattleStruct->raid.dynamaxEnergy`.
- **Dynamax handling:** Add `struct RaidData` to `BattleStruct` with `dynamaxEnergy`, `shieldHp`, `respawnTimer[]`. Boss starts permanently Dynamaxed (skip `UndoDynamax` for raid boss). Apply 4× HP via `ApplyDynamaxHPMultiplier`. Implement `CanDynamax` raid check and raid move bans per existing TODOs.

---

## Table Stakes Features

- **Map object:** Object event in `map.json` with `OBJ_EVENT_GFX_RAID_DEN_INACTIVE` / `OBJ_EVENT_GFX_RAID_DEN_ACTIVE`, `MOVEMENT_TYPE_LOOK_AROUND`, script reference.
- **Graphics registration:** Add pic tables, `ObjectEventGraphicsInfo`, constants in `event_objects.h`, and pointer table entries.
- **Den interaction script:** `checkitem ITEM_WISHING_PIECE`, `removeitem`, `Special_OpenDenLobbyScreen`, `Special_SetObjectGraphicsId` for active/inactive swap.
- **CB2 Den Lobby Screen:** `SetMainCallback2(CB2_DenLobbyScreen)`, `CreateTask`, `RunTasks`; boss silhouette via `LoadSpecialPokePic`, party icons via `CreateMonIcon`.
- **100% catch:** Branch in `Cmd_handleballthrow` for `BATTLE_TYPE_RAID` using `BALL_3_SHAKES_SUCCESS`.

---

## Architecture Decisions

- **Flags:** Use `DAILY_FLAGS` range (e.g. `FLAG_DAILY_DEN_RAIDED(denId)` at 0x935–0x948). Flag set = inactive (raided today); `ClearDailyFlags()` at midnight resets all dens.
- **Per-den species + isGmax:** Add `struct DynamaxDen { u16 species; u8 isGmax:1; }` in **SaveBlock2** as `dynamaxDens[MAX_DYNAMAX_DENS]`; add at struct end for save compatibility.
- **RTC daily reset:** Call `UpdateDynamaxDens(daysSince)` from `UpdatePerDay()` in `src/clock.c` after `ClearDailyFlags()`; recalculate species via `RollDynamaxDenPokemon(denId)`.
- **Den sprite state:** Use `MAP_SCRIPT_ON_LOAD` + special `SetupDynamaxDenObjects` that checks `!FlagGet(FLAG_DAILY_DEN_RAIDED(denId))` and calls `ObjectEventSetGraphicsIdByLocalIdAndMap`.

---

## Top Pitfalls

1. **Battler count hardcoded at 4** — Use 3v1 layout (4 battlers total); do not add a 5th battler without refactoring `MAX_BATTLERS_COUNT`, `B_POSITION_*`, and all battler-indexed arrays.
2. **OAM / sprite budget exhaustion** — Count sprites per screen; destroy lobby sprites before battle; stay under 64 sprites per screen.
3. **Permanently Dynamaxed boss breaks end-of-turn logic** — Guard `UndoDynamax` and Dynamax timer logic with `if (BATTLE_TYPE_RAID && IsRaidBoss(battler)) return;`.
4. **Save block extension without migration** — Add raid data at end of SaveBlock2; use save versioning and migration scripts if layout changes.
5. **Dynamax energy / raid checks incomplete** — Implement `gBattleStruct->raid.dynamaxEnergy` in `CanDynamax` and raid move bans in `IsMoveBlockedByDynamax` per existing TODOs.

---

## Recommended Build Order

1. **Flag + save + RTC** — Add `FLAG_DAILY_DEN_RAIDED`, `struct DynamaxDen` in SaveBlock2, `UpdateDynamaxDens` in `UpdatePerDay`. Enables den state and daily species roll.
2. **Overworld den objects** — Map JSON, graphics, interaction script, `SetupDynamaxDenObjects` special. Den appears and responds to Wishing Piece.
3. **Battle setup + controllers** — `DoRaidBattle`, `BATTLE_TYPE_RAID` branch in `InitSinglePlayerBtlControllers`, `SetControllerToRaidAlly`. 3v1 battle runs.
4. **Dynamax integration** — `struct RaidData`, permanent boss Dynamax, rotation, 4× HP, `CanDynamax` raid check.
5. **Raid mechanics** — Shield in damage path, turn limit, ally respawn.
6. **Den Lobby Screen** — CB2 screen with boss silhouette, party icons, menu.
7. **Post-battle catch** — 100% catch branch in `Cmd_handleballthrow`.

---

*Research completed: 2025-02-27*  
*Ready for roadmap: yes*
