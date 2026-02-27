# Codebase Concerns

**Analysis Date:** 2025-02-27

## Tech Debt

**Battle TV scores:**
- Issue: ~100 move effects have `battleTvScore = 0` with `// TODO: Assign points`
- Files: `src/data/battle_move_effects.h`
- Impact: Battle TV feature lacks proper scoring for most moves
- Fix approach: Assign appropriate `battleTvScore` values per move effect; reference existing non-zero entries for scale

**MonSpritesGfxManager memory waste:**
- Issue: `spritePointers` allocated as `numSprites * 32` when only `* 4` necessary; only 1 of 4 sprite positions used; unnecessarily large sizes
- Files: `src/pokemon.c` (lines 6331–6470)
- Impact: Wastes heap (112KB total in `gHeap`); increases fragmentation risk
- Fix approach: Change `AllocZeroed(gfx->numSprites * 32)` to `* 4`; consider removing unused `MON_SPR_GFX_MODE_FULL_PARTY` and `MON_SPR_GFX_MODE_BATTLE` paths

**Librfu / RFU TODOs:**
- Issue: `REG_RCNT = 0x100; // TODO: mystery bit?`; `// TODO: Make an enum for these`
- Files: `src/librfu_stwi.c` (lines 43, 486)
- Impact: Unclear behavior; magic numbers in wireless code
- Fix approach: Document or remove mystery bit; replace magic values with enum

**Graphics / asset TODOs:**
- Issue: `// TODO: These should also be combined into a single image, not matching for some reason`
- Files: `src/fldeff_misc.c` (line 71)
- Issue: `// TODO: use width 9 and makefile rule for cleanliness, make wasnt behaving`
- Files: `src/graphics.c` (line 1864)
- Impact: Duplicate assets; non-standard build rules
- Fix approach: Consolidate images; add proper makefile rule for pokenav header

**Script / region map TODOs:**
- Issue: `// TODO: Find a better way to assign a random gender`; `// TODO: probably needs a better name`
- Files: `src/script_pokemon_util.c` (line 501), `src/region_map.c` (line 1608)
- Impact: Minor; gender assignment and naming could be clearer

## Known Bugs

**Fieldmap mapView bounds overflow:**
- Symptoms: Loop iterates 0x200 times over `mapView` array of size 0x100; reads past end
- Files: `src/fieldmap.c` (lines 444–452)
- Trigger: `IsMapViewEmpty()` when `UBFIX` not defined
- Workaround: `UBFIX` is enabled when `MODERN` or `BUGFIX`; `include/config/general.h` defines both by default
- Fix: Already fixed behind `#ifdef UBFIX`; ensure UBFIX always defined for release builds

**Easy Chat unlockedTrendySayings clear bug:**
- Symptoms: Loop clears 64 bytes instead of 64 bits; corrupts Mauville old man data
- Files: `src/easy_chat.c` (lines 5585–5596)
- Trigger: `SetUnlockedEasyChatGroups()` when `UBFIX` not defined
- Workaround: UBFIX enabled by default; bug has no gameplay effect because Mauville data is reinitialized after
- Fix: Already fixed behind `#ifdef UBFIX`

**Region map dive display bug:**
- Symptoms: Map incorrectly displays "Route 129" when diving on Route 125 (Marine Cave)
- Files: `src/region_map.c` (lines 137–140)
- Trigger: Diving on Route 125
- Workaround: `#ifdef BUGFIX` changes behavior; BUGFIX enabled by default
- Fix: Apply BUGFIX path or equivalent fix

**Cable car Zigzagoon never used:**
- Symptoms: `ARRAY_COUNT - 1` excludes last Zigzagoon from use
- Files: `src/cable_car.c` (lines 883–884)
- Trigger: Cable car scene
- Fix: Use `ARRAY_COUNT` without `- 1` when BUGFIX defined

## Security Considerations

**GBA ROM hack:**
- Risk: No network stack; save data is local
- Files: `src/load_save.c`, `src/save.c`
- Current mitigation: Save encryption; checksum validation
- Recommendations: N/A for typical single-player ROM hack use

**Heap allocation failures:**
- Risk: `Alloc`/`AllocZeroed` can return NULL; many call sites do not check
- Files: `src/scrcmd.c`, `src/union_room.c`, `src/easy_chat.c`, and ~30 other files using `malloc.h`
- Current mitigation: Some screens check and exit; others may dereference NULL
- Recommendations: Audit critical paths (battle, save, script) for NULL checks; add fallbacks or graceful exit

## Performance Bottlenecks

**Heap allocation in battle:**
- Problem: `Alloc` used in battle flow (Forewarn, AllocSaveBattleMons)
- Files: `src/battle_util.c` (line 4081), `src/battle_ai_util.c` (lines 3593–3642)
- Cause: Heap fragmentation; alloc during battle can stall
- Improvement path: Pre-allocate battle buffers in EWRAM or use static arrays for hot paths

**EWRAM usage:**
- Problem: 256KB EWRAM shared by heap (112KB), save blocks, and many `EWRAM_DATA` statics
- Files: `include/malloc.h` (HEAP_SIZE 0x1C000), `src/load_save.c`, `ld_script_modern.ld`
- Cause: Heap + save + UI buffers compete for same 256KB
- Improvement path: Profile EWRAM usage; consider reducing heap or moving rarely used data to ROM

**IWRAM pressure:**
- Problem: 32KB IWRAM holds `.bss`, `common_data`, and time-critical code
- Files: `constants/gba_constants.inc` (IWRAM 0x8000 bytes), `ld_script_modern.ld`
- Cause: Large battle structs and AI buffers in IWRAM
- Improvement path: Annotate hot paths with `IWRAM_CODE`; move cold data to EWRAM where possible

