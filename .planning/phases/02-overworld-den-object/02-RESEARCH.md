# Phase 2: Overworld Den Object — Research

**Researched:** 2026-02-27
**Domain:** pokeemerald-expansion — item data, object event graphics, map scripts, event scripts, specials table
**Confidence:** HIGH (all findings verified from source files)

---

## Summary

Phase 2 adds Wishing Piece item support, Active/Inactive den sprite states, a `MAP_SCRIPT_ON_LOAD`-driven graphic swap special, and an interaction script. All nine research questions are answered below with exact file paths, struct fields, and code snippets.

**Critical pre-existing finding:** `ITEM_WISHING_PIECE` (constant `195`) and its `[ITEM_WISHING_PIECE]` table entry already exist in the codebase (`include/constants/items.h:259`, `src/data/items.h:3565`). Plan 02-01 is a minor modification, not a new addition.

**Primary recommendation:** Model den sprites exactly after the `Fossil` / `BirthIslandStone` inanimate 16x16 pattern; model the interaction script after the ShoalCave Shell Bell expert; add `SetupDynamaxDenObjects` at the end of `data/specials.inc` using `def_special`.

---

## 1. Item Data Pattern

### Wishing Piece Already Exists

**Constant:** `include/constants/items.h:259`
```c
#define ITEM_WISHING_PIECE 195
```

**Table entry:** `src/data/items.h:3565–3579`
```c
[ITEM_WISHING_PIECE] =
{
    .name = _("Wishing Piece"),
    .price = 20,
    .description = COMPOUND_STRING(
        "Throw into a\n"
        "{PKMN} Den to attract\n"
        "Dynamax Pokémon."),
    .pocket = POCKET_ITEMS,
    .type = ITEM_USE_BAG_MENU,            // ← needs changing to ITEM_USE_FIELD
    .fieldUseFunc = ItemUseOutOfBattle_CannotUse, // ← update to something meaningful
    .flingPower = 50,
    .iconPic = gItemIcon_WishingPiece,
    .iconPalette = gItemIconPalette_WishingPiece,
},
```

**What Plan 02-01 must change:**
- `.type`: `ITEM_USE_BAG_MENU` → `ITEM_USE_FIELD`
- `.fieldUseFunc`: `ItemUseOutOfBattle_CannotUse` → a new `ItemUseOutOfBattle_WishingPiece` that shows "Use this near a Pokémon Den." (or leave as `CannotUse` with updated message)

**Item type constants:** `include/constants/items.h:1061–1063`
```c
#define ITEM_USE_FIELD        2    // Shows "USE" in bag, calls fieldUseFunc on select
#define ITEM_USE_BAG_MENU     4    // No exit callback, stays in bag menu
```

**`fieldUseFunc` signature:** `void MyFunc(u8 taskId)` — see `src/item_use.c` for examples. `ItemUseOutOfBattle_CannotUse` is defined at `src/item_use.c:1475`.

**Key item pattern** (for reference, uses `importance = 1`):
```c
.price = 0,
.importance = 1,
.pocket = POCKET_KEY_ITEMS,
```
Wishing Piece is a consumable, so keep `.pocket = POCKET_ITEMS` and no `importance`.

---

## 2. Object Event Graphics System

### `ObjectEventGraphicsInfo` Struct

**Defined:** `include/global.fieldmap.h:228–246`
```c
struct ObjectEventGraphicsInfo
{
    /*0x00*/ u16 tileTag;
    /*0x02*/ u16 paletteTag;
    /*0x04*/ u16 reflectionPaletteTag;
    /*0x06*/ u16 size;           // total bytes of tile data
    /*0x08*/ s16 width;
    /*0x0A*/ s16 height;
    /*0x0C*/ u8 paletteSlot:4;
             u8 shadowSize:2;
             u8 inanimate:1;
             u8 compressed:1;
    /*0x0D*/ u8 tracks;
    /*0x10*/ const struct OamData *oam;
    /*0x14*/ const struct SubspriteTable *subspriteTables;
    /*0x18*/ const union AnimCmd *const *anims;
    /*0x1C*/ const struct SpriteFrameImage *images;
    /*0x20*/ const union AffineAnimCmd *const *affineAnims;
};
```

