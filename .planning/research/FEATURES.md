# Feature Implementation: Dynamax Raid Dens

**Domain:** GBA ROM hack (pokeemerald-expansion)
**Researched:** Feb 27, 2025
**Confidence:** HIGH (codebase-derived)

Implementation guide for adding Dynamax Raid Dens: interactable overworld objects with Active/Inactive states and a custom Den Lobby Screen.

---

## 1. Map JSON Format for Object Events

**Source:** `data/maps/Route104/map.json`, `data/maps/Route117_PokemonDayCare/map.json`

Each object event in the `object_events` array has:

| Field | Type | Description |
|-------|------|--------------|
| `graphics_id` | string | OBJ_EVENT_GFX_* constant (e.g. `OBJ_EVENT_GFX_BERRY_TREE`) |
| `x` | number | Map X coordinate |
| `y` | number | Map Y coordinate |
| `elevation` | number | 0–4, controls layering |
| `movement_type` | string | `MOVEMENT_TYPE_*` (e.g. `MOVEMENT_TYPE_LOOK_AROUND` for static) |
| `movement_range_x` | number | Range for wander types |
| `movement_range_y` | number | Range for wander types |
| `trainer_type` | string | `TRAINER_TYPE_NONE` for non-trainers |
| `trainer_sight_or_berry_tree_id` | string | `"0"` for non-trainer/non-berry |
| `script` | string | Script label (e.g. `Route104_EventScript_BugCatcher`) or `"0x0"` |
| `flag` | string | `"0"` or `FLAG_*` to hide when set |

**Example for a Raid Den (static, interactable):**

```json
{
  "graphics_id": "OBJ_EVENT_GFX_RAID_DEN_INACTIVE",
  "x": 10,
  "y": 8,
  "elevation": 3,
  "movement_type": "MOVEMENT_TYPE_LOOK_AROUND",
  "movement_range_x": 0,
  "movement_range_y": 0,
  "trainer_type": "TRAINER_TYPE_NONE",
  "trainer_sight_or_berry_tree_id": "0",
  "script": "RouteX_EventScript_RaidDen",
  "flag": "0"
}
```

Use `MOVEMENT_TYPE_LOOK_AROUND` for a static one-tile object. Add the script to `data/scripts/` and reference it in the map's `scripts.inc`.

**Confidence:** HIGH

---

## 2. Registering a New Graphics ID (Den Sprite, Two States)

**Sources:** `src/data/object_events/object_event_graphics.h`, `object_event_graphics_info.h`, `object_event_graphics_info_pointers.h`, `include/constants/event_objects.h`

### Step 2.1: Add graphics data

In `src/data/object_events/object_event_graphics.h`:

```c
const u32 gObjectEventPic_RaidDenInactive[] = INCBIN_U32("graphics/object_events/pics/misc/raid_den_inactive.4bpp");
const u32 gObjectEventPic_RaidDenActive[] = INCBIN_U32("graphics/object_events/pics/misc/raid_den_active.4bpp");
const u16 gObjectEventPal_RaidDen[] = INCBIN_U16("graphics/object_events/palettes/raid_den.gbapal");
```

### Step 2.2: Add pic table

In `src/data/object_events/object_event_pic_tables.h` (or a new file):

```c
static const struct SpriteFrameImage sPicTable_RaidDenInactive[] = {
    overworld_frame(gObjectEventPic_RaidDenInactive, 2, 2, 0),
};
static const struct SpriteFrameImage sPicTable_RaidDenActive[] = {
    overworld_frame(gObjectEventPic_RaidDenActive, 2, 2, 0),
};
```

Use `overworld_frame(pic, width_tiles, height_tiles, frame_index)`. For a 16×16 sprite use `2, 2`; for 32×32 use `4, 4`.

### Step 2.3: Add ObjectEventGraphicsInfo

In `src/data/object_events/object_event_graphics_info.h`:

```c
const struct ObjectEventGraphicsInfo gObjectEventGraphicsInfo_RaidDenInactive = {
    .tileTag = TAG_NONE,
    .paletteTag = OBJ_EVENT_PAL_TAG_NPC_1,
    .reflectionPaletteTag = OBJ_EVENT_PAL_TAG_NONE,
    .size = 256,
    .width = 16,
    .height = 16,
    .paletteSlot = PALSLOT_NPC_1,
    .shadowSize = SHADOW_SIZE_S,
    .inanimate = TRUE,
    .compressed = FALSE,
    .tracks = TRACKS_NONE,
    .oam = &gObjectEventBaseOam_16x16,
    .subspriteTables = NULL,
    .anims = sAnimTable_Standard,
    .images = sPicTable_RaidDenInactive,
    .affineAnims = gDummySpriteAffineAnimTable,
};
// Repeat for gObjectEventGraphicsInfo_RaidDenActive with sPicTable_RaidDenActive
```

### Step 2.4: Add constants

In `include/constants/event_objects.h`:

```c
#define OBJ_EVENT_GFX_RAID_DEN_INACTIVE  XXX
#define OBJ_EVENT_GFX_RAID_DEN_ACTIVE    YYY
```

