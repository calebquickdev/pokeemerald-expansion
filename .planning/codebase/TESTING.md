# Testing Patterns

**Analysis Date:** 2025-02-27

## Test Framework

**Runner:**
- mGBA ROM test system — tests run on actual GBA hardware emulation
- `make check` builds `pokeemerald-test.elf` and runs it via `mgba-rom-test` or `mgba-rom-test-hydra`
- Hydra wrapper: `tools/mgba-rom-test-hydra/mgba-rom-test-hydra` runs multiple mGBA processes in parallel

**Assertion library:**
- Custom macros in `include/test/test.h` and `include/test/battle.h`
- `EXPECT`, `EXPECT_EQ`, `EXPECT_NE`, `EXPECT_LT`, `EXPECT_LE`, `EXPECT_GT`, `EXPECT_GE`, `EXPECT_MUL_EQ`
- Battle-specific: `ABILITY_POPUP`, `ANIMATION`, `HP_BAR`, `MESSAGE`, `STATUS_ICON`, etc.

**Run commands:**
```bash
make check              # Run all tests
make check -j           # Run with parallel jobs (recommended)
make check TESTS="Spikes"   # Run tests whose name starts with "Spikes"
make pokeemerald-test.elf TESTS="Spikes"   # Build test ROM for manual inspection in mGBA
```

## Test File Organization

**Location:**
- `test/` — root for all tests
- `test/battle/` — battle mechanics (abilities, move effects, hold effects, AI, etc.)
- `test/battle/ability/` — ability tests (e.g. `water_absorb.c`, `adaptability.c`)
- `test/battle/move_effect/` — move effect tests (e.g. `belly_drum.c`, `brine.c`)
- `test/battle/move_effect_secondary/` — secondary effects
- `test/battle/move_flags/` — move flag behavior
- `test/battle/hold_effect/` — held item effects
- `test/battle/ai/` — AI behavior tests
- `test/battle/form_change/` — Mega Evolution, Primal Reversion, etc.
- `test/battle/weather/` — weather tests
- `test/battle/status1/` — status condition tests
- `test/` (root) — non-battle tests (e.g. `text.c`, `species.c`, `pokemon.c`)

**Naming:**
- Test files: `snake_case.c` matching the mechanic (e.g. `water_absorb.c`, `belly_drum.c`)
- Test names: descriptive strings, prefixed by mechanic for filtering (e.g. "Water Absorb heals 25% when hit by water type moves")

**Structure:**
```
test/
├── test_runner.c           # Main test runner entry, process assignment
├── test_runner_battle.c    # Battle test runner logic
├── text.c                  # UI string/layout tests
├── species.c
├── pokemon.c
└── battle/
    ├── ability/
    ├── move_effect/
    ├── move_effect_secondary/
    ├── move_flags/
    ├── hold_effect/
    ├── ai/
    ├── form_change/
    ├── weather/
    └── ...
```

## Test Structure

**Battle tests (GIVEN / WHEN / SCENE):**
```c
#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Water Absorb heals 25% when hit by water type moves")
{
    GIVEN {
        ASSUME(GetMoveType(MOVE_BUBBLE) == TYPE_WATER);
        PLAYER(SPECIES_POLIWAG) { Ability(ABILITY_WATER_ABSORB); HP(1); MaxHP(100); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_BUBBLE); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_WATER_ABSORB);
        HP_BAR(player, damage: -25);
        MESSAGE("Poliwag restored HP using its Water Absorb!");
    }
}
```

**Function tests (generic TEST macro):**
```c
#include "global.h"
#include "test/test.h"

TEST("Move names fit on Pokemon Summary Screen")
{
    u32 i;
    const u32 fontId = FONT_NARROWER, widthPx = 72;
    u32 move = MOVE_NONE;
    for (i = 1; i < MOVES_COUNT; i++)
    {
        PARAMETRIZE_LABEL("%S", GetMoveName(i)) { move = i; }
    }
    EXPECT_LE(GetStringWidth(fontId, GetMoveName(move), 0), widthPx);
}
```

