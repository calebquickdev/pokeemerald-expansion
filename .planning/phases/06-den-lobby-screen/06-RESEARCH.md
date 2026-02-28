# Phase 6: Den Lobby Screen - Research

**Researched:** 2026-02-27
**Domain:** GBA full-screen UI (CB2), Pokémon front sprite loading, party icon, party menu callback, menu construction
**Confidence:** HIGH

## Summary

Phase 6 builds `CB2_DenLobbyScreen` — a custom full-screen UI that shows before a raid battle. This codebase has a well-established pattern for exactly this kind of screen (diploma, egg-hatch, contest). All five sub-tasks have clear verified implementation paths using existing engine APIs.

The canonical minimal CB2 screen is `src/diploma.c`: entry function → `DmaFill16` clear → init BGs → init windows → `CreateTask` → `SetMainCallback2`. The task drives state via a simple chain of callbacks (fade in → wait input → fade out → return). This pattern is directly reusable for the lobby.

For the boss sprite, the `HandleLoadSpecialPokePic` + `SetMultiuseSpriteTemplateToPokemon` + `CreateSprite` chain used in `src/egg_hatch.c` and `src/pokemon_sprite_visualizer.c` is the verified path. Silhouette is achieved by overwriting the sprite palette with `RGB_BLACK` after loading. The party icon uses `LoadMonIconPalette` + `CreateMonIcon`. The menu uses `PrintMenuTable` + `InitMenuInUpperLeftCornerNormal` + `Menu_ProcessInputNoWrap`, matching the shop menu pattern.

**Primary recommendation:** Model the screen directly on `src/diploma.c` (simplest real CB2 screen). Reuse `EggHatchCreateMonSprite`-style code for the boss sprite. Use `InitPartyMenu(..., PARTY_ACTION_CHOOSE_AND_CLOSE, ..., CB2_DenLobbyScreen)` for the "Change Pokémon" flow.

---

## Existing Stubs (Current State)

### `src/raid_den.c`

`OpenDenLobbyScreen()` currently just calls `DoRaidBattle()` — a pass-through placeholder:

```c
void OpenDenLobbyScreen(void)
{
    DoRaidBattle();
}
```

`RollDynamaxDenPokemon()` returns a hardcoded species:

```c
u16 RollDynamaxDenPokemon(u8 denId)
{
    (void)denId;
    return SPECIES_RALTS;
}
```

`struct DynamaxDen` in `include/raid_den.h`:

```c
struct DynamaxDen {
    u16 species;
    u8 isGmax;
    u8 starRating;
};
```

`DoRaidBattle()` signature (to call from the lobby):
```c
// Takes denId from gSpecialVar_0x8000 internally; sets gBattleTypeFlags then CreateBattleStartTask
void DoRaidBattle(void);
```

---

## Standard Stack

### Core
| API | Header | Purpose |
|-----|--------|---------|
| `SetMainCallback2` / `SetVBlankCallback` | `main.h` | Register main/vblank callbacks |
| `CreateTask` / `DestroyTask` / `RunTasks` | `task.h` | Task-driven state machine |
| `ResetBgsAndClearDma3BusyFlags` / `InitBgsFromTemplates` | `bg.h` | Background init |
| `InitWindows` / `AddWindow` / `RemoveWindow` | `window.h` | Window management |
| `FillWindowPixelBuffer` / `PutWindowTilemap` / `CopyWindowToVram` | `window.h` | Rendering text to BG |
| `AddTextPrinterParameterized4` | `menu.h` | Print text to a window |
| `LoadPalette` / `FillPalette` / `BlendPalettes` | `palette.h` | Palette management |
| `BeginNormalPaletteFade` | `palette.h` | Fade in/out |
| `AllocateMonSpritesGfx` / `FreeMonSpritesGfx` | `battle_gfx_sfx_util.h` | Sprite GFX allocator |
| `HandleLoadSpecialPokePic` | `decompress.h` | Load front sprite pixels into buffer |
| `SetMultiuseSpriteTemplateToPokemon` | `pokemon.h` | Configure sprite template |
| `CreateSprite` / `DestroySprite` | `sprite.h` | Sprite lifecycle |
| `LoadCompressedSpritePaletteWithTag` | `decompress.h` | Load sprite palette |
| `GetMonFrontSpritePal` | `pokemon.h` | Get palette pointer from a `Pokemon*` |
| `GetMonSpritePalFromSpecies` | `pokemon.h` | Get palette pointer from species ID |
| `LoadMonIconPalette` / `CreateMonIcon` / `SpriteCB_MonIcon` | `pokemon_icon.h` | Party icon sprite |
| `FreeAndDestroyMonIconSprite` | `pokemon_icon.h` | Clean up icon sprite |
| `PrintMenuTable` / `InitMenuInUpperLeftCornerNormal` | `menu.h` | Menu rendering |
| `Menu_ProcessInputNoWrap` | `menu.h` | Menu input — returns index, `MENU_NOTHING_CHOSEN(-2)`, or `MENU_B_PRESSED(-1)` |
| `SetStandardWindowBorderStyle` / `DrawStdWindowFrame` | `menu.h` | Window frame |
| `InitPartyMenu` (via `party_menu.c`) | `party_menu.h` | Open party screen |
| `GetCursorSelectionMonId` | `party_menu.h` | Read selected slot after party screen exits |
| `GetTotalBaseStat` | `battle_ai_util.h` | Sum of all 6 base stats for species |

