# Requirements: Dynamax Raid Dens

**Defined:** 2026-02-27
**Core Value:** Player can find an active raid den, enter it, and complete a 3v1 Dynamax battle to catch a powerful (potentially Gigantamax) Pokémon.

---

## v1 Requirements

### Den State & Data

- [ ] **DEN-01**: Each den has a persistent Active/Inactive flag (`FLAG_DAILY_DEN_RAIDED(denId)`) stored in the save system
- [ ] **DEN-02**: Each den has a stored species + isGmax boolean (`struct DynamaxDen` in SaveBlock2)
- [ ] **DEN-03**: At midnight (RTC daily reset), all dens reset to Active and recalculate their stored Pokémon via `UpdateDynamaxDens()` hooked into `UpdatePerDay()` in `src/clock.c`
- [ ] **DEN-04**: Den species selection draws only from base-form species (no alternate forms, regional variants, etc.); a random form within that species is then selected. Gigantamax forms are the sole exception — they are selected directly and spawn as their GMAX form
- [ ] **DEN-05**: Star rating is derived from stored Pokémon BST: 0–299=1★, 300–460=2★, 461–494=3★, 495–549=4★, 550+=5★; GMAX Pokémon always 5★

### Overworld Den Object

- [ ] **OBJ-01**: Den is a one-tile interactable object event with two graphics states: Active (glowing) and Inactive
- [ ] **OBJ-02**: Den sprite state reflects current Active/Inactive flag on map load via `MAP_SCRIPT_ON_LOAD`
- [ ] **OBJ-03**: Interacting with an Inactive den shows: "Seems it's a den for DYNAMAX Pokémon to appear..."
- [ ] **OBJ-04**: If den is Inactive and player has a Wishing Piece, prompt: "Would you like to use a Wishing Piece?" → Yes removes 1 Wishing Piece, activates den, rolls den Pokémon; No exits
- [ ] **OBJ-05**: Interacting with an Active den opens the Den Lobby Screen

### Wishing Piece Item

- [ ] **ITEM-01**: Wishing Piece item exists in item data (add if not already present)
- [ ] **ITEM-02**: Wishing Piece is usable only in the overworld; triggers the den activation flow when used on an inactive den

### Den Lobby Screen

- [ ] **LOBBY-01**: Custom CB2 screen opens when interacting with an Active den
- [ ] **LOBBY-02**: Left half has an orange background with the raid boss's front sprite silhouette centered; if the boss is a GMAX form, the GMAX front sprite silhouette is shown
- [ ] **LOBBY-03**: Star rating (1–5 stars) is displayed above the silhouette
- [ ] **LOBBY-04**: Right half displays the player's trainer name
- [ ] **LOBBY-05**: Right half displays the animated icon sprite of the currently selected raid Pokémon (default: party slot 1)
- [ ] **LOBBY-06**: Menu option: "Invite Others" — shows "This feature is not complete yet." and returns to lobby (WIP stub)
- [ ] **LOBBY-07**: Menu option: "Don't Invite Others" — begins the raid battle with 2 CPU allies
- [ ] **LOBBY-08**: Menu option: "Change Pokémon" — opens party screen; selected Pokémon updates in lobby UI
- [ ] **LOBBY-09**: Menu option: "Quit" — returns to overworld

### Raid Battle — Setup

- [ ] **BATTLE-01**: Battle uses `BATTLE_TYPE_RAID` flag; 3v1 layout: player's Pokémon + 2 CPU allies vs. 1 permanently-Dynamaxed boss (4 battlers total)
- [ ] **BATTLE-02**: CPU allies use a `SetControllerToRaidAlly` controller (cloned from `SetControllerToPlayerPartner`) with no bag/run/catch
- [ ] **BATTLE-03**: Boss level is randomized within star range: 1★=15–20, 2★=25–30, 3★=35–40, 4★=45–50, 5★=55–60
- [ ] **BATTLE-04**: Boss HP is multiplied by 3
- [ ] **BATTLE-05**: Boss is permanently Dynamaxed; `UndoDynamax` and Dynamax timer are skipped for the raid boss

### Raid Battle — Turn Mechanics

- [ ] **BATTLE-06**: Battle has a 10-turn limit; when the limit expires, boss flees, screen fades to black, message "The storm hurled you out of the den." displays, player returns to overworld
- [ ] **BATTLE-07**: End-of-turn message: "The storm is growing stronger." On the final turn: "The storm is growing unbearable!"
- [ ] **BATTLE-08**: Dynamax rotation cycles each turn: turn 1=player, turn 2=CPU ally 1, turn 3=CPU ally 2, turn 4=player again
- [ ] **BATTLE-09**: The currently eligible ally's Dynamax option is available that turn; others cannot Dynamax
- [ ] **BATTLE-10**: When an ally Dynamaxes, their sprite changes to the full front sprite for the duration
- [ ] **BATTLE-11**: Fainted Pokémon skip the following turn (remain fainted), then respawn at full HP at the start of the next turn before move selection

### Raid Battle — Shield Mechanic

- [ ] **BATTLE-12**: When boss HP crosses below 75%, boss gains (1 + star_count) shield units
- [ ] **BATTLE-13**: When boss HP crosses below 50%, boss gains a second set of (1 + star_count) shield units
- [ ] **BATTLE-14**: While shielded, attacks deal 0 damage and remove 1 shield unit instead (Max moves remove 2 shield units)

### Raid Battle — UI