### Step 2.5: Add to pointer table

In `src/data/object_events/object_event_graphics_info_pointers.h`:

```c
[OBJ_EVENT_GFX_RAID_DEN_INACTIVE] = &gObjectEventGraphicsInfo_RaidDenInactive,
[OBJ_EVENT_GFX_RAID_DEN_ACTIVE] = &gObjectEventGraphicsInfo_RaidDenActive,
```

### Two-frame animation (optional)

For a simple two-frame loop (e.g. inactive ↔ active), use two frames in one pic table:

```c
static const struct SpriteFrameImage sPicTable_RaidDen[] = {
    overworld_frame(gObjectEventPic_RaidDenInactive, 2, 2, 0),
    overworld_frame(gObjectEventPic_RaidDenActive, 2, 2, 0),
};
```

Then add an anim in `object_event_anims.h` that alternates between frames. Berry trees use `sAnimTable_BerryTree` with stage-based anims; for a den you can use `sAnimTable_Standard` or a custom anim.

**Confidence:** HIGH

---

## 3. Den Interaction Script Template

**Sources:** `data/maps/NewMauville_Entrance/scripts.inc`, `src/scrcmd.c`, `data/maps/MauvilleCity_GameCorner/scripts.inc`

### checkitem / removeitem / VAR_RESULT

- `checkitem ITEM_X` → sets `gSpecialVar_Result` (TRUE/FALSE)
- Scripts read `VAR_RESULT` (0x800D), which maps to `gSpecialVar_Result`
- `removeitem ITEM_X, 1` removes one and sets `gSpecialVar_Result`

### Example script (in `data/scripts/raid_den.inc` or map-specific scripts.inc)

```asm
RouteX_EventScript_RaidDen::
	lockall
	msgbox RouteX_Text_RaidDenPrompt, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, RouteX_EventScript_RaidDen_Exit
	checkitem ITEM_WISHING_PIECE, 1
	goto_if_eq VAR_RESULT, FALSE, RouteX_EventScript_RaidDen_NoWishingPiece
	msgbox RouteX_Text_UseWishingPiece, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, RouteX_EventScript_RaidDen_Exit
	removeitem ITEM_WISHING_PIECE, 1
	call RouteX_EventScript_ActivateRaidDen
	special Special_OpenDenLobbyScreen
	releaseall
	end

RouteX_EventScript_RaidDen_NoWishingPiece::
	msgbox RouteX_Text_NeedWishingPiece, MSGBOX_DEFAULT
	releaseall
	end

RouteX_EventScript_RaidDen_Exit::
	releaseall
	end

RouteX_EventScript_ActivateRaidDen::
	setvar VAR_0x8004, LOCALID_RAID_DEN
	setvar VAR_0x8005, OBJ_EVENT_GFX_RAID_DEN_ACTIVE
	special Special_SetObjectGraphicsId
	return
```

To change graphics from script you need a `special` that calls `ObjectEventSetGraphicsIdByLocalIdAndMap(localId, mapNum, mapGroup, graphicsId)`. Add it in `src/field_specials.c` and register in the specials table.

**Confidence:** HIGH

---

## 4. CB2 Den Lobby Screen Pattern

**Sources:** `src/wallclock.c`, `src/main.c`, `include/overworld.h`

### 4.1 Entry point

From script, call a special that does:

```c
SetMainCallback2(CB2_DenLobbyScreen);
gMain.savedCallback = CB2_ReturnToFieldContinueScript;  // or CB2_ReturnToField
```

### 4.2 CB2 structure

```c
static void CB2_DenLobbyScreen(void)
{
    RunTasks();
}

void CB2_InitDenLobbyScreen(void)
{
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_BG0_ON | DISPCNT_BG1_ON | ...);
    // Load tilemap, load palettes, create sprites
    CreateTask(Task_DenLobby_Main, 0);
    SetVBlankCallback(VBlankCB_DenLobby);
    SetMainCallback2(CB2_DenLobbyScreen);
}
```

### 4.3 Typical flow

1. **Init:** `ResetBgsAndClearDma3BusyFlags`, `InitBgsFromTemplates`, `ChangeBgX/Y`, `LoadBgTiles`, `LoadBgTilemap`, `LoadPalette`
2. **Sprites:** `LoadCompressedSpriteSheet`, `LoadSpritePalettes`, `CreateSprite`
3. **Task:** `CreateTask(Task_DenLobby_Main, 0)` — main logic and input
4. **Return:** In the task, when done: `SetMainCallback2(gMain.savedCallback)` and `FreeAllWindowBuffers()`

### 4.4 References

- `src/wallclock.c` — simple CB2 with BgTemplates, sprites, tasks
- `src/pokemon_storage_system.c` — complex UI, windows, party icons
- `src/party_menu.c` — party display and input

**Confidence:** HIGH

---

## 5. Pokémon Front Sprite and Icon in Custom Screen

### 5.1 Pokémon front sprite