### Supporting
| API | Header | Purpose |
|-----|--------|---------|
| `gSpeciesInfo[species].isGigantamax` | `data.h` (gSpeciesInfo) | Check if species is a GMAX form |
| `gSaveBlock2Ptr->playerName` | `global.h` / `save_block.h` | Player name (encoded string) |
| `GetMonData(mon, MON_DATA_SPECIES, NULL)` | `pokemon.h` | Read species from party slot |
| `GetMonData(mon, MON_DATA_PERSONALITY, NULL)` | `pokemon.h` | Read PID for icon/sprite |
| `struct MenuAction { const u8 *text; union { void (*void_u8)(u8); } func; }` | `menu.h` | Menu item definition |

---

## Architecture Patterns

### CB2 Screen Skeleton (from `src/diploma.c`)

```c
// VBlank callback — always the same for simple screens
static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// MainCB2 — drives the screen
static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

// Entry point — called from the overworld or script
void CB2_DenLobbyScreen(void)
{
    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    // DMA-clear VRAM, OAM, PLTT
    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    // Load BGs, windows, sprites...
    BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    EnableInterrupts(1);
    SetVBlankCallback(VBlankCB);
    SetMainCallback2(MainCB2);
    CreateTask(Task_LobbyFadeIn, 0);
}

// Task chain
static void Task_LobbyFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_LobbyMain;
}

static void Task_LobbyMain(u8 taskId)
{
    // handle menu input, button presses
    s8 choice = Menu_ProcessInputNoWrap();
    // ...
}

static void Task_LobbyFadeOut(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        // free resources
        FreeAllWindowBuffers();
        FreeMonSpritesGfx();
        DestroyTask(taskId);
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
    }
}
```

### Re-entry Pattern for Party Screen

When "Change Pokémon" is selected, the lobby screen must:
1. Free/save sprite state (or accept that `CB2_DenLobbyScreen` will fully reinitialize on return)
2. Call `InitPartyMenu(PARTY_MENU_TYPE_FIELD, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_AND_CLOSE, FALSE, PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput, CB2_DenLobbyScreen)`
3. `InitPartyMenu` calls `SetMainCallback2(CB2_InitPartyMenu)` internally, transferring control to the party screen
4. When the party screen closes, `gPartyMenu.exitCallback` (`CB2_DenLobbyScreen`) is invoked
5. On re-entry, read `GetCursorSelectionMonId()` — returns `gPartyMenu.slotId`; if `>= PARTY_SIZE`, user cancelled

**Key state persistence:** The lobby needs an EWRAM variable to carry the selected slot across re-entry:

```c
static EWRAM_DATA u8 sLobbySelectedSlot = 0;

// In Task_LobbyMain, "Change Pokémon" option:
static void Task_LobbyChangePokemon(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_LobbyWaitFadeBeforeParty;
}

static void Task_LobbyWaitFadeBeforeParty(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        // Full cleanup before handing off
        FreeAllWindowBuffers();
        FreeMonSpritesGfx();
        DestroyTask(taskId);
        InitPartyMenu(PARTY_MENU_TYPE_FIELD, PARTY_LAYOUT_SINGLE,
                      PARTY_ACTION_CHOOSE_AND_CLOSE, FALSE,
                      PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput,
                      CB2_DenLobbyScreen);
    }
}

// On CB2_DenLobbyScreen re-entry, check if returning from party screen:
// gPartyMenu.slotId < PARTY_SIZE means valid selection
```