### Inanimate 16×16 Template (Fossil — closest equivalent)

**Definition:** `src/data/object_events/object_event_graphics_info.h:3820`
```c
const struct ObjectEventGraphicsInfo gObjectEventGraphicsInfo_Fossil = {
    .tileTag = TAG_NONE,
    .paletteTag = OBJ_EVENT_PAL_TAG_NPC_1,
    .reflectionPaletteTag = OBJ_EVENT_PAL_TAG_NONE,
    .size = 128,              // 16×16 pixels / 2 bpp packed = 128 bytes
    .width = 16,
    .height = 16,
    .paletteSlot = PALSLOT_NPC_1,
    .shadowSize = SHADOW_SIZE_S,
    .inanimate = TRUE,
    .compressed = FALSE,
    .tracks = TRACKS_NONE,
    .oam = &gObjectEventBaseOam_16x16,
    .subspriteTables = sOamTables_16x16,
    .anims = sAnimTable_Inanimate,
    .images = sPicTable_Fossil,
    .affineAnims = gDummySpriteAffineAnimTable,
};
```

Den sprites are also 16×16 inanimate objects and should use the same pattern. Use a **new palette tag** `OBJ_EVENT_PAL_TAG_RAID_DEN = 0x1125` and `PALSLOT_NPC_SPECIAL` to avoid sharing/overwriting NPC palettes, since Active/Inactive use distinct colors. Both Active and Inactive can share the same palette (one `.pal` file with colors used by both frames), or use separate tags `0x1125`/`0x1126`.

### `OBJ_EVENT_GFX_*` Constants

**File:** `include/constants/event_objects.h`

Last entry: `OBJ_EVENT_GFX_OW_MON = 240`. Current count: `NUM_OBJ_EVENT_GFX = 241`.

**New constants to add:**
```c
#define OBJ_EVENT_GFX_RAID_DEN_INACTIVE  241
#define OBJ_EVENT_GFX_RAID_DEN_ACTIVE    242
```
**Update:** `NUM_OBJ_EVENT_GFX` from `241` to `243`.

**Note from comment at line 246:** Maximum is 65519 due to dynamic graphics IDs needing 16 reserved values — no constraint at 243.

---

## 3. Sprite Registration

### Files to modify

| File | What to add |
|------|-------------|
| `src/data/object_events/object_event_graphics.h` | `gObjectEventPic_RaidDenInactive[]` and `gObjectEventPic_RaidDenActive[]` + palette |
| `src/data/object_events/object_event_pic_tables.h` | `sPicTable_RaidDenInactive[]` and `sPicTable_RaidDenActive[]` |
| `src/data/object_events/object_event_graphics_info.h` | Two `gObjectEventGraphicsInfo_RaidDen*` structs |
| `src/data/object_events/object_event_graphics_info_pointers.h` | Two `extern` declarations + pointer table entries |
| `include/constants/event_objects.h` | Two new `OBJ_EVENT_GFX_RAID_DEN_*` constants + palette tag + update `NUM_OBJ_EVENT_GFX` |

### Graphics file loading (`object_event_graphics.h`)

Pattern:
```c
// Single-frame inanimate object — use INCBIN_U32 for tiles, INCBIN_U16 for palette
const u32 gObjectEventPic_RaidDenInactive[] = INCBIN_U32("graphics/object_events/pics/misc/raid_den_inactive.4bpp");
const u32 gObjectEventPic_RaidDenActive[]   = INCBIN_U32("graphics/object_events/pics/misc/raid_den_active.4bpp");
const u16 gObjectEventPal_RaidDen[]         = INCBIN_U16("graphics/object_events/palettes/raid_den.gbapal");
```