**Source:** `src/decompress.c`, `src/pokemon_storage_system.c`, `src/trade.c`

```c
#include "decompress.h"

// Allocate buffer (e.g. in EWRAM or use gMonSpritesGfxPtr)
LoadSpecialPokePic(destBuffer, species, personality, TRUE);  // TRUE = front pic
LZ77UnCompWram(GetMonSpritePalFromSpeciesAndPersonality(species, isShiny, personality), palBuffer);
```

Then copy tiles to VRAM and load palette. For a sprite:

```c
CpuCopy32(destBuffer, spriteTilePtr, MON_PIC_SIZE);
LoadPalette(palBuffer, paletteOffset, PLTT_SIZE_4BPP);
```

`HandleLoadSpecialPokePic` is a wrapper. Use `gMonSpritesGfxPtr->spritesGfx[...]` if using the shared mon sprite buffers (see `pokemon_storage_system.c`, `trade.c`).

### 5.2 Pokémon icon

**Source:** `src/pokemon_icon.c`, `include/pokemon_icon.h`

```c
#include "pokemon_icon.h"

LoadMonIconPalettes();  // or LoadMonIconPalette(species) for one
u8 spriteId = CreateMonIcon(species, SpriteCB_MonIcon, x, y, subpriority, personality);
// Or CreateMonIconNoPersonality(species, callback, x, y, subpriority)
```

Icons are 32×32, animated. Position with `x`, `y` (pixel coords). Free with `FreeAndDestroyMonIconSprite(&gSprites[spriteId])`.

**Confidence:** HIGH

---

## 6. Den Lobby Screen Step-by-Step

1. **Create `src/raid_den_lobby.c`**
   - `CB2_InitDenLobbyScreen()` — entry, sets `gMain.savedCallback = CB2_ReturnToFieldContinueScript`
   - `CB2_DenLobbyScreen()` — `RunTasks()` only
   - `Task_DenLobby_Main` — state machine: init → wait input → handle choice → exit

2. **Background**
   - Left: orange rect via `struct BgTemplate` + tilemap or `FillBgTilemapRect`
   - Right: separate BG or window

3. **Boss silhouette**
   - Use `LoadSpecialPokePic` for front pic, or a pre-made silhouette asset
   - Render as a sprite or in a window

4. **Star rating**
   - Text (`AddTextPrinterParameterized`) or small sprites

5. **Trainer name**
   - `GetPlayerName()` or `GetMonData(..., MON_DATA_NICKNAME)` for display
   - `AddTextPrinterParameterized` with a `struct WindowTemplate`

6. **Party Pokémon icon**
   - `CreateMonIcon(GetMonData(partyMon, MON_DATA_SPECIES), ...)` at desired (x, y)

7. **Menu**
   - `CreateMenu`, `Menu_PrintItems`, or manual `AddTextPrinterParameterized` for each option
   - Handle input with `JOY_HELD` / `JOY_NEW` in the task

8. **Return**
   - `SetMainCallback2(gMain.savedCallback)`
   - `FreeAllWindowBuffers()`
   - `DestroyTask(taskId)`

**Confidence:** MEDIUM (pattern is standard; exact layout is project-specific)

---

## 7. Summary Table

| Task | Location | Key APIs |
|------|----------|----------|
| Map object | `data/maps/<Map>/map.json` | `object_events` array |
| Graphics | `object_event_graphics.h`, `object_event_graphics_info.h` | `INCBIN_*`, `ObjectEventGraphicsInfo` |
| Constants | `include/constants/event_objects.h` | `OBJ_EVENT_GFX_*` |
| Pointers | `object_event_graphics_info_pointers.h` | `gObjectEventGraphicsInfoPointers` |
| Script | `data/scripts/*.inc` | `checkitem`, `removeitem`, `goto_if_eq VAR_RESULT` |
| Special | `src/field_specials.c` | Add handler, register in specials table |
| CB2 screen | New `src/*.c` | `SetMainCallback2`, `CreateTask`, `RunTasks` |
| Mon pic | `decompress.h` | `LoadSpecialPokePic`, `HandleLoadSpecialPokePic` |
| Mon icon | `pokemon_icon.h` | `CreateMonIcon`, `LoadMonIconPalettes` |
| Graphics swap | `event_object_movement.h` | `ObjectEventSetGraphicsId`, `ObjectEventSetGraphicsIdByLocalIdAndMap` |

---

## Confidence Assessment

| Area | Level | Notes |
|------|-------|-------|
| Map JSON | HIGH | Verified against Route104, Route117_PokemonDayCare |
| Object graphics | HIGH | Matches berry tree, breakable rock, item ball patterns |
| Scripts | HIGH | checkitem/removeitem/VAR_RESULT from scrcmd.c, NewMauville |
| CB2 pattern | HIGH | Matches wallclock, contest, shop |
| Mon sprite/icon | HIGH | From decompress.c, pokemon_icon.c, pokemon_storage_system.c |
| Special for graphics | MEDIUM | `ObjectEventSetGraphicsIdByLocalIdAndMap` exists; script binding needs new special |