### Boss Front Sprite — Full Load Sequence (from `src/egg_hatch.c`)

```c
// Step 1 (can happen across two task states for DMA spread):
AllocateMonSpritesGfx();
HandleLoadSpecialPokePic(TRUE,  // TRUE = front sprite
                         gMonSpritesGfxPtr->spritesGfx[B_POSITION_OPPONENT_LEFT],
                         species, personality);
LoadCompressedSpritePaletteWithTag(GetMonSpritePalFromSpecies(species, FALSE, FALSE), species);

// Step 2 (after DMA completes — next task state):
SetMultiuseSpriteTemplateToPokemon(species, B_POSITION_OPPONENT_LEFT);
u8 spriteId = CreateSprite(&gMultiuseSpriteTemplate, x, y, subpriority);
gSprites[spriteId].callback = SpriteCallbackDummy;
```

### Silhouette Effect

After the sprite is created, override its palette with solid black (skip index 0 = transparent):

```c
// Source: src/battle_anim_dark.c lines 515,525 (uses FillPalette for BG; same principle for OBJ)
// For an OBJ sprite:
u8 palNum = gSprites[spriteId].oam.paletteNum;
FillPalette(RGB_BLACK, OBJ_PLTT_ID(palNum) + 1, PLTT_SIZE_4BPP - 2);
```

`OBJ_PLTT_ID(n)` is defined in `gba/gba.h` or `gba/macro.h` and equals `256 + n*16`. Size `PLTT_SIZE_4BPP` = 32 bytes = 16 colors × 2 bytes; `-2` to skip color 0 (transparent).

### Party Icon

```c
// Must call before CreateMonIcon, or palette won't be loaded:
LoadMonIconPalette(species);
u8 iconSpriteId = CreateMonIcon(species, SpriteCB_MonIcon, x, y, subpriority, personality);

// On cleanup:
FreeAndDestroyMonIconSprite(&gSprites[iconSpriteId]);
FreeMonIconPalette(species);
```

`SpriteCB_MonIcon` drives the two-frame flip animation automatically.

### Star Rating Text

Stars are drawn via text tiles (the `★` character is available in the font). Use `AddTextPrinterParameterized4` with a pre-built string. Alternatively, draw `★` characters using tile indices directly into the BG tilemap.

### Trainer Name

```c
// gSaveBlock2Ptr->playerName is the encoded name string (GBA font encoding, not ASCII)
// Use AddTextPrinterParameterized4 directly — the text subsystem handles the encoding
u8 color[3] = { TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY };
AddTextPrinterParameterized4(windowId, FONT_NORMAL, x, y, 0, 0, color, TEXT_SKIP_DRAW,
                              gSaveBlock2Ptr->playerName);
PutWindowTilemap(windowId);
CopyWindowToVram(windowId, COPYWIN_FULL);
```

### Menu Construction (from `src/shop.c`)

```c
static const u8 sText_InviteOthers[]    = _("Invite Others");
static const u8 sText_DontInviteOthers[]= _("Don't Invite Others");
static const u8 sText_Quit[]            = _("Quit");

static const struct MenuAction sLobbyMenuActions[] =
{
    { sText_InviteOthers,     { .void_u8 = Task_LobbyInviteOthers } },
    { sText_DontInviteOthers, { .void_u8 = Task_LobbyStartRaid } },
    { sText_Quit,             { .void_u8 = Task_LobbyQuit } },
};

// Setup (called once during screen init):
u8 windowId = AddWindow(&sLobbyMenuWinTemplate);
SetStandardWindowBorderStyle(windowId, FALSE);
PrintMenuTable(windowId, ARRAY_COUNT(sLobbyMenuActions), sLobbyMenuActions);
InitMenuInUpperLeftCornerNormal(windowId, ARRAY_COUNT(sLobbyMenuActions), 0);
PutWindowTilemap(windowId);
CopyWindowToVram(windowId, COPYWIN_MAP);

// In Task_LobbyMain each frame:
s8 input = Menu_ProcessInputNoWrap();
switch (input)
{
case MENU_NOTHING_CHOSEN: break;
case MENU_B_PRESSED:      Task_LobbyQuit(taskId); break;
default:                  sLobbyMenuActions[input].func.void_u8(taskId); break;
}
```

