# Roadmap: Dynamax Raid Dens

## Overview

Adds a complete Dynamax Raid Den system to pokeemerald-expansion: interactable overworld den objects with daily RTC resets, a custom lobby screen, a 3v1 Dynamax raid battle with shield mechanics and ally respawn, and post-battle guaranteed catch. The build order is dictated by hard dependencies — den state must exist before the overworld object can read it, the battle engine must run before mechanics layer on top of it, and the lobby and UI are built last once the data and battle they wrap are stable.

## Phases

- [x] **Phase 1: Den Foundation** — Save structure, flags, and daily RTC reset
- [x] **Phase 2: Overworld Den Object** — Map objects, interaction scripts, Wishing Piece item
- [x] **Phase 3: Battle Core** — BATTLE_TYPE_RAID, 3v1 layout, boss stats, RaidAlly controller
- [x] **Phase 4: Dynamax Integration** — RaidData struct, permanent boss Dynamax, ally rotation, sprite swap
- [x] **Phase 5: Raid Mechanics** — Shield, turn limit, storm messages, ally respawn
- [ ] **Phase 6: Den Lobby Screen** — Species pool, BST→star formula, full CB2 lobby UI
- [ ] **Phase 7: Battle UI & Post-Battle Catch** — Raid battle UI, Run menu, 100% catch, GMAX delivery

---

## Phase Details

### Phase 1: Den Foundation
**Goal**: Den state persists across save/load cycles and resets daily with a fresh Pokémon roll
**Depends on**: Nothing (first phase)
**Requirements**: DEN-01, DEN-02, DEN-03
**Success Criteria** (what must be TRUE):
  1. `FLAG_DAILY_DEN_RAIDED(denId)` can be set and cleared via the save system; state survives a save/load
  2. `struct DynamaxDen` in SaveBlock2 stores species + isGmax per den with no save corruption (migration-safe, added at struct end)
  3. Advancing the RTC past midnight triggers `UpdateDynamaxDens()` via `UpdatePerDay()`; all den flags clear and each den's species re-rolls
**Plans**: 3 plans

Plans:
- [x] 01-01: Add `FLAG_DAILY_DEN_RAIDED(denId)` macro to the DAILY_FLAGS range; verify `ClearDailyFlags()` resets all den flags
- [x] 01-02: Add `struct DynamaxDen { u16 species; u8 isGmax:1; }` and `dynamaxDens[MAX_DYNAMAX_DENS]` to the end of SaveBlock2
- [x] 01-03: Stub `RollDynamaxDenPokemon(denId)` (returns a placeholder species) and wire `UpdateDynamaxDens()` into `UpdatePerDay()` in `src/clock.c`

---

### Phase 2: Overworld Den Object
**Goal**: Player can find den objects in the overworld, see their Active/Inactive state, and use a Wishing Piece to activate an inactive den
**Depends on**: Phase 1
**Requirements**: OBJ-01, OBJ-02, OBJ-03, OBJ-04, OBJ-05, ITEM-01, ITEM-02
**Success Criteria** (what must be TRUE):
  1. Den object appears on the map with the correct Active/Inactive graphic on map load (reads the Phase 1 flag)
  2. Interacting with an inactive den shows "Seems it's a den for DYNAMAX Pokémon to appear..."
  3. With a Wishing Piece in the bag, the inactive den prompts use; accepting removes 1 Wishing Piece and activates the den (flag set, species rolled)
  4. Interacting with an active den triggers the lobby transition (stub print/log acceptable at this phase)
  5. Wishing Piece exists in item data and is restricted to overworld use
**Plans**: 4 plans

Plans:
- [x] 02-01-PLAN.md — Update Wishing Piece `.type` to `ITEM_USE_FIELD` in `src/data/items.h`
- [x] 02-02-PLAN.md — Create placeholder den sprites; register GFX constants, `ObjectEventGraphicsInfo` structs, pointer table entries, and palette lookup
- [x] 02-03-PLAN.md — Declare `ObjectEventSetGraphicsIdByLocalIdAndMap`; add script flag aliases; implement `SetupDynamaxDenObjects`, `ActivateDynamaxDen`, `OpenDenLobbyScreen` stub; register specials
- [x] 02-04-PLAN.md — Add den object event to LittlerootTown map; `MAP_SCRIPT_ON_LOAD` hook; full den interaction script with Wishing Piece flow

