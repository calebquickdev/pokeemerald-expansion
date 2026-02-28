# Phase 1 Research: Den Foundation

## 1. SaveBlock2 Structure

**Location:** `include/global.h` lines 533–572

**Current structure (end):**
```c
struct SaveBlock2
{
    // ... many fields ...
    /*0x64C*/ struct BattleFrontier frontier;
    u32 randomizerSeed;  // last field
}; // sizeof = 0xF2C = 3884 bytes
```

- **Last field:** `u32 randomizerSeed;` (line 571)
- **Current size:** 3884 bytes
- **Sector limit:** `SECTOR_DATA_SIZE = 3968` bytes (`include/save.h` line 5)
- **Free space:** **84 bytes remaining**
- **Size check:** `STATIC_ASSERT(sizeof(struct SaveBlock2) <= SECTOR_DATA_SIZE, SaveBlock2FreeSpace);` in `src/save.c` line 82 — build fails if exceeded

**Gotcha:** 32 dens × 3 bytes = 96 bytes → exceeds limit. 20 dens × 3 bytes = 60 bytes → fits. Packing into `u16 species:12; u16 isGmax:1; u16 padding:3;` saves nothing meaningful (still 2 bytes/den × 20 = 40 bytes).

---

## 2. Flags System & Daily Flags

**Location:** `include/constants/flags.h` lines 1569–1641

**DAILY_FLAGS range:**
```c
#define DAILY_FLAGS_START   0x920   // (aligned)
#define DAILY_FLAGS_END     0x967
#define NUM_DAILY_FLAGS     72      // flags
```

**ClearDailyFlags()** — `src/event_data.c` lines 69–72:
```c
void ClearDailyFlags(void)
{
    memset(&gSaveBlock1Ptr->flags[DAILY_FLAGS_START / 8], 0, DAILY_FLAGS_SIZE);
}
```
Clears the entire DAILY_FLAGS byte range via `memset`. No per-flag logic needed.

**Available flags for dens:**
- `FLAG_UNUSED_0x935` through `FLAG_UNUSED_0x948` — 20 contiguous unused flags
- These map to `DAILY_FLAGS_START + 0x15` through `DAILY_FLAGS_START + 0x28`

**Flags are stored in SaveBlock1** (`gSaveBlock1Ptr->flags[]`), not SaveBlock2.

---

## 3. Clock / UpdatePerDay Hook

**Location:** `src/clock.c` lines 38–60

```c
static void UpdatePerDay(struct Time *localTime)
{
    u16 *days = GetVarPointer(VAR_DAYS);
    u16 daysSince;

    if (*days != localTime->days && *days <= localTime->days)
    {
        daysSince = localTime->days - *days;
        ClearDailyFlags();                      // ← hook after here
        UpdateDewfordTrendPerDay(daysSince);
        UpdateTVShowsPerDay(daysSince);
        // ... more updates ...
        *days = localTime->days;
    }
}
```

- **Hook point:** Add `UpdateDynamaxDens(daysSince);` immediately after `ClearDailyFlags()` on line 46
- `UpdatePerDay()` is `static` — add call directly inside this function
- `daysSince` is available and handles multi-day skips (>1 day)
- Called via `DoTimeBasedEvents()` which checks `FLAG_SYS_CLOCK_SET`

---

## 4. Flag Macro Patterns

No existing parameterized `FLAG_X(id)` macros found. Existing patterns use arithmetic:
- `TRAINER_FLAGS_START + trainerId`
- `DAILY_FLAGS_START + offset`

**Recommended macro:**
```c
#define FLAG_DAILY_DEN_RAIDED(denId)  (DAILY_FLAGS_START + 0x15 + (denId))
```
Must be valid at compile time if used in array indexing; works fine for `FlagGet`/`FlagSet` calls at runtime.

---

## 5. Save System Constraints

- SaveBlock2 occupies exactly one 3968-byte sector (sector 0)
- Build will fail with `STATIC_ASSERT` if struct exceeds 3968 bytes
- No explicit migration system — adding fields at the **end** of the struct is safe (old saves get zero-initialized uninitialized data)
- SaveBlock2 loaded into `gSaveBlock2Ptr` in EWRAM; pointer-based access throughout

---

## Key Decisions for Planner

1. **Den count:** `MAX_DYNAMAX_DENS = 20` (60 bytes used of 84 free). Define as a constant in a config or constants header.
2. **Struct layout:** `struct DynamaxDen { u16 species; u8 isGmax; };` — 3 bytes with natural padding → 4 bytes/den × 20 = 80 bytes. **Safer to use `u8 isGmax:1; u8 padding:7;` to stay at 3 bytes packed, or use `__attribute__((packed))`.** Alternatively: `struct DynamaxDen { u16 species; u8 isGmax; u8 _pad; };` = 4 bytes × 20 = 80 bytes (fits within 84).
3. **Placement:** Add `struct DynamaxDen dynamaxDens[MAX_DYNAMAX_DENS];` after `randomizerSeed` (last field) in SaveBlock2.
4. **Flag range:** Use `FLAG_UNUSED_0x935`–`FLAG_UNUSED_0x948` (20 flags). Define macro `FLAG_DAILY_DEN_RAIDED(denId)` pointing into this range.
5. **ClearDailyFlags already handles flag reset** — no extra den-specific clear needed.
6. **Hook:** `UpdateDynamaxDens(daysSince)` added in `src/clock.c` after `ClearDailyFlags()` inside `UpdatePerDay()`.
7. **Phase 1 stub:** `RollDynamaxDenPokemon(denId)` returns a hardcoded placeholder species (e.g., `SPECIES_RALTS`) — real pool logic deferred to Phase 6.
8. **Declaration:** `UpdateDynamaxDens()` and `RollDynamaxDenPokemon()` need forward declarations; add to a new `include/dynamax_den.h` or `include/raid_den.h`.

## RESEARCH COMPLETE