## Fragile Areas

**UBFIX / BUGFIX conditional code:**
- Files: 50+ files with `#ifdef UBFIX` or `#ifdef BUGFIX`
- Why fragile: Disabling these can reintroduce UB and bugs; some paths only safe with MODERN compiler
- Safe modification: Do not disable BUGFIX or UBFIX in `include/config/general.h`; add new fixes behind same guards
- Test coverage: Tests in `test/sprite.c` define UBFIX; `make check` exercises some paths

**Event object movement:**
- Files: `src/event_object_movement.c` (~10k+ lines)
- Why fragile: Large, complex sprite/tile logic; many BUGFIX/UBFIX blocks; `ReallocSpriteTiles` and sheet handling
- Safe modification: Follow existing patterns; run overworld tests; avoid changing `ReallocSpriteTiles` without tests
- Test coverage: Limited; no dedicated overworld movement tests

**Pokemon storage system:**
- Files: `src/pokemon_storage_system.c` (~9.8k lines)
- Why fragile: Large EWRAM usage; box/party logic; BUGFIX/UBFIX in critical paths
- Safe modification: Test PC and box operations; verify save/load after changes
- Test coverage: No dedicated storage tests in `test/`

**Battle AI switch items:**
- Files: `src/battle_ai_switch_items.c` (2353 lines)
- Why fragile: Large AI decision logic; many item-specific branches
- Safe modification: Add or extend battle AI tests; verify AI behavior in doubles/singles
- Test coverage: `test/battle/ai/` has some coverage

## Scaling Limits

**ROM size:**
- Current: Build output up to 32MB
- Limit: `constants/gba_constants.inc` `ROM_SIZE = 0x2000000`; `ld_script_modern.ld` `LENGTH = 32M`
- Scaling path: Cannot exceed 32MB on GBA; reduce assets (cries, sprites, music) or disable features via `include/config/`

**Species / items / moves:**
- Limits: `NATIONAL_DEX_COUNT`, `ITEM_COUNT`, `MOVES_COUNT` in `include/constants/`
- Impact: Adding species/items/moves increases ROM and table sizes
- Scaling path: Use `include/config/species_enabled.h` to disable families; consider data compression

**Heap:**
- Current: 112KB (`HEAP_SIZE` in `include/malloc.h`)
- Limit: Fragmentation; large simultaneous allocations can fail
- Scaling path: Reduce per-screen allocations; pool common buffer sizes

## Dependencies at Risk

**Alloc / Free usage:**
- Risk: Custom heap in EWRAM; no guard pages; corruption can cascade
- Impact: Crashes or undefined behavior if heap corrupted
- Migration plan: Add heap validation in debug builds; consider static pools for critical subsystems

## Missing Critical Features

**Battle TV scoring:**
- Problem: Most moves have `battleTvScore = 0`
- Blocks: Meaningful Battle TV rankings for many moves

**Overworld script coverage:**
- Problem: Map scripts in `data/scripts/` not unit-tested
- Blocks: Safe refactors of event scripts

## Test Coverage Gaps

**Overworld / field:**
- What's not tested: Map scripts, field effects, movement, PC/storage
- Files: `data/scripts/*.inc`, `src/field_*.c`, `src/pokemon_storage_system.c`
- Risk: Regressions in story and exploration
- Priority: Medium

**Save / load:**
- What's not tested: Save structure changes, migration, corruption recovery
- Files: `src/load_save.c`, `src/save.c`
- Risk: Save corruption or loss
- Priority: High

**Union room / wireless:**
- What's not tested: Multiplayer, RFU, union room flows
- Files: `src/union_room.c`, `src/librfu_*.c`
- Risk: Link/multiplayer bugs
- Priority: Low (hard to automate)

## Migration Concerns

**Version upgrade scripts:**
- Location: `migration_scripts/` (10 Python scripts for 1.7→1.8, 1.8→1.9, 1.9→1.10, 1.10→1.11)
- Risk: Forks that add custom data (trainers, items, maps) may need manual migration
- Fix approach: Run scripts from `migration_scripts/README.md`; extend scripts for custom tables; test save compatibility
- Key scripts: `convert_trainer_parties.py`, `convert_item_icons.py`, `egg_move_refactor.py`, `convert_battle_frontier_trainers.py`

**Save block layout:**
- Risk: Adding fields to save blocks breaks old saves
- Files: `include/constants/save.h`, `src/load_save.c`
- Fix approach: Use save versioning; provide migration in load path for new fields

## Hardcoded Limits

**Game constants (from `include/constants/global.h`):**
- `PARTY_SIZE` = 6
- `MAX_MON_MOVES` = 4
- `BAG_ITEMS_COUNT` = 30
- `MAX_BATTLERS_COUNT` = 4 (from `include/constants/battle.h`)
- `POKEMON_SLOTS_NUMBER` = `NATIONAL_DEX_COUNT + 1` (from `include/constants/pokedex.h`)

**Memory:**
- `HEAP_SIZE` = 0x1C000 (112KB) in `include/malloc.h`
- EWRAM = 256KB, IWRAM = 32KB in `constants/gba_constants.inc` and `ld_script_modern.ld`

**Config-driven limits:**
- Species, items, moves, and mechanics caps in `include/config/` (`pokemon.h`, `battle.h`, `caps.h`, etc.)
- Prefer editing config headers over engine source when changing limits

---

*Concerns audit: 2025-02-27*