---

### Phase 3: Battle Core
**Goal**: A 3v1 raid battle initializes with the correct battler layout, boss stats, and CPU ally controller
**Depends on**: Phase 1
**Requirements**: BATTLE-01, BATTLE-02, BATTLE-03, BATTLE-04, BATTLE-05
**Success Criteria** (what must be TRUE):
  1. Calling `DoRaidBattle()` from a script starts a battle with `BATTLE_TYPE_RAID`; 4 battlers load (player + 2 CPU allies + boss)
  2. CPU allies take turns using `SetControllerToRaidAlly` — no Bag, Run, or catch options in their turn
  3. Boss spawns at the correct level for its star range (1★=15–20, 2★=25–30, …, 5★=55–60)
  4. Boss HP is the base HP × 3
  5. Boss remains permanently Dynamaxed; `UndoDynamax` and the Dynamax timer are skipped for the raid boss
**Plans**: 3 plans

Plans:
- [x] 03-01-PLAN.md — DoRaidBattle() entry point, starRating rename, BATTLE_TYPE_RAID branch in InitSinglePlayerBtlControllers
- [x] 03-02-PLAN.md — SetControllerToRaidAlly controller cloned from PlayerPartner; header declaration
- [x] 03-03-PLAN.md — SetupRaidBossParty() with level/HP×3; UndoDynamax guard; permanent dynamaxTurns[1]=0xFF

---

### Phase 4: Dynamax Integration
**Goal**: Dynamax rotation cycles through allies each turn and ally sprites reflect Dynamax state
**Depends on**: Phase 3
**Requirements**: BATTLE-08, BATTLE-09, BATTLE-10
**Success Criteria** (what must be TRUE):
  1. Turn 1 only the player's Dynamax option is available; turn 2 only CPU ally 1; turn 3 only CPU ally 2; turn 4 cycles back to the player
  2. Attempting to Dynamax out-of-rotation is blocked (option not shown / cannot be selected)
  3. When an ally Dynamaxes, their battle sprite changes from the icon to the full front sprite; reverts when Dynamax ends
**Plans**: 3 plans

Plans:
- [x] 04-01-PLAN.md — Add `struct RaidData` to `BattleStruct`; initialize all raid fields in RAID battle start path
- [x] 04-02-PLAN.md — `TryAdvanceRaidRotation()` + hook in `BattleTurnPassed()`; patch `CanDynamax()` for RAID rotation gating
- [x] 04-03-PLAN.md — Ally sprite swap: `RaidAllyHandleLoadMonSprite` two-sprite setup; `ActivateDynamax`/`UndoDynamax` icon↔front swap hooks

---

### Phase 5: Raid Mechanics
**Goal**: Shield mechanic, 10-turn storm limit, and ally respawn all function correctly in battle
**Depends on**: Phase 4
**Requirements**: BATTLE-06, BATTLE-07, BATTLE-11, BATTLE-12, BATTLE-13, BATTLE-14
**Success Criteria** (what must be TRUE):
  1. When boss HP drops below 75%, boss gains `(1 + star_count)` shield units; dropping below 50% grants a second set
  2. Attacks against a shielded boss deal 0 damage and consume 1 shield unit (Max moves consume 2)
  3. When all 3 allies faint or turn 10 ends, the boss flees, screen fades to black, "The storm hurled you out of the den." appears, and the player returns to the overworld
  4. "The storm is growing stronger." appears at end of each turn; "The storm is growing unbearable!" replaces it on the final turn
  5. A fainted ally skips the next turn, then respawns at full HP at the start of the following turn before move selection
**Plans**: 4 plans

Plans:
- [ ] 05-01-PLAN.md — Struct changes (shieldPhase, respawnTimer[4], gRaidCurrentStarRating) + shield damage intercept in Cmd_adjustdamage
- [ ] 05-04-PLAN.md — Storm counter + battle scripts (BattleScript_RaidStormMessage, BattleScript_RaidStormExpired) + boss flee via B_OUTCOME_PLAYER_TELEPORTED
- [ ] 05-02-PLAN.md — Shield activation at 75%/50% HP thresholds in Cmd_datahpupdate using shieldPhase tracking
- [ ] 05-03-PLAN.md — Ally respawn: faint intercept in HandleFaintedMonActions + TryRaidAllyRespawn() + all-allies-fainted edge case

