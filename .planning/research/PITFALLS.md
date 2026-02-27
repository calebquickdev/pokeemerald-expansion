# Domain Pitfalls: Dynamax Raid Dens

**Domain:** GBA ROM hack (pokeemerald-expansion) — Dynamax Raid Dens system
**Researched:** 2025-02-27
**Confidence:** HIGH (codebase inspection); MEDIUM (OAM/sprite estimates)

---

## Critical Pitfalls

### Pitfall 1: Battler Count Hardcoded at 4

**What goes wrong:** Raid battles need 5 battlers (4 allies + 1 boss). The engine uses `MAX_BATTLERS_COUNT = 4` everywhere. `gBattleMons`, `gBattlerPartyIndexes`, `gHealthboxSpriteIds`, and 50+ other arrays are fixed-size with `MAX_BATTLERS_COUNT`. `B_POSITION_*` macros (0–3) and `BATTLE_OPPOSITE`/`BATTLE_PARTNER` assume 2×2 layout.

**Why it happens:** Engine was designed for singles/doubles only.

**Consequences:** Out-of-bounds access, crashes, or wrong battler targeting if you add a 5th slot without a full engine change.

**Prevention:** Either (a) increase `MAX_BATTLERS_COUNT` to 5 and refactor all `B_POSITION_*`/`BATTLE_OPPOSITE`/`BATTLE_PARTNER` usage, or (b) model the raid as 4 battlers: 4 allies vs 1 boss, with the boss as a single “opponent” slot and the 4 allies on the player side. Option (b) avoids engine changes but requires custom UI layout and targeting logic.

**Detection:** Grep for `MAX_BATTLERS_COUNT`, `B_POSITION_`, `BATTLE_OPPOSITE`, `BATTLE_PARTNER`; `include/constants/battle.h` line 27.

**Phase:** Battle architecture / raid battle setup.

**Confidence:** HIGH

---

### Pitfall 2: OAM / Sprite Budget Exhaustion

**What goes wrong:** GBA has 128 OAM slots. `gMain.oamBuffer[128]` in `include/main.h`; `MAX_SPRITES = 64` in `include/sprite.h`. Battle uses: 4 health boxes (each with multiple sprites), 4 mon sprites, shadows, gimmick indicators, and animations. Battle interface alone has ~19 `CreateSprite`/`CreateMonPicSprite` calls. Lobby UI adds: 4 party icons, full-screen boss silhouette, trainer name, buttons, etc.

**Why it happens:** Lobby + battle can exceed 64–128 sprites if both are designed without a budget.

**Consequences:** Sprites disappear, flicker, or overwrite each other; undefined behavior.

**Prevention:**
- Count sprites per screen: lobby (icons, silhouette, UI) vs battle (health bars, mons, shadows, animations).
- Avoid showing lobby and battle sprites at once; transition cleanly.
- Reuse `CreateSprite` slots; destroy lobby sprites before entering battle.
- Reserve ~4–8 sprites per battler (health bar, mon, shadow, gimmick).

**Detection:** Audit `CreateSprite`, `CreateMonPicSprite`, `CreateMonIcon`; `src/battle_interface.c`, `src/battle_gfx_sfx_util.c`.

**Phase:** Lobby UI + battle sprite allocation.

**Confidence:** MEDIUM (estimates based on code patterns)

---

### Pitfall 3: Permanently Dynamaxed Boss Breaks End-of-Turn Logic

**What goes wrong:** Normal Dynamax ends after 3 turns via `gBattleStruct->dynamax.dynamaxTurns[battler]`. `UndoDynamax` is called when the timer hits 0. A raid boss is permanently Dynamaxed and never “ends” Dynamax. Code that assumes Dynamax always ends (e.g. `BS_UndoDynamax`, end-of-turn checks) may misbehave or never run.

**Why it happens:** `battle_dynamax.c` and `battle_script_commands.c` treat Dynamax as temporary.

**Consequences:** Boss may not get correct HP/stat handling; `GetNonDynamaxHP`/`GetNonDynamaxMaxHP` may be wrong; form-change logic may fail.

**Prevention:**
- Add raid boss flag: `if (gBattleTypeFlags & BATTLE_TYPE_RAID && IsRaidBoss(battler))` skip Dynamax end-of-turn logic.
- Ensure `GetActiveGimmick(battler) == GIMMICK_DYNAMAX` works for both temporary and permanent Dynamax.
- Use `ApplyDynamaxHPMultiplier` for raid boss; avoid `UndoDynamax` for the boss.