**Patterns:**
- `ASSUMPTIONS` block at file top for shared prerequisites
- `ASSUME(cond)` to skip test if prerequisite fails (documents assumptions)
- `PARAMETRIZE` for running the same test with different parameters
- `FINALLY` for checks after all parameters (e.g. `EXPECT_MUL_EQ` for damage ratios)
- `KNOWN_FAILING` for tests that document known bugs
- `TO_DO_BATTLE_TEST` for placeholder tests

## Mocking

**Framework:** No traditional mocks. Tests run against the real game engine in headless mGBA.

**Patterns:**
- RNG is rigged: moves always hit, never critical, secondary effects always activate (unless overridden)
- `RNGSeed(seed)` for deterministic runs (avoid when possible — fragile)
- `WITH_RNG(tag, value)` in `MOVE` for explicit RNG overrides
- `FLAG_SET(flagId)` to set flags in `GIVEN`
- `WITH_CONFIG(configTag, value)` for config overrides

**What to mock:** Not applicable — full integration against game code.

**What NOT to mock:** Nothing; tests use real battle engine, items, moves, abilities.

## Fixtures and Factories

**Test data:**
- `PLAYER(species)` / `OPPONENT(species)` with customization: `Ability`, `Item`, `Moves`, `HP`, `MaxHP`, `Level`, `Status1`, etc.
- `SPECIES_WOBBUFFET` commonly used as neutral test subject
- Constants from `include/constants/` (moves, items, abilities, species)

**Location:** Inline in test files; no separate fixture directory.

## Coverage

**Requirements:** None enforced. Tests are added per feature; battle mechanics have extensive coverage.

**View coverage:** Not applicable — no coverage tooling for GBA.

## Test Types

**Unit tests:**
- Function tests via `TEST()` — e.g. `text.c` checks string widths, `species.c` checks species data
- Run in same mGBA environment but without full battle setup

**Integration tests:**
- Battle tests via `SINGLE_BATTLE_TEST`, `DOUBLE_BATTLE_TEST`, `WILD_BATTLE_TEST`, `AI_SINGLE_BATTLE_TEST`, `AI_DOUBLE_BATTLE_TEST`
- Full battle simulation; assertions check player-visible output (animations, messages, HP bars, status icons)
- Prefer checking observable output over internal state for refactoring robustness

**E2E tests:** Not used — tests run in headless mGBA, not full game flow.

## Common Patterns

**Parametrized damage comparison:**
```c
SINGLE_BATTLE_TEST("Meditate raises Attack", s16 damage)
{
    bool32 raiseAttack;
    PARAMETRIZE { raiseAttack = FALSE; }
    PARAMETRIZE { raiseAttack = TRUE; }
    GIVEN { ... }
    WHEN { ... }
    SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.5), results[1].damage);
    }
}
```

**Randomness testing:**
```c
PASSES_RANDOMLY(25, 100, RNG_PARALYSIS);
GIVEN { PLAYER(SPECIES_WOBBUFFET) { Status1(STATUS1_PARALYSIS); } ... }
```

**Error testing:**
- `ASSUME` causes `TEST_RESULT_ASSUMPTION_FAIL` (skipped)
- `EXPECT` / `EXPECT_EQ` etc. cause `TEST_RESULT_FAIL` via `Test_ExitWithResult`

## Build Configuration

**Test build:**
- `make check` sets `TEST=1`, uses `build/modern-test/` for objects
- `ld_script_test.ld` — test-specific linker script; entry point is `CB2_TestRunner`
- `gInitialMainCB2 = CB2_TestRunner` in test ld script
- Tests collected via `__attribute__((section(".tests")))`; `__start_tests` / `__stop_tests` in `test_runner.c`

**Documentation:**
- `docs/tutorials/how_to_testing_system.md` — full reference
- `include/test/battle.h` — extensive inline documentation for battle test DSL

---

*Testing analysis: 2025-02-27*