---

### Phase 6: Den Lobby Screen
**Goal**: Player sees a complete lobby screen with boss silhouette, star rating, and party selection before entering the raid
**Depends on**: Phase 2, Phase 3
**Requirements**: LOBBY-01, LOBBY-02, LOBBY-03, LOBBY-04, LOBBY-05, LOBBY-06, LOBBY-07, LOBBY-08, LOBBY-09, DEN-04, DEN-05
**Success Criteria** (what must be TRUE):
  1. `Special_OpenDenLobbyScreen` opens a CB2 screen with an orange left half showing the boss's front sprite silhouette (GMAX sprite if applicable)
  2. The correct star count (1–5) displays above the silhouette; GMAX always shows 5★
  3. The player's trainer name and the icon of the selected party Pokémon (default: slot 1) show on the right half
  4. "Change Pokémon" opens the party screen; closing it updates the displayed icon in the lobby
  5. All four menu options work: "Invite Others" shows "This feature is not complete yet." and returns; "Don't Invite Others" starts the raid; "Quit" returns to the overworld
**Plans**: 5 plans

Plans:
- [ ] 06-01: Complete `RollDynamaxDenPokemon(denId)`: base-form-only species pool (GMAX forms as the sole exception); BST→star formula (0–299=1★, 300–460=2★, 461–494=3★, 495–549=4★, 550+=5★; GMAX always 5★)
- [ ] 06-02: Build `CB2_DenLobbyScreen` scaffolding: `SetMainCallback2`, `CreateTask`, `RunTasks`; allocate and free VRAM/palettes on enter/exit
- [ ] 06-03: Render left half: orange background fill; load and display boss silhouette via `LoadSpecialPokePic`; draw star rating above silhouette
- [ ] 06-04: Render right half: player trainer name text; create party Pokémon icon via `CreateMonIcon`; hook "Change Pokémon" to party screen and update icon on return
- [ ] 06-05: Implement lobby menu: "Invite Others" WIP message, "Don't Invite Others" calls `DoRaidBattle()`, "Quit" returns to overworld via `SetMainCallback2(CB2_ReturnToFieldWithOpenMenu)`

---

### Phase 7: Battle UI & Post-Battle Catch
**Goal**: The raid battle renders its custom UI and the player catches the boss (including GMAX form delivery) after winning
**Depends on**: Phase 5, Phase 6
**Requirements**: UI-01, UI-02, UI-03, CATCH-01, CATCH-02, CATCH-03
**Success Criteria** (what must be TRUE):
  1. Left side of the battle screen shows animated icon sprites for the player + 2 CPU allies, each with a relative HP bar
  2. Right side shows the boss's full front sprite with HP bar and name
  3. Player menu shows Fight / Bag / Pokémon / Run; selecting Run exits the raid and returns the player to the overworld
  4. After the boss faints, a Poké Ball selection prompt appears; any ball thrown results in a guaranteed catch
  5. If the caught Pokémon is a GMAX form, the player receives the base (non-GMAX) species with Gigantamax Factor set
**Plans**: 3 plans

Plans:
- [ ] 07-01: Implement left-side battle UI: render animated icon sprites for all 3 allied battlers with relative HP bars; update bars on HP change
- [ ] 07-02: Implement right-side boss UI: load full front sprite, HP bar, and name window; add Run option to player menu that triggers raid-exit battle script
- [ ] 07-03: Implement post-battle catch: ball selection prompt after boss KO; `BALL_3_SHAKES_SUCCESS` branch in `Cmd_handleballthrow` for `BATTLE_TYPE_RAID`; GMAX form → base species + Gigantamax Factor flag on delivery

---

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Den Foundation | 3/3 | Complete | 2026-02-27 |
| 2. Overworld Den Object | 4/4 | Complete | 2026-02-27 |
| 3. Battle Core | 3/3 | Complete | 2026-02-27 |
| 4. Dynamax Integration | 3/3 | Complete | 2026-02-27 |
| 5. Raid Mechanics | 4/4 | Complete | 2026-02-27 |
| 6. Den Lobby Screen | 0/5 | Not started | - |
| 7. Battle UI & Post-Battle Catch | 0/3 | Not started | - |
