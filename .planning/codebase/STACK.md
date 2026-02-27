# Technology Stack

**Analysis Date:** 2025-02-27

## Languages

**Primary:**
- C (GNU C17 / gnu17) — Game logic, battle engine, overworld, menus, data tables in `src/`
- ARM assembly (Thumb) — Low-level routines, BIOS syscall wrappers, entry points in `asm/`, `data/`

**Secondary:**
- C++11/17 — Build tools: `tools/jsonproc`, `tools/mapjson`, `tools/preproc`, `tools/scaninc`, `tools/mid2agb`, `tools/ramscrgen`
- C (C11) — Build tools: `tools/gbagfx`, `tools/rsfont`, `tools/aif2pcm`, `tools/gbafix`, `tools/bin2c`, `tools/trainerproc`
- Python 3 — Learnset helper (`tools/learnset_helpers/teachable.py`), migration scripts, dev scripts, docs link fixer
- JSON — Map data (`data/maps/*/map.json`), config templates, learnset reference data (`tools/learnset_helpers/porymoves_files/*.json`)

## Runtime

**Target:** Game Boy Advance (ARM7TDMI, ARMv4T)
- No OS; bare-metal firmware
- Memory: EWRAM 256KB, IWRAM 32KB, ROM up to 32MB
- Linker script: `ld_script_modern.ld`

**Build host:** Linux (WSL recommended on Windows), macOS; CI uses `ubuntu-latest` container

## Package Manager

**None:** No npm, pip, cargo, etc. for the game runtime. Dependencies are:
- System libraries (libpng, libz) via `pkg-config` for build tools
- ARM toolchain (arm-none-eabi-gcc) via system package manager or DevkitARM

## Frameworks

**Core:**
- pret pokeemerald decompilation base — Engine architecture

**Build / dev:**
- GNU Make — Build system (`Makefile`, `make_tools.mk`, `graphics_file_rules.mk`, `map_data_rules.mk`, `audio_rules.mk`, `json_data_rules.mk`, `spritesheet_rules.mk`)
- mdBook — Documentation (`docs/book.toml`, `docs/SUMMARY.md`)

**Testing:**
- Custom test runner — Tests run inside mGBA; `test/test_runner.c`, `test/test_runner_battle.c`

## Key Dependencies

**Target (game ROM):**
- `libagbsyscall` — GBA BIOS syscall wrappers (`libagbsyscall/`, `libagbsyscall.s`)
- `libgcc`, `libc`, `libnosys` — ARM toolchain runtime libraries

**Build tools:**
- libpng — PNG conversion for graphics (`tools/gbagfx`, `tools/rsfont`)
- libz — Compression (`tools/gbagfx`)
- nlohmann/json — JSON parsing in `tools/jsonproc` (header-only)
- inja — Template engine (`tools/jsonproc/inja.hpp`) for JSON → C/header generation

## Configuration

**Build:**
- `Makefile` — ROM name, build dirs, toolchain, C flags
- `make_tools.mk` — Tool definitions
- `DEVKITARM` — Optional env var for toolchain path; defaults to `arm-none-eabi-*` if unset

**Game:**
- `include/config/*.h` — Feature toggles: `battle.h`, `pokemon.h`, `ai.h`, `overworld.h`, `item.h`, `save.h`, `general.h`, `debug.h`, `caps.h`

**Environment:**
- No `.env` or runtime config files; behavior is compile-time via `#define` in config headers

## Build System

**Toolchain:**
- `arm-none-eabi-gcc` (DevkitARM or system install)
- `arm-none-eabi-as`, `arm-none-eabi-ld`, `arm-none-eabi-objcopy`, `arm-none-eabi-objdump`
- `arm-none-eabi-cpp` for preprocessing

**Flags:**
- `-mthumb -mthumb-interwork -std=gnu17 -O2 -march=armv4t -mtune=arm7tdmi -mabi=apcs-gnu`
- `-Werror -Wall -Wno-strict-aliasing -Wno-attribute-alias -Woverride-init`

**Build pipeline:**
1. `make tools` — Build native tools (gbagfx, preproc, scaninc, jsonproc, mapjson, etc.)
2. `make generated` — Generate map JSON sources, headers, etc.
3. C source → preproc (charmap) → cc1 → as → .o
4. Link with `ld_script_modern.ld` → ELF → objcopy → gbafix → ROM

**Output:**
- `pokeemerald.gba` — ROM
- `pokeemerald.elf` — Debug ELF
- `build/modern/` — Object files
- `build/modern-test/` — Test build objects
- `build/modern-debug/` — Debug build objects

## Platform Requirements

**Development:**
- WSL (Ubuntu) on Windows or native Linux/macOS
- `build-essential`, `binutils-arm-none-eabi`, `gcc-arm-none-eabi`, `libnewlib-arm-none-eabi`, `libpng-dev`, `python3`, `pkg-config`
- Optional: `DEVKITARM` (DevkitPro) for full toolchain

**Production:**
- GBA ROM (`.gba`); runs on real hardware or emulators (mGBA, VBA, etc.)

---

*Stack analysis: 2025-02-27*
