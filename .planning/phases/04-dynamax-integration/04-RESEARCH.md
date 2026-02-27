# Phase 4: Dynamax Integration — Research

**Researched:** 2026-02-27
**Domain:** pokeemerald-expansion battle engine — Dynamax gimmick system, battle sprite management
**Confidence:** HIGH — all findings verified directly from codebase source files

---

## Summary

Phase 4 implements the Dynamax rotation cycle (who can Dynamax each turn) and the ally sprite swap (icon → full front sprite on Dynamax, reverse on UndoDynamax). Phase 3 already added the controller layout, boss permanent Dynamax, and `GetBattlerSide` guard for battler 3. Phase 4 builds directly on that infrastructure.

The codebase has a commented-out stub in `CanDynamax()` (`battle_dynamax.c:116`) that explicitly names the `gBattleStruct->raid.dynamaxEnergy` field — Phase 4 just adds `struct RaidData` to `BattleStruct`, defines the field, and activates the check. The gating flows naturally: `CanDynamax` → `CanActivateGimmick` → `AssignUsableGimmicks` → gimmick trigger sprite shown only for eligible battler. The player controller checks this at turn-start so the START button only works for the eligible battler.

For ally sprites, Phase 3 left allies with inconsistent sprites (back pixels/animation for battler 2, back pixels but front animation for battler 3). Phase 4 overrides `RaidAllyHandleLoadMonSprite` to: (a) load a full front sprite into `gBattlerSpriteIds[battler]` (initially invisible) for battle animations and targeting, and (b) create a 32×32 icon sprite at the ally's screen position (initially visible). Sprite swap hooks live in `ActivateDynamax()` and `UndoDynamax()`, guarded by RAID + player-side + non-player.

**Primary recommendation:** Add `struct RaidData { u8 dynamaxEnergy; u8 shieldHp; u8 respawnTimer[3]; u8 allyIconSpriteId[2]; }` to `BattleStruct`; activate the existing stub in `CanDynamax`; skip `HasTrainerUsedGimmick` in RAID; advance rotation in `BattleTurnPassed()` before `AssignUsableGimmicks()`; manage icon ↔ full-sprite swap in `ActivateDynamax`/`UndoDynamax`.

---

## Q1: CanDynamax() — Exact Location and Gating

**File:** `src/battle_dynamax.c:74`
**Signature:** `bool32 CanDynamax(u32 battler)`

### Current checks (in order)
| Line | Check | What it does |
|------|-------|--------------|
| 80 | `BATTLE_TYPE_FIRST_BATTLE && opponent` | Blocks Zigzagoon from Dynamaxing in tutorial |
| 84–91 | Player position + Dynamax Band item | Player must have band + flag set |
| 93–97 | Species exclusion | Blocks Zacian, Zamazenta, Eternatus |
| 100 | `HasTrainerUsedGimmick(battler, GIMMICK_DYNAMAX)` | **Blocks re-use this battle** |
| 104 | `ShouldTrainerBattlerUseGimmick(battler, GIMMICK_DYNAMAX)` | Trainer party `shouldUseDynamax` flag |
| 108 | `GetActiveGimmick(battler) != GIMMICK_NONE` | No stacking gimmicks |
| 112 | Hold effect Z-Crystal or Mega Stone | Items block Dynamax |
| 116–117 | **(commented out)** `gBattleStruct->raid.dynamaxEnergy != battler` | **Phase 4 activates this** |

### Phase 4 modifications to CanDynamax

