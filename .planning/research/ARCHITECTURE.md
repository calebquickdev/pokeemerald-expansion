# Architecture: Dynamax Raid Dens — Flag/Var Allocation, RTC Daily Reset, Per-Den Storage

**Project:** pokeemerald-expansion (Dynamax Raid Dens)
**Researched:** 2025-02-27
**Domain:** GBA ROM hack, C codebase, save system, RTC, overworld

---

## 1. Flag Allocation for Den Active/Inactive

### Recommended: DAILY_FLAGS range

**Use DAILY_FLAGS for den active/inactive.** Convention:
- **Flag cleared (0)** = Active (available to raid)
- **Flag set (1)** = Inactive (already raided today)

At midnight, `ClearDailyFlags()` clears the entire DAILY_FLAGS range, so all dens automatically become Active. No extra logic needed for the flag reset.

### Safe flag range

| Range | Start | End | Count | Notes |
|-------|-------|-----|-------|-------|
| DAILY_FLAGS | 0x920 | 0x967 | 72 | Cleared every day; many FLAG_UNUSED |
| Recommended for dens | FLAG_UNUSED_0x935 | FLAG_UNUSED_0x948 | 20 | 0x935–0x948 |

**Define in `include/constants/flags.h`:**
```c
#define FLAG_DAILY_DEN_0_RAIDED   (DAILY_FLAGS_START + 0x15)  // was FLAG_UNUSED_0x935; set when raided
#define FLAG_DAILY_DEN_1_RAIDED   (DAILY_FLAGS_START + 0x16)
// ... through FLAG_DAILY_DEN_19_RAIDED
```

Or use a macro: `#define FLAG_DAILY_DEN_RAIDED(denId) (FLAG_UNUSED_0x935 + (denId))`  
Active = `!FlagGet(FLAG_DAILY_DEN_RAIDED(denId))`

**Confidence:** HIGH — DAILY_FLAGS semantics match exactly; ClearDailyFlags already runs at day change.

---

## 2. Var Allocation

### Vars not suitable for per-den species

- **VARS_START** = 0x4000, **VARS_END** = 0x40FF (256 vars)
- Unused range: VAR_UNUSED_0x40E5–0x40FF (27 vars)
- Per den: species (u16) + isGmax (bool) ≈ 2 vars each → 40 vars for 20 dens
- **Conclusion:** Not enough unused vars; use SaveBlock instead.

### Vars for other den logic

Unused vars (e.g. 0x40E5–0x40FF) are fine for temporary or single-den state (e.g. “current den id” during a script).

**Confidence:** HIGH — vars.h layout and usage are clear.

---

## 3. Per-Den Pokémon Storage (Species + isGmax)

### Recommended: SaveBlock2 extension

**Add a struct to SaveBlock2** (not SaveBlock1). SaveBlock2 fits in one 3968-byte sector and holds time-related data (`lastBerryTreeUpdate`, `localTimeOffset`), so den state fits well there.

**Struct:**
```c
#define MAX_DYNAMAX_DENS 32

struct DynamaxDen {
    u16 species;      // 0 = no Pokémon / invalid
    u8 isGmax:1;
    u8 padding:7;
};  // 3 bytes

// In SaveBlock2:
struct DynamaxDen dynamaxDens[MAX_DYNAMAX_DENS];  // 96 bytes
```

**Placement:** Add after `randomizerSeed` in SaveBlock2 (see `include/global.h` ~line 416).

### Alternatives considered

| Approach | Pros | Cons |
|----------|------|------|
| SaveBlock1 | Same block as berry trees | SaveBlock1 is large; dens are time-based like SaveBlock2 |
| SaveBlock3 | Small, replicated per sector | Max 1624 bytes; used for dex, nuzlocke, etc. |
| Vars | No save layout change | 27 unused vars; need 40+ for 20 dens |
| Flags only | No new struct | Cannot store species; flags are 1 bit each |

**Confidence:** HIGH — SaveBlock2 layout and sector size are known; 96 bytes is small.

---

## 4. RTC Daily Reset Hook

### Exact location

**File:** `src/clock.c`  
**Function:** `UpdatePerDay()` (static, called from `DoTimeBasedEvents()`)

**Flow:**
1. `DoTimeBasedEvents()` is called from `Task_RunTimeBasedEvents()` in `src/field_tasks.c` (every ~17 seconds when `gMain.vblankCounter1 & (1<<12)`).
2. `DoTimeBasedEvents()` calls `UpdatePerDay(&gLocalTime)`.
3. `UpdatePerDay()` checks `VAR_DAYS` vs `localTime->days`; when a new day has passed, it runs daily logic.

**Current `UpdatePerDay()` (lines 38–59):**
```c
static void UpdatePerDay(struct Time *localTime)
{
    u16 *days = GetVarPointer(VAR_DAYS);
    u16 daysSince;

    if (*days != localTime->days && *days <= localTime->days)
    {
        daysSince = localTime->days - *days;
        ClearDailyFlags();           // <-- den flags cleared here
        UpdateDewfordTrendPerDay(daysSince);
        UpdateTVShowsPerDay(daysSince);
        // ... more updates
        *days = localTime->days;
    }
}
```

### Hook to add

Add **after** `ClearDailyFlags()` and **before** other updates:

```c
UpdateDynamaxDens(daysSince);
```

**New function** (e.g. in `src/dynamax_den.c`):
```c
void UpdateDynamaxDens(u16 daysSince)
{
    u8 i;
    for (i = 0; i < MAX_DYNAMAX_DENS; i++)
    {
        // Recalculate species + isGmax from RNG (seed with day + den id)
        gSaveBlock2Ptr->dynamaxDens[i] = RollDynamaxDenPokemon(i);
    }
}
```