---

## RollDynamaxDenPokemon Implementation (Task 06-01)

`GetTotalBaseStat(species)` in `include/battle_ai_util.h` returns the sum of all 6 base stats from `gSpeciesInfo`:

```c
// Source: src/battle_ai_util.c:325
u32 GetTotalBaseStat(u32 species)
{
    return gSpeciesInfo[species].baseHP
        + gSpeciesInfo[species].baseAttack
        + gSpeciesInfo[species].baseDefense
        + gSpeciesInfo[species].baseSpeed
        + gSpeciesInfo[species].baseSpAttack
        + gSpeciesInfo[species].baseSpDefense;
}
```

GMAX form detection: `gSpeciesInfo[species].isGigantamax` (bool field, `TRUE` only for `*_GMAX` species IDs).

BST thresholds are already decided:

```c
static u8 BstToStarRating(u32 bst)
{
    if (bst <= 299) return 1;
    if (bst <= 460) return 2;
    if (bst <= 494) return 3;
    if (bst <= 549) return 4;
    return 5;
}

u16 RollDynamaxDenPokemon(u8 denId)
{
    // Roll a random non-GMAX species from gSpeciesInfo
    // Then set starRating and isGmax on gSaveBlock2Ptr->dynamaxDens[denId]
    u16 species = /* random valid base-form species */;
    bool8 isGmax = /* random chance or locked to FALSE for base roll */;
    u8 stars;

    if (isGmax)
        stars = 5;
    else
        stars = BstToStarRating(GetTotalBaseStat(species));

    gSaveBlock2Ptr->dynamaxDens[denId].species  = species;
    gSaveBlock2Ptr->dynamaxDens[denId].isGmax   = isGmax;
    gSaveBlock2Ptr->dynamaxDens[denId].starRating = stars;
    return species;
}
```

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead |
|---------|-------------|-------------|
| Pokémon front sprite loading | Custom decompressor | `HandleLoadSpecialPokePic(TRUE, buffer, species, pid)` |
| Sprite palette loading | Manual VRAM writes | `LoadCompressedSpritePaletteWithTag(ptr, tag)` |
| Silhouette effect | Custom shader | `FillPalette(RGB_BLACK, OBJ_PLTT_ID(palNum)+1, PLTT_SIZE_4BPP-2)` |
| Party icon animation | Custom frame timer | `CreateMonIcon(..., SpriteCB_MonIcon, ...)` |
| Party screen invocation | Custom selection UI | `InitPartyMenu(PARTY_MENU_TYPE_FIELD, ..., PARTY_ACTION_CHOOSE_AND_CLOSE, ..., CB2_DenLobbyScreen)` |
| BST calculation | Sum loop over species data | `GetTotalBaseStat(species)` from `battle_ai_util.h` |
| GMAX detection | String suffix check or ID range | `gSpeciesInfo[species].isGigantamax` |
| Menu cursor + text | Manual tile drawing | `PrintMenuTable` + `InitMenuInUpperLeftCornerNormal` + `Menu_ProcessInputNoWrap` |

---

## Common Pitfalls

### Pitfall 1: Forgetting `LoadMonIconPalette` Before `CreateMonIcon`
`CreateMonIcon` sets `paletteTag` in the template, but if the palette hasn't been loaded into OAM palette RAM yet, the icon renders with garbage colors or black.
**Prevention:** Always call `LoadMonIconPalette(species)` immediately before `CreateMonIcon`.

### Pitfall 2: Silhouette Palette Covers Color 0 (Transparent)
`FillPalette(RGB_BLACK, OBJ_PLTT_ID(palNum), PLTT_SIZE_4BPP)` would overwrite color 0, turning the transparent background black. The sprite's border becomes opaque.
**Prevention:** Start at `OBJ_PLTT_ID(palNum) + 1` and size `PLTT_SIZE_4BPP - 2`.