**Problem 1 — `HasTrainerUsedGimmick` cross-contaminates raid allies:**
`SetGimmickAsActivated(battler, GIMMICK_DYNAMAX)` (called inside `ActivateDynamax`) also marks `BATTLE_PARTNER(battler)` as activated because `IsPartnerMonFromSameTrainer` returns TRUE in RAID (RAID doesn't set `BATTLE_TYPE_MULTI` or `BATTLE_TYPE_INGAME_PARTNER`). This would permanently block the partner from Dynamaxing on their rotation turn.

**Fix:** In `CanDynamax`, skip the `HasTrainerUsedGimmick` check for RAID player-side battlers. The rotation check is the sole gate:

```c
// Before existing HasTrainerUsedGimmick check:
if (!(gBattleTypeFlags & BATTLE_TYPE_RAID && GetBattlerSide(battler) == B_SIDE_PLAYER))
{
    if (HasTrainerUsedGimmick(battler, GIMMICK_DYNAMAX))
        return FALSE;
    if (!ShouldTrainerBattlerUseGimmick(battler, GIMMICK_DYNAMAX))
        return FALSE;
}
```

**Note:** `ShouldTrainerBattlerUseGimmick` already returns TRUE for all player-side battlers in RAID (they aren't BATTLE_TYPE_MULTI and their side == B_SIDE_PLAYER). So only `HasTrainerUsedGimmick` needs bypassing.

**Problem 2 — activate the rotation check:**
Uncomment and finalize the stub at line 116:

```c
// Already authored in the codebase, just needs the struct to exist:
if (gBattleTypeFlags & BATTLE_TYPE_RAID && gBattleStruct->raid.dynamaxEnergy != battler)
    return FALSE;
```

With these two changes, `CanDynamax` returns TRUE only for the battler whose index matches `dynamaxEnergy`. All other battlers return FALSE, which propagates to `CanActivateGimmick` → `AssignUsableGimmicks` → `usableGimmick[battler] = GIMMICK_NONE` for everyone except the eligible battler.

**Confidence:** HIGH — full source read; cross-contamination verified through `IsPartnerMonFromSameTrainer` source.

---

## Q2: Player Controller Dynamax Button Gate

**File:** `src/battle_controller_player.c`

### UI flow
1. Each turn start: `BattleTurnPassed()` at `src/battle_main.c:4001` calls `AssignUsableGimmicks()`.
2. `AssignUsableGimmicks` iterates all battlers, calls `CanActivateGimmick(battler, GIMMICK_DYNAMAX)` → `CanDynamax(battler)`.
3. `gBattleStruct->gimmick.usableGimmick[battler]` = `GIMMICK_DYNAMAX` only for eligible battler; others get `GIMMICK_NONE`.
4. **Trigger sprite creation** (`CreateGimmickTriggerSprite`, called from player controller's `HandleChooseMoveAfterDma3` at line 2136–2139): skips sprite creation if `usableGimmick[battler] == GIMMICK_NONE`. → No visual for out-of-rotation battlers.
5. **START button handler** (line 880): `if (gBattleStruct->gimmick.usableGimmick[battler] != GIMMICK_NONE && !HasTrainerUsedGimmick(...))` — since `usableGimmick` is NONE for out-of-rotation battlers, START does nothing.

**For raid allies (battlers 2 and 3):** `RaidAllyHandleChooseMove` (in `battle_controller_raid_ally.c:347`) checks `usableGimmick[battler] != GIMMICK_NONE` before emitting `RET_GIMMICK`. With `CanDynamax` returning FALSE for non-eligible allies, `usableGimmick` is NONE and allies skip the Dynamax path automatically.

**No changes needed in the player controller itself** — gating in `CanDynamax` is sufficient.

**Confidence:** HIGH — full player controller and gimmick flow read.

---

## Q3: BattleStruct — Adding RaidData

**File:** `include/battle.h`, struct `BattleStruct` (starts at line 648, ends at line 840)

### Current state
No `raid` field exists in `BattleStruct`. The commented-out code at `battle_dynamax.c:116` uses `gBattleStruct->raid.dynamaxEnergy` — this is the authorial naming intent.

### Where to add
Append after the existing `struct BattleGimmickData gimmick;` field (line 758) or at the end of `BattleStruct` (before line 840). Near gimmick data is most cohesive:

```c
// In include/battle.h, after struct BattleGimmickData gimmick:
struct RaidData raid;
```

### struct RaidData definition
Per the phase plan (04-01), add to `include/battle.h` alongside `struct DynamaxData` and `struct BattleGimmickData`:

```c
struct RaidData
{
    u8 dynamaxEnergy;         // current eligible battler index (0, 2, or 3); 0xFF = nobody
    u8 allyIconSpriteId[2];  // sprite IDs for icon sprites of battlers 2 and 3
    u8 shieldHp;             // Phase 5: boss shield HP (zeroed this phase)
    u8 respawnTimer[3];      // Phase 5: faint-respawn countdowns (zeroed this phase)
};
```

### Initialization
In `DoRaidBattle()` (src/raid_den.c) or the `BATTLE_TYPE_RAID` branch of `InitSinglePlayerBtlControllers()`:

```c
gBattleStruct->raid.dynamaxEnergy = 0;      // Turn 1: player eligible
gBattleStruct->raid.allyIconSpriteId[0] = MAX_SPRITES;
gBattleStruct->raid.allyIconSpriteId[1] = MAX_SPRITES;
gBattleStruct->raid.shieldHp = 0;
memset(gBattleStruct->raid.respawnTimer, 0, sizeof(gBattleStruct->raid.respawnTimer));
```

**Confidence:** HIGH — struct layout read directly; no `raid` field confirmed absent via search.

---

## Q4: dynamaxTurns[] — Per-Battler Mechanics

**File:** `include/battle.h:572` (`struct DynamaxData`), `src/battle_dynamax.c`, `src/battle_util.c:2854`

### How it works
```c
struct DynamaxData {
    u8 dynamaxTurns[MAX_BATTLERS_COUNT];   // one per battler
    u16 baseMoves[MAX_BATTLERS_COUNT];
    u16 lastUsedBaseMove;
};
```

- `ActivateDynamax(battler)` (line 181): `dynamaxTurns[battler] = DYNAMAX_TURNS_COUNT` (=3)
- `UndoDynamax(battler)` (line 215): `dynamaxTurns[battler] = 0`
- End-of-turn decrement (battle_util.c:2854–2863): `--dynamaxTurns[battler] == 0` triggers `UndoDynamax` + `BattleScript_DynamaxEnds`
- `DYNAMAX_TURNS_COUNT = 3` defined in `include/battle_dynamax.h:4` — matches requirement of 3 turns counting the activation turn

### Boss protection (already in Phase 3)
- `dynamaxTurns[1] = 0xFF` set in `InitSinglePlayerBtlControllers` (line 151) — 255 turns before natural expiry
- `UndoDynamax` guard at line 200: `if ((gBattleTypeFlags & BATTLE_TYPE_RAID) && GetBattlerSide(battler) == B_SIDE_OPPONENT) return;` — unconditional early exit for boss

### Ally Dynamax duration
Ally Dynamax uses the standard `DYNAMAX_TURNS_COUNT = 3`. No changes needed to the timer mechanic.

**Confidence:** HIGH — all call sites read directly.

---

## Q5: Ally Sprite Current State (Post-Phase 3)

**Files:** `src/battle_controllers.c:2434` (`BtlController_HandleLoadMonSprite`), `src/battle_gfx_sfx_util.c:611` (`BattleLoadMonSpriteGfx`), `src/pokemon.c:2198` (`SetMultiuseSpriteTemplateToPokemon`)

### Current sprite state per battler
| Battler | Position | GetBattlerSide | Sprite pixels (HandleLoadSpecialPokePic) | Template animation |
|---------|----------|---------------|------------------------------------------|-------------------|
| 0 (player) | 0 (PLAYER_LEFT) | B_SIDE_PLAYER | Back sprite | Back anim (gAnims_MonPic) |
| 1 (boss) | 1 (OPPONENT_LEFT) | B_SIDE_OPPONENT | Front sprite | Front anim |
| 2 (ally 1) | 2 (PLAYER_RIGHT) | B_SIDE_PLAYER | Back sprite | Back anim (position 2 = player) |
| 3 (ally 2) | 3 (OPPONENT_RIGHT) | **B_SIDE_PLAYER** (RAID guard) | **Back sprite** (RAID guard) | **Front anim** (position 3 ≠ player) |

Battler 3's back sprite pixels + front animation is inconsistent — the guard makes GetBattlerSide return PLAYER, so `BattleLoadMonSpriteGfx` loads the back sprite, but `SetMultiuseSpriteTemplateToPokemon(species, 3)` uses position 3 (not player positions) and assigns front animation frames. This is tolerable for Phase 3 but Phase 4 replaces this with a designed icon-to-front-sprite system.

**Confidence:** HIGH — all three functions read directly; inconsistency identified through code trace.

---

## Q6: Icon Sprite Loading for Allies

**File:** `src/pokemon_icon.c:137`

### CreateMonIcon signature
```c
u8 CreateMonIcon(u16 species, void (*callback)(struct Sprite *), s16 x, s16 y, u8 subpriority, u32 personality);
```
- Returns sprite ID (u8)
- 32×32 OAM sprite with looping 2-frame animation
- Palette: must pre-load with `LoadMonIconPalette(species)` before first use
- Clean up with `FreeAndDestroyMonIconSprite(&gSprites[spriteId])`

### Screen positions for allies
Both allies appear in the lower-left area of the screen (player side). Suggested positions:
- Battler 2 (ally 1): approx x=32, y=96 (below player's healthbox area)
- Battler 3 (ally 2): approx x=56, y=104 (offset from ally 1)

Exact values are for the planner to decide based on visual layout; these should not overlap with the player's sprite or healthboxes. Can use `GetBattlerSpriteCoord(battler, BATTLER_COORD_X)` + offset as a reference.

### Storage
`gBattleStruct->raid.allyIconSpriteId[battler - 2]` — maps battler 2 → index 0, battler 3 → index 1.

**Confidence:** HIGH — full pokemon_icon.c read.

---

## Q7: Full Front Sprite Loading for Dynamaxed Allies

### How to load a front sprite for a player-side battler

`BattleLoadMonSpriteGfx` internally calls:
```c
HandleLoadSpecialPokePic((GetBattlerSide(battler) == B_SIDE_OPPONENT),
                         gMonSpritesGfxPtr->spritesGfx[position],
                         species, personalityValue);
```

For RAID allies, `GetBattlerSide` returns B_SIDE_PLAYER → back sprite. To force front sprite for the Dynamax state, call directly:

```c
// Load front sprite pixels into the battler's GFX slot
u32 position = GetBattlerPosition(battler);
HandleLoadSpecialPokePic(TRUE,   // TRUE = front sprite
                         gMonSpritesGfxPtr->spritesGfx[position],
                         species, personalityValue);

// Set front animation template (use position 1 = opponent template)
SetMultiuseSpriteTemplateToPokemon(species, B_POSITION_OPPONENT_LEFT);

// Create sprite at ally's actual screen coordinates
gBattlerSpriteIds[battler] = CreateSprite(&gMultiuseSpriteTemplate,
                                           GetBattlerSpriteCoord(battler, BATTLER_COORD_X_2),
                                           GetBattlerSpriteDefault_Y(battler),
                                           GetBattlerSpriteSubpriority(battler));
gSprites[gBattlerSpriteIds[battler]].oam.paletteNum = battler;
gSprites[gBattlerSpriteIds[battler]].data[0] = battler;
gSprites[gBattlerSpriteIds[battler]].data[2] = species;
StartSpriteAnim(&gSprites[gBattlerSpriteIds[battler]], 0);
gSprites[gBattlerSpriteIds[battler]].invisible = TRUE;  // hidden until Dynamax
```

This mirrors how the boss sprite is loaded (position 1 template, front pixels) while placing the sprite at the ally's screen coordinates.

### GFX buffer slots
`gMonSpritesGfxPtr->spritesGfx[MAX_BATTLERS_COUNT]` has 4 independent slots (0–3). Battler 2 uses slot 2, battler 3 uses slot 3. There is no overlap.

**Confidence:** HIGH — `HandleLoadSpecialPokePic`, `SetMultiuseSpriteTemplateToPokemon`, `BattleLoadMonSpriteGfx` all read directly.

---

## Q8: UndoDynamax Hook for Sprite Restoration

**File:** `src/battle_dynamax.c:198`

### Current UndoDynamax body
```c
void UndoDynamax(u32 battler)
{
    if ((gBattleTypeFlags & BATTLE_TYPE_RAID) && GetBattlerSide(battler) == B_SIDE_OPPONENT)
        return;                // boss guard — Phase 3

    // HP revert, SetActiveGimmick(NONE), timer clear, form revert...
}
```

### Where to add sprite restoration
Add at the **end** of `UndoDynamax`, after the existing body:

```c
    // Restore icon sprite for RAID allies when their Dynamax ends
    if ((gBattleTypeFlags & BATTLE_TYPE_RAID)
        && GetBattlerSide(battler) == B_SIDE_PLAYER
        && battler != B_POSITION_PLAYER_LEFT)  // not the player themselves
    {
        gSprites[gBattlerSpriteIds[battler]].invisible = TRUE;
        RestoreRaidAllyIconSprite(battler);  // creates new icon, stores in raid.allyIconSpriteId
    }
```

### Where to add sprite activation
Add to `ActivateDynamax(battler)`, after `SetActiveGimmick`:

```c
    // Swap icon for full front sprite when RAID ally Dynamaxes
    if ((gBattleTypeFlags & BATTLE_TYPE_RAID)
        && GetBattlerSide(battler) == B_SIDE_PLAYER
        && battler != B_POSITION_PLAYER_LEFT)
    {
        ActivateRaidAllyDynamaxSprite(battler);  // destroys icon, makes gBattlerSpriteIds visible
    }
```

### Alternative hook via RaidAllyHandleSwitchInAnim
`BattleScript_DynamaxBegins` calls `switchinanim` which triggers `RaidAllyHandleSwitchInAnim`. We could detect `GetActiveGimmick(battler) == GIMMICK_DYNAMAX` there and load the front sprite. However, intercepting in `ActivateDynamax` is cleaner (no dependency on the battle script path).

**Recommended:** Hook in `ActivateDynamax` and `UndoDynamax` directly.

**Confidence:** HIGH — all callers of `UndoDynamax` verified; `ActivateDynamax` source read.

---

## Q9: Rotation Counter Advance — Recommended Hook Point

**File:** `src/battle_main.c` — `BattleTurnPassed()` function

### Turn lifecycle in BattleTurnPassed() (lines 3939–4008)
```
3979: gBattleResults.battleTurnCounter++     ← existing turn counter
3981–3992: per-battler cleanup
4001: AssignUsableGimmicks()                 ← recalculates who can Dynamax
4004: gBattleMainFunc = HandleTurnActionSelectionState
```

### Recommended placement
Between line 3992 and line 4001 — after cleanup, before `AssignUsableGimmicks`:

```c
if (gBattleTypeFlags & BATTLE_TYPE_RAID)
    TryAdvanceRaidRotation();
```

`TryAdvanceRaidRotation()` in `src/raid_den.c`:
```c
static void TryAdvanceRaidRotation(void)
{
    static const u8 sRaidRotation[] = {0, 2, 3};
    u32 i;

    // Don't advance while any player-side battler is Dynamaxed
    for (i = 0; i < ARRAY_COUNT(sRaidRotation); i++)
    {
        if (GetActiveGimmick(sRaidRotation[i]) == GIMMICK_DYNAMAX)
            return;
    }

    // Advance to next slot
    switch (gBattleStruct->raid.dynamaxEnergy)
    {
        case 0: gBattleStruct->raid.dynamaxEnergy = 2; break;
        case 2: gBattleStruct->raid.dynamaxEnergy = 3; break;
        default:
        case 3: gBattleStruct->raid.dynamaxEnergy = 0; break;
    }
}
```

### Why this placement
`AssignUsableGimmicks()` is called immediately after, so the new `dynamaxEnergy` value is immediately consumed to populate `usableGimmick[]` for the new turn's action selection. The player sees the correct Dynamax option at the start of the new turn.

### Fainted battler handling
If the eligible battler has fainted, `GetActiveGimmick` returns `GIMMICK_NONE` for them (their HP is 0, but the gimmick flag may still read from `activeGimmick`). Regardless, a fainted battler cannot choose Dynamax action, so `CanDynamax` returning TRUE for them is harmless — they never act. The rotation advances correctly since they're not Dynamaxed.

To be safe, add a fainted check in `CanDynamax` for RAID:
```c
if (gBattleTypeFlags & BATTLE_TYPE_RAID && !IsBattlerAlive(battler))
    return FALSE;
```

**Confidence:** HIGH — `BattleTurnPassed` flow read directly.

---

## Architecture Patterns

### Files to modify for Phase 4
```
include/battle.h                  — add struct RaidData; add raid field to BattleStruct
src/battle_dynamax.c              — CanDynamax: skip HasTrainerUsedGimmick for RAID;
                                    activate raid.dynamaxEnergy check;
                                    add sprite swap in ActivateDynamax + UndoDynamax
src/battle_main.c                 — BattleTurnPassed: call TryAdvanceRaidRotation()
src/battle_controllers.c          — InitSinglePlayerBtlControllers: init raid fields
src/battle_controller_raid_ally.c — RaidAllyHandleLoadMonSprite: icon sprite approach
src/raid_den.c                    — TryAdvanceRaidRotation, sprite helper functions
```

### RaidAllyHandleLoadMonSprite override pattern

Replace the current `BtlController_HandleLoadMonSprite` call with custom logic:

```c
static void RaidAllyHandleLoadMonSprite(u32 battler)
{
    struct Pokemon *party = GetBattlerParty(battler);
    u16 species = GetMonData(&party[gBattlerPartyIndexes[battler]], MON_DATA_SPECIES);
    u32 personality = GetMonData(&party[gBattlerPartyIndexes[battler]], MON_DATA_PERSONALITY);
    u32 position = GetBattlerPosition(battler);

    // Load front sprite GFX into battler's slot (for Dynamax use)
    HandleLoadSpecialPokePic(TRUE, gMonSpritesGfxPtr->spritesGfx[position], species, personality);
    LoadMonFrontSpritePalette(&party[gBattlerPartyIndexes[battler]], battler);  // or BattleLoadMonSpriteGfx palette path
    SetMultiuseSpriteTemplateToPokemon(species, B_POSITION_OPPONENT_LEFT);
    gBattlerSpriteIds[battler] = CreateSprite(&gMultiuseSpriteTemplate,
                                               GetBattlerSpriteCoord(battler, BATTLER_COORD_X_2),
                                               GetBattlerSpriteDefault_Y(battler),
                                               GetBattlerSpriteSubpriority(battler));
    gSprites[gBattlerSpriteIds[battler]].oam.paletteNum = battler;
    gSprites[gBattlerSpriteIds[battler]].data[0] = battler;
    gSprites[gBattlerSpriteIds[battler]].data[2] = species;
    StartSpriteAnim(&gSprites[gBattlerSpriteIds[battler]], 0);
    gSprites[gBattlerSpriteIds[battler]].invisible = TRUE;  // hidden until Dynamax

    // Create 32x32 icon sprite for normal display
    LoadMonIconPalette(species);
    u8 iconX = (battler == 2) ? 32 : 56;
    u8 iconY = (battler == 2) ? 96 : 104;
    gBattleStruct->raid.allyIconSpriteId[battler - 2] =
        CreateMonIcon(species, SpriteCB_MonIcon, iconX, iconY, 0, personality);

    gBattlerControllerFuncs[battler] = WaitForMonAnimAfterLoad;
}
```

**Note:** `LoadMonFrontSpritePalette` needs to load the palette into `OBJ_PLTT_ID(battler)`. This can be done via the same palette path as `BattleLoadMonSpriteGfx` but calling `GetMonFrontSpritePal(mon)` and `LoadPalette` directly.

### Rotation field naming
Use `dynamaxEnergy` as specified by the existing commented stub — this is the authorial field name. The planner need not rename it.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead |
|---------|-------------|-------------|
| Eligibility gating | Custom "is it my turn" flag | Modify `CanDynamax`; let `AssignUsableGimmicks` propagate |
| Icon sprite | Custom 32×32 sprite system | `CreateMonIcon` / `FreeAndDestroyMonIconSprite` from `pokemon_icon.c` |
| Front sprite load | Custom decompressor call | `HandleLoadSpecialPokePic(TRUE, buffer, species, personality)` |
| Rotation logic | Bit manipulation | Simple `switch` on `dynamaxEnergy` through {0, 2, 3} |
| Gimmick trigger hide | Custom UI hide | Gates through `CanDynamax` → `usableGimmick = GIMMICK_NONE` → trigger sprite not created |

---

## Common Pitfalls

### Pitfall 1: SetGimmickAsActivated cross-contaminates raid partners
**What goes wrong:** `SetGimmickAsActivated(2, GIMMICK_DYNAMAX)` marks battler 3 as activated (`IsPartnerMonFromSameTrainer(2)` returns TRUE in RAID). On battler 3's rotation turn, `HasTrainerUsedGimmick(3)` returns TRUE → CanDynamax fails → battler 3 can never Dynamax.
**Fix:** Skip `HasTrainerUsedGimmick` for RAID player-side battlers in `CanDynamax`. The `activated` flag state is irrelevant; `dynamaxEnergy` is the gate.

### Pitfall 2: Rotation advances while ally is Dynamaxed
**What goes wrong:** If `TryAdvanceRaidRotation` doesn't check for active Dynamax, the rotation slot advances to battler 2 while battler 0 is still Dynamaxed, allowing two simultaneous Dynamax states.
**Fix:** Check `GetActiveGimmick(0/2/3) == GIMMICK_DYNAMAX` before advancing rotation. Only advance when no player-side battler is Dynamaxed.

### Pitfall 3: Icon sprite leaks on battle end or faint
**What goes wrong:** If `FreeAndDestroyMonIconSprite` isn't called when the battle ends (or when an ally faints), VRAM sprite slots leak.
**Fix:** Call `FreeAndDestroyMonIconSprite` in the ally faint animation path and in battle cleanup. Check `allyIconSpriteId[i] != MAX_SPRITES` before destroying.

### Pitfall 4: dynamaxEnergy initialized to 0 — but battler 0 should be eligible turn 1
**What goes wrong:** Could set `dynamaxEnergy = 0xFF` thinking "nobody eligible" initially.
**Fix:** Initialize `dynamaxEnergy = 0` so battler 0 (player) is eligible turn 1, matching the requirement.

### Pitfall 5: SetMultiuseSpriteTemplateToPokemon with OPPONENT_LEFT template — palette tag
**What goes wrong:** `SetMultiuseSpriteTemplateToPokemon(species, B_POSITION_OPPONENT_LEFT)` sets the palette tag to the species tag and applies front animation. But the palette is loaded into `OBJ_PLTT_ID(battler)` (battler 2 or 3), while the template's `paletteTag` is the species ID. These must be reconciled.
**Fix:** After `SetMultiuseSpriteTemplateToPokemon`, override `gMultiuseSpriteTemplate.paletteTag` with the battler-specific palette tag OR manually set `oam.paletteNum = battler` on the created sprite (as done in `BtlController_HandleLoadMonSprite`).

### Pitfall 6: Raid rotation field naming collides with Phase 3 boss setup
**What goes wrong:** `InitSinglePlayerBtlControllers` initializes `gBattleStruct->dynamax.dynamaxTurns[1] = 0xFF` for the boss. Adding `raid.dynamaxEnergy = 0` initialization in the same block is fine — but if the raid struct is cleared after the boss setup (e.g., by a `memset` of BattleStruct), the boss timer would also be zeroed.
**Fix:** Place `gBattleStruct->raid` initialization AFTER the boss Dynamax setup. `BattleStruct` is zeroed at battle start (`memset` in `InitBattleStruct`); the boss timer is set in the controller init which runs later. No ordering conflict.

---

## Code Examples

### CanDynamax modifications (src/battle_dynamax.c:74)
```c
bool32 CanDynamax(u32 battler)
{
    u16 species = gBattleMons[battler].species;
    u16 holdEffect = GetBattlerHoldEffect(battler, FALSE);

    if (gBattleTypeFlags & BATTLE_TYPE_FIRST_BATTLE && GetBattlerSide(battler) == B_SIDE_OPPONENT)
        return FALSE;

    if (!TESTING && (GetBattlerPosition(battler) == B_POSITION_PLAYER_LEFT
        || (!(gBattleTypeFlags & BATTLE_TYPE_MULTI) && GetBattlerPosition(battler) == B_POSITION_PLAYER_RIGHT)))
    {
        if (!CheckBagHasItem(ITEM_DYNAMAX_BAND, 1))
            return FALSE;
        if (B_FLAG_DYNAMAX_BATTLE == 0 || (B_FLAG_DYNAMAX_BATTLE != 0 && !FlagGet(B_FLAG_DYNAMAX_BATTLE)))
            return FALSE;
    }

    if (GET_BASE_SPECIES_ID(species) == SPECIES_ZACIAN
        || GET_BASE_SPECIES_ID(species) == SPECIES_ZAMAZENTA
        || GET_BASE_SPECIES_ID(species) == SPECIES_ETERNATUS)
        return FALSE;

    // In RAID, rotation governs eligibility — skip the "already used" / "should use" gates
    if (!(gBattleTypeFlags & BATTLE_TYPE_RAID && GetBattlerSide(battler) == B_SIDE_PLAYER))
    {
        if (HasTrainerUsedGimmick(battler, GIMMICK_DYNAMAX))
            return FALSE;
        if (!ShouldTrainerBattlerUseGimmick(battler, GIMMICK_DYNAMAX))
            return FALSE;
    }

    if (GetActiveGimmick(battler) != GIMMICK_NONE)
        return FALSE;

    if (!TESTING && (holdEffect == HOLD_EFFECT_Z_CRYSTAL || holdEffect == HOLD_EFFECT_MEGA_STONE))
        return FALSE;

    // Rotation gate: only the eligible battler may Dynamax each turn
    if (gBattleTypeFlags & BATTLE_TYPE_RAID && gBattleStruct->raid.dynamaxEnergy != battler)
        return FALSE;

    return TRUE;
}
```

### Rotation advance (src/raid_den.c)
```c
void TryAdvanceRaidRotation(void)
{
    static const u8 sRaidRotation[] = {0, 2, 3};
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sRaidRotation); i++)
    {
        if (GetActiveGimmick(sRaidRotation[i]) == GIMMICK_DYNAMAX)
            return;
    }

    switch (gBattleStruct->raid.dynamaxEnergy)
    {
        case 0:  gBattleStruct->raid.dynamaxEnergy = 2; break;
        case 2:  gBattleStruct->raid.dynamaxEnergy = 3; break;
        default:
        case 3:  gBattleStruct->raid.dynamaxEnergy = 0; break;
    }
}
```

### BattleTurnPassed hook (src/battle_main.c, in BattleTurnPassed)
```c
// Source: battle_main.c ~ line 3997, between cleanup and AssignUsableGimmicks
if (gBattleTypeFlags & BATTLE_TYPE_RAID)
    TryAdvanceRaidRotation();

AssignUsableGimmicks();  // existing line 4001
```

---

## Open Questions

1. **Exact icon sprite screen positions for battlers 2 and 3**
   - What we know: allies are player-side (lower screen); `GetBattlerSpriteCoord(battler, BATTLER_COORD_X_2/Y)` returns positions based on position 2/3 in the battler coords table
   - What's unclear: the exact pixel positions for the icon that look good with the 3v1 layout
   - Recommendation: Use `GetBattlerSpriteCoord(battler, BATTLER_COORD_X)` and `GetBattlerSpriteCoord(battler, BATTLER_COORD_Y)` as base, adjusted by a small offset to avoid overlapping the healthbox

2. **Palette loading for front sprite in RaidAllyHandleLoadMonSprite**
   - What we know: `BattleLoadMonSpriteGfx` loads palette via `GetMonFrontSpritePal(mon)` + `LoadPalette` into `OBJ_PLTT_ID(battler)`
   - What's unclear: Whether `BattleLoadMonSpriteGfx` can be called directly with a temp "force opponent side" hack or if a custom palette loader is needed
   - Recommendation: Call `BattleLoadMonSpriteGfx` as-is for palette (it handles shiny, illusion, etc.) and override only the `HandleLoadSpecialPokePic` call for the sprite pixels; or replicate the palette path inline

3. **WaitForMonAnimAfterLoad callback when gBattlerSpriteIds is invisible**
   - What we know: `WaitForMonAnimAfterLoad` waits for `gSprites[gBattlerSpriteIds[battler]].animEnded`. An invisible sprite still animates internally.
   - What's unclear: Whether an invisible sprite's `animEnded` flag behaves normally (it should — invisible only affects rendering)
   - Recommendation: Proceed with the invisible full-sprite approach; verify in testing

---

## Sources

### Primary (HIGH confidence — direct codebase reads)
- `src/battle_dynamax.c:74–121` — `CanDynamax` full body + commented stub at line 116
- `src/battle_dynamax.c:177–220` — `ActivateDynamax`, `UndoDynamax` full bodies
- `src/battle_gimmick.c:19–120` — `AssignUsableGimmicks`, `CanActivateGimmick`, `HasTrainerUsedGimmick`, `ShouldTrainerBattlerUseGimmick`, `SetGimmickAsActivated`
- `include/battle.h:572–588` — `struct DynamaxData`, `struct BattleGimmickData`
- `include/battle.h:648–840` — `struct BattleStruct` — confirmed no `raid` field
- `include/battle.h:1243–1248` — `GetBattlerSide` RAID guard (battler 3 → B_SIDE_PLAYER)
- `src/battle_controllers.c:121–154` — `InitSinglePlayerBtlControllers` RAID branch (as implemented)
- `src/battle_main.c:3939–4008` — `BattleTurnPassed` full function
- `src/battle_util.c:2854–2864` — `ENDTURN_DYNAMAX` case
- `src/pokemon_icon.c:137–184` — `CreateMonIcon`, `CreateMonIconNoPersonality`
- `src/battle_controller_raid_ally.c` — full file; `RaidAllyHandleLoadMonSprite`, `RaidAllyHandleChooseMove`
- `src/battle_gfx_sfx_util.c:611–660` — `BattleLoadMonSpriteGfx`
- `src/decompress.c:122–125` — `HandleLoadSpecialPokePic`
- `src/pokemon.c:2198–2222` — `SetMultiuseSpriteTemplateToPokemon`
- `src/battle_util.c:10885–10895` — `IsPartnerMonFromSameTrainer`
- `include/battle.h:1193–1202` — `IsBattlerAlive`
- `data/battle_scripts_1.s:9974–9991` — `BattleScript_DynamaxBegins`, `BattleScript_DynamaxEnds`
- `src/raid_den.c` — full file (Phase 3 implementation)
- `include/raid_den.h` — current struct definitions

---

## Metadata

**Confidence breakdown:**
- Struct placement (Q3): HIGH — struct read completely; no raid field found
- CanDynamax gating (Q1): HIGH — all checks verified; cross-contamination identified and fixed
- HasTrainerUsedGimmick bypass (Q1): HIGH — IsPartnerMonFromSameTrainer source confirmed
- Rotation hook placement (Q9): HIGH — BattleTurnPassed flow read completely
- Sprite loading for front sprite (Q7): HIGH — all three sprite functions read
- Icon sprite system (Q6): HIGH — pokemon_icon.c read
- Sprite swap hook points (Q8): HIGH — ActivateDynamax/UndoDynamax read completely
- Icon screen positions (Open Q1): LOW — exact pixel values require runtime testing

**Research date:** 2026-02-27
**Valid until:** 2026-03-29 (stable domain, 30-day window)