`ClearDailyFlags()` already clears den flags, so all dens become Active. `UpdateDynamaxDens()` only needs to refresh species and isGmax.

**Confidence:** HIGH — `clock.c` and `field_tasks.c` were inspected directly.

---

## 5. “New Day” Trigger — When Does It Run?

### When UpdatePerDay runs

- **Condition:** Player is in overworld, not in a Pokémon Center, and `VAR_DAYS` has changed (new day).
- **Frequency:** Checked every ~17 seconds via `Task_RunTimeBasedEvents`.
- **No explicit “midnight” check:** The game compares `VAR_DAYS` with RTC-derived `localTime->days`. When the RTC crosses midnight, `localTime->days` increments and the next check triggers `UpdatePerDay`.

### Berry tree analogy

Berry trees use **`UpdatePerMinute()`**, which compares `lastBerryTreeUpdate` with current time and calls `BerryTreeTimeUpdate(minutes)`. That is per-minute, not per-day.

For dens, the correct hook is **`UpdatePerDay()`**, which already handles “new day” semantics.

**Confidence:** HIGH — flow traced through `clock.c` and `field_tasks.c`.

---

## 6. Sprite State Based on Flag — Existing Patterns

### Berry tree pattern

- **Location:** `src/event_object_movement.c`, `SetBerryTreeGraphics()` (lines 2851–2871)
- **Mechanism:** Object uses `trainerRange_berryTreeId` as berry tree id. `MovementType_BerryTreeGrowth` calls `GetStageByBerryTreeId()` and `GetBerryTypeByBerryTreeId()` from SaveBlock1, then sets graphics via `SetBerryTreeGraphicsById()`.
- **Data flow:** Object template → `trainerRange_berryTreeId` → SaveBlock1 `berryTrees[id]` → graphics.

### Options for dens

| Approach | Description | Effort |
|----------|-------------|--------|
| **A. Custom movement type** | Add `MOVEMENT_TYPE_DYNAMAX_DEN`; init uses `trainerRange_berryTreeId` as den id, reads flag + species from SaveBlock2, sets graphics. | Medium |
| **B. MAP_SCRIPT_ON_LOAD** | Map script calls `ObjectEventSetGraphicsIdByLocalIdAndMap()` or `ObjectEventSetInvisibility()` per den based on flag. | Low |
| **C. VAR_OBJ_GFX_ID** | Set `VAR_OBJ_GFX_ID_X` in map script from den state. Object uses `OBJ_EVENT_GFX_VAR_X`. | Low, but only 16 vars; awkward for many dens per map |

### Recommendation

**Option B (MAP_SCRIPT_ON_LOAD)** for a first version:
- Each den map has `MAP_SCRIPT_ON_LOAD` calling a special like `SetupDynamaxDenObjects`.
- Script loops den objects, checks `!FlagGet(FLAG_DAILY_DEN_RAIDED(denId))` for active, and shows/hides or updates graphics.
- Reuse `ObjectEventSetGraphicsIdByLocalIdAndMap()` and/or `SetObjectInvisibility()`.

**Option A** if you want den objects to behave like berry trees (movement type drives graphics from save data).

**Confidence:** MEDIUM — berry tree pattern is clear; den-specific movement type not yet implemented.

---

## 7. Save Compatibility Risks

| Risk | Mitigation |
|------|------------|
| **New SaveBlock2 fields** | Old saves have no `dynamaxDens`; new fields will be zero. Init `species = 0` as “invalid”; `UpdateDynamaxDens()` will fill on first day change. |
| **FLAGS_COUNT change** | Using existing DAILY_FLAGS; no change to `FLAGS_COUNT` or flag array size. |
| **Struct layout** | Add new fields at the end of SaveBlock2 to avoid shifting offsets. |
| **Checksum** | Save system uses sector checksums; adding fields changes block layout. Ensure `STATIC_ASSERT` in `save.c` still passes (SaveBlock2 ≤ 3968 bytes). |

**Confidence:** MEDIUM — layout rules are clear; actual `sizeof(SaveBlock2)` should be verified before adding 96 bytes.

---

## 8. Summary Table

| Item | Recommendation | Location | Confidence |
|------|----------------|----------|------------|
| Den active/inactive | DAILY_FLAGS (FLAG_DAILY_DEN_RAIDED, 0x935–0x948) | `include/constants/flags.h` | HIGH |
| Per-den species + isGmax | `struct DynamaxDen` in SaveBlock2 | `include/global.h`, SaveBlock2 | HIGH |
| RTC daily reset hook | `UpdateDynamaxDens(daysSince)` in `UpdatePerDay()` | `src/clock.c` | HIGH |
| New day trigger | `UpdatePerDay()` via `DoTimeBasedEvents()` | `src/field_tasks.c` → `src/clock.c` | HIGH |
| Den sprite/visibility | MAP_SCRIPT_ON_LOAD + special | Map scripts, new special | MEDIUM |

---

## 9. Implementation Checklist

1. Add `FLAG_DAILY_DEN_RAIDED(denId)` in `flags.h` (reuse FLAG_UNUSED_0x935+).
2. Add `struct DynamaxDen` and `dynamaxDens[MAX_DYNAMAX_DENS]` to SaveBlock2 in `global.h`.
3. Implement `UpdateDynamaxDens(u16 daysSince)` and call it from `UpdatePerDay()` in `clock.c`.
4. Implement `RollDynamaxDenPokemon(u8 denId)` (RNG for species + isGmax).
5. Add `SetupDynamaxDenObjects` special and MAP_SCRIPT_ON_LOAD on den maps.
6. Ensure SaveBlock2 size stays within one sector (3968 bytes).
