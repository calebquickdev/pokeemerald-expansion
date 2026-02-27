# Coding Conventions

**Analysis Date:** 2025-02-27

## Naming Patterns

**Functions and structs:**
- `UpperCamelCase` (e.g. `BattleScriptCmd_AttackAnimation`, `GetSwitchChance`, `InitializeSwitchinCandidate`)
- Source: `.cursor/rules/c-conventions.mdc`, observed in `src/battle_ai_switch_items.c`, `src/battle_util.c`

**Constants and macros:**
- `SCREAMING_SNAKE_CASE` (e.g. `MOVE_TACKLE`, `SPECIES_PIKACHU`, `ABILITY_WATER_ABSORB`, `TYPE_WATER`)
- Source: `.cursor/rules/c-conventions.mdc`, `include/constants/` headers

**Local variables and struct fields:**
- `lowerCamelCase` (e.g. `minCost`, `minCostProcess`, `battleStringId`, `shouldSwitchScenario`)
- Source: `.cursor/rules/c-conventions.mdc`, observed in `test/test_runner.c`, `src/battle_ai_switch_items.c`

**IDs and constants:**
- Never hardcode numeric IDs — always use named constants from `include/constants/` (e.g. `include/constants/moves.h`, `include/constants/species.h`, `include/constants/items.h`)

## Code Style

**Formatting:**
- No ESLint/Prettier/Biome — C codebase for GBA
- Compiler: `arm-none-eabi-gcc` with `-std=gnu17 -Werror -Wall`
- Key flags: `-mthumb -mthumb-interwork -O2 -march=armv4t -mtune=arm7tdmi`

**Linting:**
- `-Werror` treats warnings as errors
- `-Wno-strict-aliasing -Wno-attribute-alias -Woverride-init` for compatibility
- Optional: `make ANALYZE=1` enables `-fanalyzer` for undefined behavior analysis

## Import Organization

**Order:**
1. `global.h` first (common in `src/` and `test/`)
2. Feature-specific headers (e.g. `battle.h`, `item.h`)
3. `constants/` headers
4. Test headers (`test/battle.h`, `test/test.h`)

**Example from `test/battle/ability/water_absorb.c`:**
```c
#include "global.h"
#include "test/battle.h"
```

## GBA-Specific Constraints

**No heap allocation in performance-critical or battle code:**
- Use static or stack allocation instead
- Custom allocator exists: `include/malloc.h` defines `Alloc`, `AllocZeroed`, `Free` (backed by `gHeap` in `src/malloc.c`)
- `malloc`/`free` from stdlib do not exist; the project uses `Alloc`/`Free` for non-critical paths (menus, decompression, etc.)
- Avoid `malloc` in battle engine and hot paths — `.cursor/rules/project-overview.mdc`, `.cursor/rules/c-conventions.mdc`

**No standard I/O:**
- No `printf`, `puts`, `scanf` — GBA has no OS
- Use `ConvertIntToDecimalStringN` and similar from `string_util.h` for string formatting
- `mini_printf` in `src/mini_printf.c` provides minimal `snprintf`-style formatting for limited use
- Tests use `Test_MgbaPrintf` for mGBA debug output

**No threads:**
- Single-threaded with task/callback system
- Use `CreateTask` / `DestroyTask` for long-running processes
- Use `SetMainCallback2` / `gMain.callback2` for menu and overworld state

**Fixed call stack:**
- No recursion in battle or overworld code — stack size is fixed

## Memory Sections

**IWRAM (32KB fast internal RAM):**
- Annotate time-critical functions with `IWRAM_CODE` (places code in `.iwram.code` section)
- Use sparingly — IWRAM is precious
- Assembly uses `.section .iwram.code` (e.g. `src/crt0.s`, `src/m4a_1.s`)
- C equivalent: `__attribute__((section(".iwram.code")))` — documented in `.cursor/rules/c-conventions.mdc`

**EWRAM (256KB external work RAM):**
- Default for most data
- Use `EWRAM_DATA` for large or frequently accessed data: `__attribute__((section(".sbss")))`
- Examples: `src/string_util.c`, `src/trainer_hill.c`, `src/trade.c`

**IWRAM_DATA:**
- `__attribute__((section(".bss")))` — places data in IWRAM
- Use sparingly for hot data

## Types

**Use fixed-width types from `include/gba/types.h`:**
- `u8`, `u16`, `u32`, `s8`, `s16`, `s32`, `bool8`, `bool16`, `bool32`
- Avoid `int`/`long` without explicit width when size matters for ROM data layout

## Error Handling

**Strategy:** No exceptions; use return values and early returns.

**Patterns:**
- Functions return `bool32` or status codes for success/failure
- `Test_ExitWithResult` in tests for failures
- Avoid nesting; use early returns and `continue` for exit conditions

## Logging

**Framework:** No standard logging. Debug output via mGBA debug protocol in tests (`Test_MgbaPrintf`).

**Patterns:**
- Tests: use `Test_MgbaPrintf` for diagnostic output
- Production: no `printf`; use in-game UI or debug builds

## Comments

**When to comment:**
- Function-level documentation for non-obvious behavior
- Document assumptions in tests with `ASSUME`
- Avoid unnecessary or verbose comments; target advanced programmers

## Function Design

**Size:** Prefer small, focused functions. Large files exist (e.g. `src/battle_ai_switch_items.c` ~2353 lines) — split when adding new logic.

**Parameters:** Use `u32`, `u16`, struct pointers as appropriate. Avoid `int` for IDs.

**Return values:** Use `bool32` for predicates, `u32`/`s32` for counts/IDs, or void for side effects.

## Module Design

**Exports:** Functions used across translation units are declared in headers; avoid `static` for functions referenced from data tables or other TUs.

**Barrel files:** Not used; include specific headers.

## Static Functions

- Do not add `static` to functions referenced from data tables or other translation units
- Use `static` for file-local helpers

---

*Convention analysis: 2025-02-27*