- [ ] **UI-01**: Left side shows animated icon sprites for the player's Pokémon and 2 CPU allies, each with a relative HP bar
- [ ] **UI-02**: Right side shows the full front sprite of the raid boss with HP bar and name
- [ ] **UI-03**: Player menu: Fight / Bag / Pokémon / Run (Run exits raid, returns to overworld)

### Post-Battle Catch

- [ ] **CATCH-01**: After boss is knocked out, player is prompted to select a Poké Ball to throw
- [ ] **CATCH-02**: Any thrown Poké Ball has 100% catch rate (`BALL_3_SHAKES_SUCCESS` in `Cmd_handleballthrow` for `BATTLE_TYPE_RAID`)
- [ ] **CATCH-03**: If the caught Pokémon is a GMAX form, player receives the base (non-GMAX) species with Gigantamax Factor set (so Dynamaxing it in future battles produces the GMAX form)

---

## v2 Requirements

### Networked Raid Dens

- **NET-01**: Invite Others — real multiplayer over link/wireless
- **NET-02**: Up to 4 human players in a single raid
- **NET-03**: Lobby shows connected players' names and selected Pokémon

### Den Content

- **CONT-01**: Curated per-den Pokémon pools (specific species per den location)
- **CONT-02**: Raid rewards beyond the caught Pokémon (berries, TMs, items)
- **CONT-03**: Full CPU ally species list (beyond placeholder)

### World Placement

- **PLACE-01**: Dens placed across routes and areas of the game world

---

## Out of Scope

| Feature | Reason |
|---------|--------|
| Networked multiplayer (Invite Others) | High complexity, defer to v2 |
| Per-den curated species pools | Content work, system first |
| Raid item rewards (berries, TMs) | Post-v1 feature |
| 5th battler (true 4v1) | Engine refactor risk; 3v1 is the safe implementation |
| World map den placement | Out of scope until system is proven |

---

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| DEN-01 | Phase 1 — Den Foundation | Pending |
| DEN-02 | Phase 1 — Den Foundation | Pending |
| DEN-03 | Phase 1 — Den Foundation | Pending |
| DEN-04 | Phase 6 — Den Lobby Screen | Pending |
| DEN-05 | Phase 6 — Den Lobby Screen | Pending |
| OBJ-01 | Phase 2 — Overworld Den Object | Pending |
| OBJ-02 | Phase 2 — Overworld Den Object | Pending |
| OBJ-03 | Phase 2 — Overworld Den Object | Pending |
| OBJ-04 | Phase 2 — Overworld Den Object | Pending |
| OBJ-05 | Phase 2 — Overworld Den Object | Pending |
| ITEM-01 | Phase 2 — Overworld Den Object | Pending |
| ITEM-02 | Phase 2 — Overworld Den Object | Pending |
| LOBBY-01 | Phase 6 — Den Lobby Screen | Pending |
| LOBBY-02 | Phase 6 — Den Lobby Screen | Pending |
| LOBBY-03 | Phase 6 — Den Lobby Screen | Pending |
| LOBBY-04 | Phase 6 — Den Lobby Screen | Pending |
| LOBBY-05 | Phase 6 — Den Lobby Screen | Pending |
| LOBBY-06 | Phase 6 — Den Lobby Screen | Pending |
| LOBBY-07 | Phase 6 — Den Lobby Screen | Pending |
| LOBBY-08 | Phase 6 — Den Lobby Screen | Pending |
| LOBBY-09 | Phase 6 — Den Lobby Screen | Pending |
| BATTLE-01 | Phase 3 — Battle Core | Pending |
| BATTLE-02 | Phase 3 — Battle Core | Pending |
| BATTLE-03 | Phase 3 — Battle Core | Pending |
| BATTLE-04 | Phase 3 — Battle Core | Pending |
| BATTLE-05 | Phase 3 — Battle Core | Pending |
| BATTLE-06 | Phase 5 — Raid Mechanics | Pending |
| BATTLE-07 | Phase 5 — Raid Mechanics | Pending |
| BATTLE-08 | Phase 4 — Dynamax Integration | Pending |
| BATTLE-09 | Phase 4 — Dynamax Integration | Pending |
| BATTLE-10 | Phase 4 — Dynamax Integration | Pending |
| BATTLE-11 | Phase 5 — Raid Mechanics | Pending |
| BATTLE-12 | Phase 5 — Raid Mechanics | Pending |
| BATTLE-13 | Phase 5 — Raid Mechanics | Pending |
| BATTLE-14 | Phase 5 — Raid Mechanics | Pending |
| UI-01 | Phase 7 — Battle UI & Post-Battle Catch | Pending |
| UI-02 | Phase 7 — Battle UI & Post-Battle Catch | Pending |
| UI-03 | Phase 7 — Battle UI & Post-Battle Catch | Pending |
| CATCH-01 | Phase 7 — Battle UI & Post-Battle Catch | Pending |
| CATCH-02 | Phase 7 — Battle UI & Post-Battle Catch | Pending |
| CATCH-03 | Phase 7 — Battle UI & Post-Battle Catch | Pending |

**Coverage:**
- v1 requirements: 41 total (DEN×5, OBJ×5, ITEM×2, LOBBY×9, BATTLE×14, UI×3, CATCH×3)
- Mapped to phases: 41 ✓
- Unmapped: 0 ✓

---
*Requirements defined: 2026-02-27*
*Last updated: 2026-02-27 after initial definition*
