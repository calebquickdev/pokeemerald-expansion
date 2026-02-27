# External Integrations

**Analysis Date:** 2025-02-27

## APIs & External Services

**None.** This is a GBA ROM hack. There are no HTTP APIs, cloud services, or network calls. The game runs entirely offline on the device.

## Data Storage

**Databases:**
- None. All data is stored in ROM (static tables) or in SRAM (save file) on the cartridge.

**File Storage:**
- Local filesystem only during build. Assets (PNG, MIDI, AIF, JSON) are converted to binary formats and embedded in the ROM.

**Caching:**
- None.

## Authentication & Identity

**Auth Provider:**
- Not applicable. No user accounts or auth.

## Monitoring & Observability

**Error Tracking:**
- None. Optional mGBA debug output when `LOG_HANDLER == LOG_HANDLER_MGBA_PRINT` in `include/config/general.h` (`MgbaPrintf`, `DebugPrintfLevel` in `src/libisagbprn.c`).

**Logs:**
- Optional mGBA debug output via GBA debug registers. Not used in production builds.

## CI/CD & Deployment

**Hosting:**
- GitHub Actions — `.github/workflows/build.yml`, `.github/workflows/docs.yml`

**CI Pipeline:**
- Container: `devkitpro/devkitarm`
- Steps: `sudo apt install build-essential libpng-dev libelf-dev`, `make -j${nproc} -O all`, `make check` (test suite)
- Docs: `mdbook build` with latest mdbook release from GitHub

**Deployment:**
- ROM is built artifact; users run `make` locally and load the resulting `.gba` in an emulator or flash to a cartridge.

## Environment Configuration

**Required env vars:**
- `DEVKITARM` — Optional; path to DevkitARM toolchain. If unset, system `arm-none-eabi-*` binaries are used.

**Secrets location:**
- None. No secrets required.

## Webhooks & Callbacks

**Incoming:**
- None.

**Outgoing:**
- None.

## External Tools & Libraries (Build-Time)

**Emulators:**
- **mGBA** — Primary test runner. `make check` uses `mgba-rom-test` (or `tools/mgba/mgba-rom-test`) to run the test suite. `tools/mgba-rom-test-hydra` runs multiple mGBA processes in parallel. Source: <https://github.com/mgba-emu/mgba>.
- Prebuilt mGBA binaries: `tools/mgba/` (Windows, Linux; see `tools/mgba/README.md`).

**Build Tools (built from source in `tools/`):**
| Tool | Purpose |
|------|---------|
| `gbagfx` | PNG → 1bpp/4bpp/8bpp, LZ, RLE compression; palette conversion |
| `aif2pcm` | AIF audio → PCM `.bin` |
| `mid2agb` | MIDI → GBA music assembly |
| `scaninc` | Dependency scanning for C/asm |
| `preproc` | Charmap preprocessing for strings |
| `ramscrgen` | Symbol/BSS generation for linker |
| `gbafix` | ROM header fix, checksum |
| `mapjson` | Map JSON → assembly inc files |
| `jsonproc` | JSON + Inja template → C/header |
| `trainerproc` | Trainer party `.party` → C header |
| `patchelf` | Patch ELF for test runner args |
| `mgba-rom-test-hydra` | Parallel test runner wrapper |

**Third-Party Libraries (header-only, vendored):**
- `inja` — Template engine in `tools/jsonproc/inja.hpp`
- `nlohmann/json` — JSON parser in `tools/jsonproc` (header-only)

**Reference Data (JSON, not APIs):**
- `tools/learnset_helpers/porymoves_files/*.json` — Learnset data from Porymoves (hgss, bw, b2w2, ptd, oras, lgpe, c) for teachable move generation when `P_LEARNSET_HELPER_TEACHABLE` is enabled.

## Documentation

**mdBook:**
- `docs/book.toml` — mdBook config
- `docs/fix_links.py` — Preprocessor for link fixes
- `docs/SUMMARY.md` — Book structure
- Built and served via `mdbook build` / `mdbook serve`; CI uses latest mdbook from GitHub releases.

---

*Integration audit: 2025-02-27*