Graphics source PNG files go in `graphics/object_events/pics/misc/` (like `fossil.4bpp`). The `.4bpp` and `.gbapal` files are converted automatically by the build system from PNG sources via `gbagfx`.

### Pic table (`object_event_pic_tables.h`)

Single-frame inanimates use `obj_frame_tiles`:
```c
static const struct SpriteFrameImage sPicTable_RaidDenInactive[] = {
    obj_frame_tiles(gObjectEventPic_RaidDenInactive),
};
static const struct SpriteFrameImage sPicTable_RaidDenActive[] = {
    obj_frame_tiles(gObjectEventPic_RaidDenActive),
};
```

### Graphics info structs (`object_event_graphics_info.h`)

```c
const struct ObjectEventGraphicsInfo gObjectEventGraphicsInfo_RaidDenInactive = {
    .tileTag = TAG_NONE,
    .paletteTag = OBJ_EVENT_PAL_TAG_RAID_DEN,
    .reflectionPaletteTag = OBJ_EVENT_PAL_TAG_NONE,
    .size = 128,
    .width = 16,
    .height = 16,
    .paletteSlot = PALSLOT_NPC_SPECIAL,
    .shadowSize = SHADOW_SIZE_S,
    .inanimate = TRUE,
    .compressed = FALSE,
    .tracks = TRACKS_NONE,
    .oam = &gObjectEventBaseOam_16x16,
    .subspriteTables = sOamTables_16x16,
    .anims = sAnimTable_Inanimate,
    .images = sPicTable_RaidDenInactive,
    .affineAnims = gDummySpriteAffineAnimTable,
};
// Identical but with .images = sPicTable_RaidDenActive for the active variant
```

### Pointer table (`object_event_graphics_info_pointers.h`)

Add `extern` declarations at the top, then pointer entries in `gObjectEventGraphicsInfoPointers[]`:
```c
extern const struct ObjectEventGraphicsInfo gObjectEventGraphicsInfo_RaidDenInactive;
extern const struct ObjectEventGraphicsInfo gObjectEventGraphicsInfo_RaidDenActive;
```

In the table body (after `[OBJ_EVENT_GFX_OW_MON]`):
```c
[OBJ_EVENT_GFX_RAID_DEN_INACTIVE] = &gObjectEventGraphicsInfo_RaidDenInactive,
[OBJ_EVENT_GFX_RAID_DEN_ACTIVE]   = &gObjectEventGraphicsInfo_RaidDenActive,
```

### Palette tag constant

**File:** `include/constants/event_objects.h` — add after `OBJ_EVENT_PAL_TAG_DYNAMIC = 0x1124`:
```c
#define OBJ_EVENT_PAL_TAG_RAID_DEN  0x1125
```
Tags `0x1125`–`0x114F` are currently unassigned.

---

## 4. MAP_SCRIPT_ON_LOAD Pattern

### Header macro format (`asm/macros/map.inc:10`)

```asm
.macro map_script type:req, script:req
.byte \type
.4byte \script
.endm
```

### Minimal map scripts block for a den map

```asm
DenTestMap_MapScripts::
    map_script MAP_SCRIPT_ON_LOAD, DenTestMap_OnLoad
    .byte 0

DenTestMap_OnLoad:
    special SetupDynamaxDenObjects
    end
```

The `.byte 0` terminates the map script table. `special` directly calls the C function; `end` terminates the script.

**Verified patterns from existing maps:**
- `AncientTomb_MapScripts` / `AncientTomb_OnLoad` (`data/maps/AncientTomb/scripts.inc`) — uses `call_if_unset` + `end`
- `ShoalCave_LowTideEntranceRoom_OnTransition` — calls `special UpdateShoalTideFlag` directly, proving `special` is valid in map script handlers

### Map JSON object event format

