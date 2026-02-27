# Codebase Structure

**Analysis Date:** 2025-02-27

## Directory Layout

```
pokeemerald-expansion/
├── asm/                    # Hand-written ARM/Thumb assembly
│   └── macros/             # Script macros (battle_script.inc, event.inc, field_effect_script.inc)
├── constants/              # Assembly constants (gba_constants.inc, global.inc)
├── data/                   # Map data, scripts, layouts (JSON + .inc)
│   ├── layouts/            # Per-map layout binaries (map.bin, border.bin)
│   ├── maps/               # Map headers, connections, events (generated)
│   ├── scripts/            # Map event scripts (.inc, one per map)
│   ├── text/               # String tables
│   └── tilesets/           # Tileset definitions
├── docs/                   # Documentation
│   └── tutorials/          # How-to guides (new Pokémon, moves, trainers, etc.)
├── graphics/               # Source PNGs and converted assets (.4bpp, .gbapal, .lz)
├── include/                # C headers
│   ├── config/             # Feature toggles (battle.h, pokemon.h, ai.h, overworld.h, etc.)
│   ├── constants/          # Enums and #define constants
│   └── gba/                # GBA hardware register definitions
├── libagbsyscall/          # BIOS/syscall library
├── migration_scripts/      # Python scripts for version migration
├── src/                    # C source files
│   └── data/               # C data tables (trainers.h, items.h, pokemon/, etc.)
├── sound/                  # Music (.s assembly) and sound effects
├── test/                   # Test suite (make check)
├── tools/                  # Build tools (gbagfx, scaninc, preproc, trainerproc, etc.)
├── Makefile                # Build entry
├── ld_script_modern.ld      # Linker script (entry, memory layout)
└── charmap.txt             # Character map for preproc
```

## Directory Purposes

**asm/:**
- Purpose: Low-level assembly, interrupt handlers, script macro definitions
- Contains: `crt0.s` (not in asm; in src), `rom_header.s`, `IntrMain`, `battle_script.inc`, `event.inc`, `field_effect_script.inc`
- Key files: `asm/macros/battle_script.inc`, `asm/macros/event.inc`

**src/:**
- Purpose: All game logic (battle, overworld, menus, link, etc.)
- Contains: ~350 .c files; flat structure with some subdirs (`data/`, `data/pokemon/`, etc.)
- Key files: `main.c`, `overworld.c`, `battle_main.c`, `task.c`, `script.c`, `field_effect.c`

**src/data/:**
- Purpose: C data tables (trainer parties, species, items, moves, graphics refs)
- Contains: `.h` files with `static const` data; `trainers.h` (from `trainers.party` via trainerproc), `items.h`, `pokemon/` (learnsets, species_info), `battle_move_effects.h`
- Key files: `src/data/trainers.h`, `src/data/items.h`, `src/data/pokemon/`, `src/data/battle_move_effects.h`

**include/:**
- Purpose: Headers mirroring `src/`; no implementation
- Contains: `main.h`, `overworld.h`, `battle.h`, `task.h`, `script.h`, etc.

**include/config/:**
- Purpose: Feature toggles; primary customization surface
- Contains: `battle.h`, `pokemon.h`, `ai.h`, `overworld.h`, `item.h`, `save.h`, `general.h`, `caps.h`, `debug.h`
- Edit these before modifying engine source when possible

**include/constants/:**
- Purpose: Enums and `#define` for items, species, moves, maps, flags, vars
- Contains: `species.h`, `moves.h`, `items.h`, `trainers.h`, `maps.h`, `flags.h`, `vars.h`, `battle_ai.h`, etc.

**data/:**
- Purpose: Map layouts, scripts, text; some generated at build time
- Contains: `layouts/<MapName>/`, `scripts/<MapName>.inc`, `maps/` (connections, events, headers)
- Key files: `data/scripts/*.inc`, `data/maps/*.json`

**graphics/:**
- Purpose: Source PNGs; converted to .4bpp, .gbapal, .lz by gbagfx
- Contains: `items/icons/`, `pokemon/`, `battle_transitions/`, etc.

**sound/:**
- Purpose: Music and SFX; `.mid` → `.s` via mid2agb
- Contains: `songs/`, `voicegroups/`

**test/:**
- Purpose: Battle/script scenarios for `make check`
- Contains: `test_runner_*.c`, scenario-specific tests

**tools/:**
- Purpose: Custom build tools (gbagfx, scaninc, preproc, trainerproc, mapjson, etc.)
- Generated: Built before main ROM; outputs feed into build

## Key File Locations

**Entry Points:**
- `src/rom_header.s`: ROM header
- `src/crt0.s`: Bootstrap, `AgbMain` call
- `src/main.c`: `AgbMain`, `AgbMainLoop`, callback dispatch
- `ld_script_modern.ld`: `gInitialMainCB2 = CB2_InitCopyrightScreenAfterBootup`

**Configuration:**
- `include/config/battle.h`: Battle mechanics, damage, crits
- `include/config/pokemon.h`: Species limits, egg groups, level caps
- `include/config/ai.h`: AI difficulty and behavior
- `include/config/overworld.h`: Overworld engine settings
- `include/config/item.h`, `save.h`, `general.h`, `debug.h`, `caps.h`