**Detection:** `src/battle_dynamax.c` (lines 201–220, 381–386); `src/battle_script_commands.c` (lines 4796–4800); search for `UndoDynamax`, `dynamaxTurns`.

**Phase:** Dynamax / raid battle integration.

**Confidence:** HIGH

---

### Pitfall 4: Save Block Extension Without Migration

**What goes wrong:** Adding raid den data to `SaveBlock1` or `SaveBlock2` changes struct layout. Old saves load with wrong offsets; checksums fail or data corrupts.

**Why it happens:** Save blocks are fixed-size structs; `include/load_save.h` uses `SAVEBLOCK_MOVE_RANGE` (128) for ASLR; `SECTOR_DATA_SIZE` is 3968 bytes per sector.

**Consequences:** Save corruption, lost progress, crashes on load.

**Prevention:**
- Add raid den data at end of struct (e.g. `SaveBlock2` or `SaveBlock1`).
- Use save versioning; in load path, check version and migrate if old.
- Follow `migration_scripts/` patterns; extend `convert_*.py` for new fields.
- Use `include/config/save.h` `FREE_*` flags to reclaim space if needed.

**Detection:** `include/load_save.h`, `include/global.h` (SaveBlock structs), `src/load_save.c`, `migration_scripts/`.

**Phase:** Save data design.

**Confidence:** HIGH

---

### Pitfall 5: Flag Exhaustion for Per-Den State

**What goes wrong:** Each den needs state (active/inactive, cleared today, etc.). Using one flag per den quickly consumes flags. `FLAGS_COUNT` ≈ 0x960 (from `DAILY_FLAGS_END + 1`); many `FLAG_UNUSED_*` exist; DAILY_FLAGS have many unused slots.

**Why it happens:** Flags are global; den count is unbounded.

**Consequences:** Running out of flags if you add too many dens; or reusing flags for unrelated features.

**Prevention:**
- Use a compact den state array in save data instead of one flag per den.
- Reserve a small flag range (e.g. `FLAG_RAID_DEN_ACTIVE_START` … `FLAG_RAID_DEN_ACTIVE_START + N`) for dens.
- Prefer vars or save-block fields for den-specific state (species, level, etc.).

**Detection:** `include/constants/flags.h` (lines 1572–1641); `FLAGS_COUNT`; `DAILY_FLAGS_*`.

**Phase:** Den state design.

**Confidence:** HIGH

---

### Pitfall 6: Heap Allocation During Battle

**What goes wrong:** `Alloc`/`AllocZeroed` can return NULL; many call sites do not check. Heap is 112KB; battle and lobby allocate sprites, buffers, etc. Fragmentation can cause failures.

**Why it happens:** `include/malloc.h` `HEAP_SIZE` 0x1C000; no guard pages.

**Consequences:** NULL dereference, crash, or corrupted state.

**Prevention:**
- Check `Alloc`/`AllocZeroed` return in lobby and battle paths.
- Pre-allocate raid-specific buffers in EWRAM or use static arrays where possible.
- Avoid large allocations during battle; use pools or reuse.

**Detection:** `src/battle_util.c`, `src/battle_gfx_sfx_util.c`.

**Phase:** Lobby + battle implementation.

**Confidence:** HIGH (from CONCERNS.md)

---

### Pitfall 7: IWRAM / EWRAM Pressure

**What goes wrong:** IWRAM: 32KB; EWRAM: 256KB. Battle structs, AI buffers, and common data live in IWRAM. Adding raid state (e.g. shield HP, turn limit, respawn flags) can overflow.

**Why it happens:** GBA limits; `ld_script_modern.ld` defines fixed sizes.

**Consequences:** Linker errors or runtime corruption if sections overflow.

**Prevention:**
- Put raid data in EWRAM; avoid IWRAM for non-critical raid state.
- Use `EWRAM_DATA` for raid structs; keep hot paths in IWRAM.

**Detection:** `ld_script_modern.ld`; `include/constants/gba_constants.inc`; `EWRAM_DATA` usage.

**Phase:** Architecture and data layout.

**Confidence:** HIGH

---

### Pitfall 8: Dynamax Energy / Raid-Specific Checks Incomplete