```json
{
  "graphics_id": "OBJ_EVENT_GFX_RAID_DEN_INACTIVE",
  "x": 5, "y": 5, "elevation": 3,
  "movement_type": "MOVEMENT_TYPE_NONE",
  "movement_range_x": 0, "movement_range_y": 0,
  "trainer_type": "TRAINER_TYPE_NONE",
  "trainer_sight_or_berry_tree_id": "0",
  "script": "DenTestMap_EventScript_Den0",
  "flag": "0"
}
```

---

## 5. ObjectEventSetGraphicsIdByLocalIdAndMap

### Definition

**File:** `src/event_object_movement.c:2797`
```c
void ObjectEventSetGraphicsIdByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup, u16 graphicsId)
{
    u8 objectEventId;
    if (!TryGetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup, &objectEventId))
        ObjectEventSetGraphicsId(&gObjectEvents[objectEventId], graphicsId);
}
```

### Declaration gap — action required

**`ObjectEventSetGraphicsIdByLocalIdAndMap` is NOT declared in `include/event_object_movement.h`.**

`ObjectEventSetGraphicsId` (single object pointer variant) IS declared at `include/event_object_movement.h:156`.

**Plan 02-03 must add** this declaration to `include/event_object_movement.h`:
```c
void ObjectEventSetGraphicsIdByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup, u16 graphicsId);
```

### Getting current map coordinates in C

From `gSaveBlock1Ptr->location` (`struct WarpData` at `include/global.h:615`):
```c
gSaveBlock1Ptr->location.mapGroup  // u8-compatible s8
gSaveBlock1Ptr->location.mapNum    // u8-compatible s8
```

### SetupDynamaxDenObjects design

The special must correlate den IDs to localIds on the current map. Recommended: a file-scope table in `src/raid_den.c`:
```c
struct DenMapEntry { u8 mapGroup; u8 mapNum; u8 localId; u8 denId; };
static const struct DenMapEntry sDenMapTable[] = {
    // populated as dens are placed on maps
};
```

`SetupDynamaxDenObjects(void)` iterates `sDenMapTable`, matches current map, and calls `ObjectEventSetGraphicsIdByLocalIdAndMap`.

```c
void SetupDynamaxDenObjects(void)
{
    u8 mapGroup = (u8)gSaveBlock1Ptr->location.mapGroup;
    u8 mapNum   = (u8)gSaveBlock1Ptr->location.mapNum;
    u16 gfxId;
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sDenMapTable); i++)
    {
        if (sDenMapTable[i].mapGroup != mapGroup || sDenMapTable[i].mapNum != mapNum)
            continue;
        gfxId = FlagGet(FLAG_DAILY_DEN_RAIDED(sDenMapTable[i].denId))
              ? OBJ_EVENT_GFX_RAID_DEN_INACTIVE
              : OBJ_EVENT_GFX_RAID_DEN_ACTIVE;
        ObjectEventSetGraphicsIdByLocalIdAndMap(
            sDenMapTable[i].localId, mapNum, mapGroup, gfxId);
    }
}
```

**Note on flag logic:** `FLAG_DAILY_DEN_RAIDED` is set after raiding. An un-raided den is **active** (not yet raided today). A raided den becomes inactive. So: `FlagGet(FLAG_DAILY_DEN_RAIDED(denId)) == FALSE` → den is Active. Verify this interpretation matches the game design intent.

---

## 6. Event Script Commands

### Exact macro signatures (`asm/macros/event.inc`)