**Core Logic:**
- `src/overworld.c`: Overworld loop, `CB2_Overworld`, `CB2_LoadMap`
- `src/battle_main.c`: Battle loop, `BattleMainCB2`, turn logic
- `src/task.c`: Task system
- `src/script.c`: Map script execution
- `src/field_effect.c`: Field effects (fly, dig, flash, etc.)

**Battle Engine:**
- `src/battle_main.c`: Main battle loop
- `src/battle_script_commands.c`: Battle script command implementations
- `src/battle_controllers.c`: Controller init, link tasks
- `src/battle_controller_player.c`, `battle_controller_opponent.c`, etc.
- `asm/macros/battle_script.inc`: Battle script macro definitions

**Data Tables:**
- `src/data/trainers.h`: Trainer parties (from `src/data/trainers.party` or `*.party`)
- `src/data/items.h`: Item definitions
- `src/data/pokemon/`: Species, learnsets, form data
- `src/data/battle_move_effects.h`: Move effect data
- `include/constants/species.h`, `moves.h`, `items.h`, `trainers.h`: ID constants

**Testing:**
- `test/`: Test sources
- `ld_script_test.ld`: `gInitialMainCB2 = CB2_TestRunner`
- Run: `make check`

## Naming Conventions

**Files:**
- C: `lower_snake_case.c` (e.g. `battle_main.c`, `field_effect.c`)
- Headers: `lower_snake_case.h`
- Data: `trainers.h`, `items.h`, `species.h` in `src/data/`
- Map scripts: `data/scripts/<MapName>.inc` (e.g. `LittlerootTown.inc`)

**Directories:**
- `lower_snake_case` (e.g. `battle_frontier`, `level_up_learnsets`)

**Functions:**
- `UpperCamelCase` (e.g. `BattleScriptCmd_AttackAnimation`, `CreateTask`)
- Callbacks: `CB2_<ScreenName>` (main callback), `CB1_<ScreenName>` (input callback)
- Tasks: `Task_<Description>` (e.g. `Task_UseFly`)

**Variables:**
- `lowerCamelCase` for locals and struct fields
- Globals: `g<Name>` (e.g. `gMain`, `gTasks`)

**Constants / Macros:**
- `SCREAMING_SNAKE_CASE` (e.g. `MOVE_TACKLE`, `SPECIES_PIKACHU`, `BATTLE_TYPE_WILD`)

## Where to Add New Code

**New Pokémon species:**
- Data: `src/data/pokemon/` (species_info, level_up_learnsets, etc.)
- Constants: `include/constants/species.h`
- See: `docs/tutorials/how_to_new_pokemon_*.md`

**New move:**
- Data: `src/data/moves_info.h`, `src/data/battle_move_effects.h`
- Constants: `include/constants/moves.h`
- Logic: `src/battle_script_commands.c` (if new effect)
- See: `docs/tutorials/how_to_new_move.md`

**New trainer:**
- Data: `src/data/trainers.h` (from `.party` file via trainerproc)
- Constants: `include/constants/trainers.h`
- See: `docs/tutorials/how_to_trainer_party_pool.md`

**New map script:**
- Script: `data/scripts/<MapName>.inc`
- Map header references script in `data/maps/` JSON

**New item:**
- Data: `src/data/items.h`
- Constants: `include/constants/items.h`

**New battle script command:**
- Macro: `asm/macros/battle_script.inc`
- Handler: `src/battle_script_commands.c`
- See: `docs/tutorials/how_to_battle_script_command_macro.md`

**New battle AI flag:**
- Constants: `include/constants/battle_ai.h`
- Logic: `src/battle_ai_main.c`, `src/battle_ai_switch_items.c`

**New screen/menu:**
- New .c file in `src/` (e.g. `my_screen.c`)
- Use `SetMainCallback2(CB2_MyScreen)` to enter
- Use `gMain.state` for multi-step init; `RunTasks()` in update loop

**New field effect:**
- Constants: `include/constants/field_effects.h`
- Logic: `src/field_effect.c` or `src/fldeff_*.c`
- Data: `src/data/field_effects/` if needed

## Special Directories

**build/:**
- Purpose: Object files, linked ELF, ROM
- Generated: Yes (`build/modern/`, `build/modern-test/`, `build/modern-debug/`)
- Committed: No

**data/maps/:**
- Purpose: Map headers, connections, events (some generated from JSON)
- Generated: Partially (connections.inc, events.inc, header.inc from mapjson)
- Committed: Mixed

**sound/songs/*.s:**
- Purpose: Assembled music
- Generated: Yes (from .mid via mid2agb)
- Committed: No (in .gitignore typically)

**src/data/trainers.h:**
- Purpose: Trainer party data
- Generated: Yes (from `trainers.party` or `*.party` via trainerproc when `COMPETITIVE_PARTY_SYNTAX=1`)
- Committed: Project-dependent

---

*Structure analysis: 2025-02-27*
