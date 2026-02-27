# Dynamax Raid Dens

## What This Is

A new overworld and battle system for pokeemerald-expansion: interactable raid den objects scattered across the region that allow the player to enter 4v1 Dynamax raid battles. Dens activate and reset daily, and the Wishing Piece item can manually activate an inactive den. The system includes a den lobby screen, a custom raid battle variant, and post-battle catch mechanics.

## Core Value

The player can find an active raid den in the overworld, enter it, and complete a challenging 4v1 Dynamax battle to catch a powerful (potentially Gigantamax) Pokémon.

## Requirements

### Validated

- ✓ Battle engine with Dynamax move support — existing
- ✓ Overworld object event / script interaction system — existing
- ✓ Flag and variable system for persistent state — existing
- ✓ Sprite system (OAM, task-based animations) — existing
- ✓ RTC integration (`AgbMain` initializes RTC) — existing
- ✓ Item system and bag — existing
- ✓ Party selection and Pokémon data (BST, species, forms) — existing
- ✓ AI controller framework (`battle_controller_opponent.c`) — existing
- ✓ Task/callback-based UI screen pattern — existing

### Active

**Den Overworld Object**
- [ ] Static den sprite with two states: Active (glowing) and Inactive
- [ ] Den is interactable; behavior branches on active/inactive state
- [ ] Inactive den: displays "Seems it's a den for DYNAMAX Pokémon to appear..."
- [ ] Inactive den with Wishing Piece in bag: prompts "Would you like to use a Wishing Piece?" → Yes activates den, No exits
- [ ] Active den: opens Den Lobby Screen

**Den State & Daily Reset**
- [ ] Each den tracks active/inactive state via a flag
- [ ] At midnight (RTC), all dens reset to active
- [ ] On reset, each den calculates and stores a random Pokémon (including GMAX possible) in a save var

**Den Lobby Screen**
- [ ] Orange background on left half; silhouette of raid Pokémon's front sprite centered
- [ ] 1–5 stars displayed above silhouette based on BST (see Key Decisions)
- [ ] GMAX Pokémon always display as 5 stars regardless of BST
- [ ] Right half: player trainer name, icon sprite of currently selected raid Pokémon (default: party slot 1)
- [ ] Menu options: Invite Others (WIP stub), Don't Invite Others, Change Pokémon, Quit
- [ ] "Invite Others" shows "This feature is not complete yet." and returns to lobby
- [ ] "Change Pokémon" opens party screen and updates selected Pokémon icon in lobby UI
- [ ] "Don't Invite Others" begins raid battle with 3 CPU allies

**Raid Battle**
- [ ] 4v1 layout: player's chosen Pokémon + 3 CPU allies vs. permanently-Dynamaxed boss
- [ ] Boss level randomized within range by star rating (1★: 15–20, 2★: 25–30, 3★: 35–40, 4★: 45–50, 5★: 55–60)
- [ ] Boss HP multiplied by 4
- [ ] Turn limit of 10; after turn 10 boss flees, screen fades to black, message "The storm hurled you out of the den." returns player to overworld
- [ ] End-of-turn message: "The storm is growing stronger." On final turn: "The storm is growing unbearable!"
- [ ] Dynamax rotation: turn 1 = player can Dynamax, turn 2 = CPU ally 1, turn 3 = CPU ally 2, turn 4 = CPU ally 3, turn 5 = player again (cycles)
- [ ] Ally Dynamax: sprite swaps to full front sprite while Dynamaxed
- [ ] Fainted Pokémon skip one turn, then respawn at full HP at turn start before move selection
- [ ] Menu: Fight / Bag / Pokémon / Run (Run exits raid and returns to overworld)
- [ ] Left side UI: animated icon sprites of 4 allied Pokémon, each with a simple relative HP bar
- [ ] Right side UI: full front sprite of boss Pokémon with HP bar and name

**Shield Mechanic**
- [ ] Boss gains shields at 75% HP: (1 + star_count) shield units
- [ ] Boss gains a second set of shields at 50% HP: (1 + star_count) shield units
- [ ] While shielded, attacks deal 0 damage and remove 1 shield unit instead (Max moves remove 2)

**Post-Battle Catch**
- [ ] After boss is knocked out, player selects a Poké Ball to throw (any ball, 100% catch rate)
- [ ] If caught Pokémon is a GMAX form, player receives the base (non-GMAX) form with Gigantamax Factor set

**CPU Allies**
- [ ] 3 CPU allies selected randomly from a hardcoded placeholder species list (to be expanded later)
- [ ] CPU allies use the same AI controller as standard opponent battlers

**Wishing Piece Item**
- [ ] Wishing Piece item added to item data if not already present
- [ ] Usable in overworld context only; triggers den activation flow

### Out of Scope

- Networked Raid Dens (Invite Others multiplayer) — explicitly WIP, stub only
- Den placement across the world map — system first, placement separate
- Curated CPU ally species list — placeholder list now, full list later
- Raid den rewards beyond the caught Pokémon (berries, items, etc.) — post-v1

## Context

- Built on pokeemerald-expansion; Dynamax move infrastructure already exists in the battle engine
- Den state persistence uses the existing flag/var save system
- Daily reset requires RTC (already initialized in `AgbMain`); must hook into the existing RTC time-of-day check pattern used for berries / tides
- CPU allies use the existing `battle_controller_opponent.c` AI framework with a new controller variant for the 4v1 format
- The lobby screen is a new `CB2_`-style screen following the standard task/callback pattern
- GMAX catch delivery reuses or extends the existing Gigantamax Factor flag on species data

## Constraints

- **Hardware**: No heap allocation in battle/lobby code; use static or stack allocation only
- **Memory**: IWRAM is scarce; only annotate time-critical functions with `IWRAM_CODE`
- **Save space**: Den state (active flags + stored Pokémon per den) must fit within available flag/var budget
- **No standard I/O**: All debug output via `MgbaOpen()` / `AGBPrintfInit()` in debug builds only
- **Thumb mode**: All new C code compiles with `-mthumb -mthumb-interwork`

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| BST star thresholds: 0–299=1★, 300–460=2★, 461–494=3★, 495–549=4★, 550+=5★ | Specified by project owner | — Pending |
| GMAX catch delivers base form with Gigantamax Factor | Matches mainline Sword/Shield behavior | — Pending |
| Shields trigger at 75% and 50% HP (two separate events) | Confirmed by owner; matches mainline behavior | — Pending |
| CPU allies use placeholder species list | Full list deferred; unblocks implementation | — Pending |
| "Invite Others" is a WIP stub | Multiplayer out of scope for v1 | — Pending |
| Den placement deferred | System correctness first; world placement is content work | — Pending |

---
*Last updated: 2026-02-27 after initialization*