| Command | Macro | Effect |
|---------|-------|--------|
| `goto destination` | line 31 | Jump to label unconditionally |
| `goto_if condition, destination` | line 37 | Jump if last compare result matches condition |
| `goto_if_set FLAG, dest` | line 1887 | `checkflag FLAG` + `goto_if TRUE, dest` |
| `goto_if_unset FLAG, dest` | line 1882 | `checkflag FLAG` + `goto_if FALSE, dest` |
| `compare VAR, value` | line 248 | Smart compare: var-to-var if value is a var, var-to-value otherwise |
| `special FUNCTION` | line 280 | Call `void FUNCTION(void)` from specials table |
| `specialvar VAR, FUNCTION` | line 287 | Call function, result stored in VAR |
| `checkitem ITEM, quantity=1` | line 553 | Checks bag; result in VAR_RESULT (TRUE/FALSE) |
| `removeitem ITEM, quantity=1` | line 537 | Removes items from bag |
| `msgbox text, type=MSGBOX_DEFAULT` | line 2030 | `loadword 0, text` + `callstd type` |
| `message text` | line 837 | Lower-level: sets message pointer only |
| `closemessage` | line 843 | Closes message box |

### `msgbox` types (for reference)

- `MSGBOX_DEFAULT` — waits for button, no yes/no
- `MSGBOX_YESNO` — yes/no prompt; result in `VAR_RESULT` as `YES` or `NO`
- `MSGBOX_NPC` — NPC-style with lock/release implied
- `MSGBOX_SIGN` — for signpost reading

### Comparison results

