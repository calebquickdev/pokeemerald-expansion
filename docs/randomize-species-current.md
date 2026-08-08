# Randomize Species — Current Behavior

## Summary

`FLAG_RANDOMIZE_MON` (New Game setting **RANDOMIZE SPECIES**) remaps Pokémon through `GetRandomizedSpecies()` in `src/pokemon.c`, called from `CreateMon`. Remapping is a **per-playthrough global X→Y** function of `OT_ID + sourceSpecies + NG+ offset`, not per-route. The draw is non-bijective (collisions; early-dex table species can map to the same replacement everywhere they appear).

## Entry points

| Piece | Location |
|-------|----------|
| UI toggle | `src/new_game_settings_menu.c` → `SETTING_RANDOMIZE_SPECIES` (default ON) |
| Flag | `FLAG_RANDOMIZE_MON` (`0x284`) |
| Core remap | `GetRandomizedSpecies` / `IsSpeciesValidForRandomization` in `src/pokemon.c` |
| Starters | `PickRandomSpecies` / `InitializeStarterChoices` / `BirchCase_GiveMon` in `src/ui_birch_case.c` |
| Wild | Static tables → `CreateWildMon` → `CreateMon` |
| Trainers | `CreateNPCTrainerPartyFromTrainer` → `CreateMon` + `CustomTrainerPartyAssignMoves` |

## Algorithm (wild / trainers / gifts via CreateMon)

1. If flag off or `SPECIES_NONE`, return input unchanged.
2. Seed SFC32 with `OT + species + GetNewGamePlusLevelOffset()`.
3. Draw `1 .. NUM_SPECIES-1` until `IsSpeciesValidForRandomization` passes (enabled; not mega/primal/ultra/gmax/tera/totem; not `cannotBeTraded`).

No map/zone in the seed. Not a permutation. Regional/stripe/crowned/etc. can still pass the filter.

## Starters (separate path)

- `PickRandomSpecies(set, slot)` seeds `OT + set*100 + slot + NG+` and returns a raw species ID with **no** validity filter (Gigantamax etc. possible).
- Give path clears `FLAG_RANDOMIZE_MON` to avoid double-roll.
- When species-random is on and no defined moves, `BirchCase_GiveMon` calls `GetRandomMove(..., MOVE_NONE)` — scrambled moves even if **Randomize Moves** is off.

## Movesets today

| Context | Species-random only | + Randomize Moves |
|---------|---------------------|-------------------|
| Wild | New species level-up learnset | Battle/UI remap via `ResolveMonMoves` |
| Trainers (scripted moves) | **Keeps vanilla table moves** on new species | Those originals remapped at battle intro |
| Starters | Forced random move IDs | Also subject to display remap |

## Forms

- Wild/trainer path blocks battle-only flags listed above.
- Starters do not.
- Separate system `src/random_mon_generation.c` has stricter `IsRandomSpeciesFormAllowed`; not used by this toggle.

## Alignment vs intended requirements

| Requirement | Current |
|-------------|---------|
| Per-MAPSEC independent tables | No — global species map |
| Uniform legal-base pool (no next-valid walk) | Partial — rejection over raw IDs; forms inflate domain |
| Source Gen1–3 bias must not bias destinations | Weak — seed mixes via SFC32, but ID-space sampling is not a clean base pool |
| Normalize source to base species | No |
| Block `??????????` / none / egg | Mostly via enabled check; starters unfiltered |
| One catchable form per base per playthrough | No |
| Catch reverts non-persistent forms | No dedicated hook |
| Fishing → innate Water pool | No |
| Ability lure uses remapped types | No — types vanilla table species |
| Trainer learnsets after remap | No — keeps scripted moves |
| Starter pool modes / legends toggle | No |
| Eggs / Frontier exempt | No |
| Fossils → fossil-only pool | No |

See `docs/randomize-species-requirements.md` for the target design.