### Pitfall 3: CB2 Re-entry After Party Screen Reinitializes Everything
`InitPartyMenu` calls `SetMainCallback2(CB2_InitPartyMenu)` immediately. When it's done, it calls the `exitCallback` — which is `CB2_DenLobbyScreen` — as a fresh `CB2`. The lobby screen runs its full initialization path again. This means:
- All EWRAM_DATA lobby state vars are still valid (survive across CB2 change)
- Sprite IDs from the previous run are gone; new sprites are created fresh on re-entry
- The selected slot must be read from `GetCursorSelectionMonId()` early in the re-entry path before state is reset

**Prevention:** In `CB2_DenLobbyScreen`, detect re-entry via a flag (e.g., `sLobbyReturnedFromParty`) and skip the denId read/roll while honoring the slot read.

### Pitfall 4: `gSpecialVar_0x8000` Is Consumed by `DoRaidBattle`
`DoRaidBattle` reads `denId` from `gSpecialVar_0x8000`. The lobby screen is entered via `Special_OpenDenLobbyScreen` which sets this variable. If the lobby screen re-enters itself (party screen round-trip), `gSpecialVar_0x8000` is still set (scripts don't clear it), so re-reading it is safe. But if any code path calls a script between lobby entry and `DoRaidBattle`, the var may be overwritten.
**Prevention:** Cache `(u8)gSpecialVar_0x8000` into an `EWRAM_DATA` variable immediately on first entry to the lobby.

### Pitfall 5: `AllocateMonSpritesGfx` Must Be Called Before `HandleLoadSpecialPokePic`
`HandleLoadSpecialPokePic` writes into `gMonSpritesGfxPtr->spritesGfx[position]`, which is only valid after `AllocateMonSpritesGfx()`.
**Prevention:** Always pair `AllocateMonSpritesGfx()` + load + `FreeMonSpritesGfx()` on teardown.

### Pitfall 6: Species Pool for `RollDynamaxDenPokemon` Must Exclude Invalid Species
`gSpeciesInfo` contains entries for `SPECIES_NONE` (index 0) and `SPECIES_EGG`. Iterating over the full `NUM_SPECIES` range risks selecting these.
**Prevention:** Skip species where `GetTotalBaseStat(species) == 0` or explicitly check `species != SPECIES_NONE && species != SPECIES_EGG`.

---

## Code Examples

### Minimal CB2 Screen Entry (Source: `src/diploma.c`)

```c
void CB2_ShowDiploma(void)
{
    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0);
    // ... zero all GPU regs ...
    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    // load palettes, BGs, windows
    BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    EnableInterrupts(1);
    SetVBlankCallback(VBlankCB);
    SetMainCallback2(MainCB2);
    CreateTask(Task_DiplomaFadeIn, 0);
}
```

### Front Sprite Load + Silhouette (Source: `src/egg_hatch.c:448`, palette override from `src/battle_anim_dark.c:515`)

```c
AllocateMonSpritesGfx();
HandleLoadSpecialPokePic(TRUE, gMonSpritesGfxPtr->spritesGfx[B_POSITION_OPPONENT_LEFT],
                         species, personality);
LoadCompressedSpritePaletteWithTag(GetMonSpritePalFromSpecies(species, FALSE, FALSE), species);
// (next task state, after DMA)
SetMultiuseSpriteTemplateToPokemon(species, B_POSITION_OPPONENT_LEFT);
u8 spriteId = CreateSprite(&gMultiuseSpriteTemplate, 60, 60, 1);
// Silhouette: fill OBJ palette with black, skipping transparent color 0
u8 palNum = gSprites[spriteId].oam.paletteNum;
FillPalette(RGB_BLACK, OBJ_PLTT_ID(palNum) + 1, PLTT_SIZE_4BPP - 2);
```

### Party Icon (Source: `src/party_menu.c:4211`, `src/pokemon_icon.c:137`)

```c
LoadMonIconPalette(species);
u8 iconSpriteId = CreateMonIcon(species, SpriteCB_MonIcon, x, y, 4, personality);
// Later, cleanup:
FreeAndDestroyMonIconSprite(&gSprites[iconSpriteId]);
FreeMonIconPalette(species);
```

### "Change Pokémon" → Party Screen → Return (Source: `src/party_menu.c:7640`)

```c
// Leave the lobby (fade + cleanup first, then hand off):
InitPartyMenu(PARTY_MENU_TYPE_FIELD, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_AND_CLOSE,
              FALSE, PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput, CB2_DenLobbyScreen);

// In CB2_DenLobbyScreen on re-entry, check for return:
u8 slot = GetCursorSelectionMonId();  // == gPartyMenu.slotId
if (slot < PARTY_SIZE)
    sLobbySelectedSlot = slot;       // valid selection
// else: cancel — keep previous sLobbySelectedSlot
```

---

## Open Questions

1. **Orange background tint**: The left half should be orange. The simplest approach is to load a single-color palette on BG3 and fill that BG layer with solid tiles on the left half (tiles 0–14 columns). Alternatively, use `SetBgTilemapPalette` for per-tile palette swapping. No blocker — both are straightforward once BGs are initialized.

2. **Star rendering**: The `★` glyph exists in the GBA font (confirmed by presence in text elsewhere). Drawing 1–5 stars can be done inline as text with `AddTextPrinterParameterized4`. If the font lacks `★`, use a sprite or BG tile instead. Flag as needing visual verification in emulator.

3. **Species pool selection logic**: `RollDynamaxDenPokemon` needs a valid base-form species list. The full `gSpeciesInfo` array includes legendaries, eggs, and GMAX forms. A clean pool requires iterating and filtering (non-zero BST, not GMAX, not egg). For Phase 6 a small hardcoded table or a filtered scan at roll-time is acceptable; a curated per-den table is a deferred idea.

---

## Sources

### Primary (HIGH confidence)
- `src/diploma.c` — CB2 screen scaffold, task chain, BG/window init
- `src/egg_hatch.c:423–464,491–570` — `HandleLoadSpecialPokePic` + `AllocateMonSpritesGfx` + `SetMultiuseSpriteTemplateToPokemon` + `CreateSprite` pattern
- `src/pokemon_sprite_visualizer.c:1968–2001` — complete front sprite init sequence
- `src/pokemon_icon.c:137–163` — `CreateMonIcon` internals
- `include/pokemon_icon.h` — full icon API
- `src/battle_ai_util.c:325–333` — `GetTotalBaseStat` implementation
- `src/battle_dynamax.c:131–137` — `gSpeciesInfo[species].isGigantamax` usage
- `src/shop.c:166–410` — `struct MenuAction` + `PrintMenuTable` + `Menu_ProcessInputNoWrap` pattern
- `include/menu.h` — full menu API
- `src/party_menu.c:523–570, 1409–1412, 7700–7712` — `InitPartyMenu`, `GetCursorSelectionMonId`, `PARTY_ACTION_CHOOSE_AND_CLOSE` flow
- `include/party_menu.h` — full party menu API
- `src/raid_den.c` — current stubs for `OpenDenLobbyScreen`, `RollDynamaxDenPokemon`, `DoRaidBattle`
- `include/raid_den.h` — `struct DynamaxDen` definition

### Secondary (MEDIUM confidence)
- `src/battle_anim_dark.c:515` — `FillPalette(RGB_BLACK, ...)` for silhouette effect (verified pattern, slight difference BG vs OBJ palette offset)
- `src/menu.c:2112,2248` — `gSaveBlock2Ptr->playerName` as direct text string

---

## Metadata

**Confidence breakdown:**
- CB2 screen scaffold: HIGH — multiple verified real examples
- Front sprite + silhouette: HIGH — egg_hatch.c + pokemon_sprite_visualizer.c are direct evidence
- Party icon: HIGH — pokemon_icon.h fully documented, usage in raid_ally, party_menu, visualizer
- Party screen callback: HIGH — InitPartyMenu + exitCallback pattern fully traced
- BST lookup: HIGH — GetTotalBaseStat implementation read directly
- GMAX detection: HIGH — isGigantamax field confirmed in species data + battle_dynamax.c
- Menu construction: HIGH — shop.c pattern fully read
- Trainer name: HIGH — gSaveBlock2Ptr->playerName confirmed in multiple sites

**Research date:** 2026-02-27
**Valid until:** 2026-03-30 (stable engine, no external deps)