- After `goto_if_eq VAR_RESULT, YES` — YES is 1, NO is 0
- After `checkitem`: `VAR_RESULT = TRUE` (has item) or `VAR_RESULT = FALSE` (doesn't have)

---

## 7. Item Interaction Script Pattern

### ShoalCave Shell Bell Expert (`data/maps/ShoalCave_LowTideEntranceRoom/scripts.inc:18`)

This is the canonical pattern for: check item → prompt YESNO → remove item → proceed:

```asm
ShoalCave_LowTideEntranceRoom_EventScript_ShellBellExpert::
    lock
    faceplayer
    checkitem ITEM_SHOAL_SALT, 4
    goto_if_eq VAR_RESULT, FALSE, ShoalCave_NotEnough
    msgbox ShoalCave_Text_WouldYouLike, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, ShoalCave_Decline
    removeitem ITEM_SHOAL_SALT, 4
    removeitem ITEM_SHOAL_SHELL, 4
    giveitem ITEM_SHELL_BELL
    release
    end
```

### Den interaction script template

```asm
.set LOCALID_DEN_0, 1

DenMap_EventScript_Den0::
    lock
    faceplayer
    goto_if_set FLAG_DAILY_DEN_RAIDED_0, DenMap_EventScript_Den0_Active
    @ Inactive path
    msgbox DenMap_Text_InactiveDen, MSGBOX_DEFAULT
    checkitem ITEM_WISHING_PIECE
    goto_if_eq VAR_RESULT, FALSE, DenMap_EventScript_Den0_NoItem
    msgbox DenMap_Text_UseWishingPiece, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, DenMap_EventScript_Den0_Declined
    removeitem ITEM_WISHING_PIECE
    special ActivateDen0          @ sets flag, rerolls den, swaps graphics
    special SetupDynamaxDenObjects
    release
    end

DenMap_EventScript_Den0_NoItem::
    release
    end

DenMap_EventScript_Den0_Declined::
    release
    end

DenMap_EventScript_Den0_Active::
    special OpenDenLobbyScreen    @ stub for Phase 3
    waitstate
    release
    end

DenMap_Text_InactiveDen:
    .string "Seems it's a den for DYNAMAX\n"
    .string "Pokémon to appear...$"

DenMap_Text_UseWishingPiece:
    .string "Would you like to use\n"
    .string "a WISHING PIECE?$"
```

**Note:** `FLAG_DAILY_DEN_RAIDED_0` is the concrete flag for den 0 — use the macro `FLAG_DAILY_DEN_RAIDED(0)` in C, but in scripts use the resolved constant name. The flags are defined as `FLAG_UNUSED_0x935 + 0` through `+19`, aliased via `FLAG_DAILY_DEN_RAIDED(denId)` in `include/raid_den.h`.

---

## 8. Specials Table

### File and format

**File:** `data/specials.inc`

The `def_special` macro (defined at lines 4–13):
```asm
.macro def_special ptr:req, requests_effects=0
.global SPECIAL_\ptr
.set SPECIAL_\ptr, __special__
.set __special__, __special__ + 1
    .4byte \ptr           @ (or \ptr + ROM_SIZE if requests_effects)
.endm
```

Usage:
```asm
gSpecials::
    def_special HealPlayerParty
    def_special SetCableClubWarp
    ...
    def_special EnterCode
    def_special GetCodeFeedback    @ ← currently last entry (line 565)
```

### Adding new specials

Append to the end of `data/specials.inc`:
```asm
    def_special SetupDynamaxDenObjects
    def_special ActivateDynamaxDen
    def_special OpenDenLobbyScreen
```

The `SPECIAL_SetupDynamaxDenObjects` constant is automatically generated and used by the `special SetupDynamaxDenObjects` script macro.

**C function signature for specials:** `void FunctionName(void)` — no parameters, no return value. Results passed via `VAR_RESULT`/`gSpecialVar_0x8000` for `specialvar` variants.

**Example in `src/time_events.c:54`:**
```c
void UpdateShoalTideFlag(void)
{
    // ... no taskId, void return
}
```

### Forward declarations

New special functions need to be declared somewhere the assembler can link them. Either:
- Add to `include/raid_den.h` (recommended for den-related specials)
- Or declare in `include/field_specials.h` if they'll be general-purpose

---

## 9. Phase 1 Files — Confirmed State

### `include/raid_den.h`

```c
#ifndef GUARD_RAID_DEN_H
#define GUARD_RAID_DEN_H

#include "constants/flags.h"
#include "constants/species.h"

#define MAX_DYNAMAX_DENS 20

#define FLAG_DAILY_DEN_RAIDED(denId)  (DAILY_FLAGS_START + 0x15 + (denId))

struct DynamaxDen
{
    u16 species;
    u8 isGmax;
    u8 _pad;
};

void UpdateDynamaxDens(u16 daysSince);
u16 RollDynamaxDenPokemon(u8 denId);

#endif // GUARD_RAID_DEN_H
```

### `include/global.h` — SaveBlock2

```c
// Line 572–574:
u32 randomizerSeed;
struct DynamaxDen dynamaxDens[MAX_DYNAMAX_DENS];
}; // sizeof=0xF7C
```

### `src/raid_den.c`

```c
#include "global.h"
#include "raid_den.h"

u16 RollDynamaxDenPokemon(u8 denId)
{
    (void)denId;
    return SPECIES_RALTS;
}

void UpdateDynamaxDens(u16 daysSince)
{
    u8 i;
    (void)daysSince;
    for (i = 0; i < MAX_DYNAMAX_DENS; i++)
    {
        gSaveBlock2Ptr->dynamaxDens[i].species = RollDynamaxDenPokemon(i);
        gSaveBlock2Ptr->dynamaxDens[i].isGmax = 0;
    }
}
```

### Flag constants available

`FLAG_DAILY_DEN_RAIDED(denId)` expands to a runtime expression using `DAILY_FLAGS_START`. For use in event scripts, the concrete flag values are:
- Den 0: `DAILY_FLAGS_START + 0x15` = `0x920 + 0x15` = `0x935` → constant `FLAG_UNUSED_0x935`
- Den 1: `0x936` → `FLAG_UNUSED_0x936`
- …up to Den 19: `0x948` → `FLAG_UNUSED_0x948`

For scripts that need the concrete flag name, define named aliases in `include/constants/flags.h` or use the macro in C. Since scripts use `goto_if_set FLAG_DAILY_DEN_RAIDED_0`, define these per-den constants:
```c
#define FLAG_DAILY_DEN_RAIDED_0  FLAG_DAILY_DEN_RAIDED(0)  // or hardcode 0x935
// ... repeat for dens 1–3 (or more as needed)
```

---

## Open Questions

1. **Flag direction:** Is `FLAG_DAILY_DEN_RAIDED` set when the den IS raided (inactive) or when it IS active? Current `raid_den.h` name implies "raided = done for today = inactive". Plan 02-03 says `!FlagGet(FLAG_DAILY_DEN_RAIDED(denId))` → active. Confirm: an un-raided den = active = glowing. ✓ Consistent with the name.

2. **Activation logic in ActivateDynamaxDen:** Plan says "Yes removes 1 Wishing Piece, activates den, rolls den Pokémon." Should activating a den CLEAR `FLAG_DAILY_DEN_RAIDED` (making it active) or set a separate `FLAG_DEN_WISHING_PIECE_USED`? The daily flag is cleared at midnight by `ClearDailyFlags`. A freshly activated den after a Wishing Piece should be active (flag NOT set). On raid completion, set the flag. So `ActivateDynamaxDen` does NOT set `FLAG_DAILY_DEN_RAIDED` — it just re-rolls the species (the flag being unset means it's active by default).

3. **Script flag names:** Scripts cannot call the `FLAG_DAILY_DEN_RAIDED(denId)` macro directly. Either define per-den aliases like `FLAG_DAILY_DEN_RAIDED_0` in `include/constants/flags.h`, or use `FLAG_UNUSED_0x935` directly. The cleaner approach is named aliases.

4. **Test map:** Phase 2 needs at least one test map with a den object to validate the system end-to-end. This may require creating a new map directory under `data/maps/` or placing a den on an existing map. The planner should include a task for this.

---

## Sources

| File | Lines | What was verified |
|------|-------|-------------------|
| `src/data/items.h` | 3565–3579 | Wishing Piece entry — already exists |
| `include/constants/items.h` | 259 | `ITEM_WISHING_PIECE = 195` |
| `include/global.fieldmap.h` | 228–246 | `ObjectEventGraphicsInfo` struct |
| `include/constants/event_objects.h` | 203–249 | GFX IDs, last = 240; NUM = 241; palette tags |
| `src/data/object_events/object_event_graphics_info.h` | 3820–3835 | Fossil inanimate template |
| `src/data/object_events/object_event_graphics.h` | 226 | INCBIN_U32 pattern for tiles |
| `src/data/object_events/object_event_pic_tables.h` | 979–981 | `obj_frame_tiles` single-frame pattern |
| `src/data/object_events/object_event_graphics_info_pointers.h` | 1–493 | Pointer table format |
| `src/data/object_events/object_event_subsprites.h` | 83–87 | `sOamTables_16x16` |
| `src/data/object_events/base_oam.h` | 13–17 | `gObjectEventBaseOam_16x16` |
| `src/data/object_events/object_event_anims.h` | 1097–1099 | `sAnimTable_Inanimate` |
| `src/event_object_movement.c` | 2797–2803 | `ObjectEventSetGraphicsIdByLocalIdAndMap` definition |
| `include/event_object_movement.h` | 125–156 | Declared functions — `ByLocalIdAndMap` NOT listed |
| `data/specials.inc` | 1–566 | `def_special` format; 566 entries; last = `GetCodeFeedback` |
| `asm/macros/event.inc` | 31–557 | All macro signatures verified |
| `data/maps/ShoalCave_LowTideEntranceRoom/scripts.inc` | 18–79 | checkitem/removeitem/YESNO pattern |
| `data/maps/AncientTomb/scripts.inc` | 1–10 | MAP_SCRIPT_ON_LOAD → `end` pattern |
| `src/time_events.c` | 54 | `void UpdateShoalTideFlag(void)` — special void signature |
| `include/raid_den.h` | full | Phase 1 output — confirmed exists |
| `src/raid_den.c` | full | Phase 1 output — confirmed exists |
| `include/global.h` | 572–574 | SaveBlock2 + dynamaxDens confirmed |