**What goes wrong:** `battle_dynamax.c` lines 116–117 and 241–242 have TODOs: `// TODO: Cannot Dynamax in a Max Raid if you don't have Dynamax Energy`; `// TODO: Certain moves banned in raids`. Raid-specific logic is not implemented.

**Why it happens:** Raid feature is partially stubbed.

**Consequences:** Players may Dynamax when they shouldn’t; banned moves may work in raids.

**Prevention:**
- Implement `gBattleStruct->raid.dynamaxEnergy` and `CanDynamax` raid check.
- Add raid move ban list in `IsMoveBlockedByDynamax` or equivalent.
- Implement `IsRaidBattle()` and wire it into these checks.

**Detection:** `src/battle_dynamax.c` (lines 116–117, 241–242).

**Phase:** Dynamax / raid battle integration.

**Confidence:** HIGH

---

## Confirmations

### Wishing Piece

**Status:** EXISTS.

- `include/constants/items.h` line 259: `#define ITEM_WISHING_PIECE 195`
- `src/data/items.h` lines 3565–3567: `[ITEM_WISHING_PIECE]` with `.name = _("Wishing Piece")`

No new item needed; wire Wishing Piece into den activation logic.

**Confidence:** HIGH

---

### OAM Budget Reality Check

| Screen        | Estimated Sprites | Notes                                              |
|---------------|-------------------|----------------------------------------------------|
| Lobby         | 15–25             | 4 icons, silhouette, UI elements, buttons         |
| Battle (4v1)  | 40–55             | 5 health boxes, 5 mons, shadows, gimmick, anims   |
| Combined      | 55–80             | If lobby and battle overlap; avoid if possible     |

GBA OAM: 128 slots. `MAX_SPRITES`: 64. Stay under 64 for lobby; destroy lobby sprites before battle. Battle alone should fit; animations add temporary sprites.

**Confidence:** MEDIUM (estimates)

---

### Battler Count Limit

**Current:** `MAX_BATTLERS_COUNT = 4` everywhere. `gBattleMons[4]`, `gBattlerPartyIndexes[4]`, etc. `B_POSITION_*` 0–3.

**For 5 battlers:** Must either increase `MAX_BATTLERS_COUNT` to 5 and refactor all `B_POSITION_*`/`BATTLE_OPPOSITE`/`BATTLE_PARTNER` usage, or model as 4 battlers: 4 allies vs 1 boss (boss as single opponent slot).

**Confidence:** HIGH

---

### Save Extension Safety

- Use `SaveBlock2` or `SaveBlock1` end; add at struct end.
- Add save version field; migrate on load if version < current.
- Use `include/config/save.h` `FREE_*` to reclaim space if needed.
- Keep `SECTOR_DATA_SIZE` (3968) in mind; ensure block fits in sectors.
- Use `migration_scripts/` for layout changes.

**Confidence:** HIGH

---

## Phase-Specific Warnings

| Phase Topic        | Likely Pitfall              | Mitigation                                      |
|--------------------|-----------------------------|-------------------------------------------------|
| Battle setup       | 5 battlers vs 4 engine      | Decide 4 vs 5 battlers; refactor if 5           |
| Lobby UI           | OAM overflow                | Count sprites; destroy before battle           |
| Dynamax boss       | End-of-turn logic           | Skip `UndoDynamax` for raid boss               |
| Save data          | Layout change               | Versioning + migration                         |
| Den state          | Flag exhaustion             | Per-den array in save block                     |
| Wishing Piece      | N/A                         | Item exists; use it                             |

---

## Sources

- `include/constants/battle.h` — MAX_BATTLERS_COUNT, B_POSITION_*, BATTLE_TYPE_RAID
- `include/sprite.h` — MAX_SPRITES
- `include/main.h` — oamBuffer[128]
- `include/load_save.h` — SaveBlock structure, SAVEBLOCK_MOVE_RANGE
- `include/save.h` — SECTOR_DATA_SIZE, SECTOR layout
- `include/constants/flags.h` — FLAGS_COUNT, DAILY_FLAGS_*
- `src/battle_dynamax.c` — Dynamax logic, raid TODOs
- `src/battle_main.c` — gBattleMons, gBattlerPartyIndexes
- `src/battle_gfx_sfx_util.c` — sprite allocation
- `ld_script_modern.ld` — EWRAM, IWRAM, ROM layout
- `.planning/codebase/CONCERNS.md` — heap, EWRAM, IWRAM
