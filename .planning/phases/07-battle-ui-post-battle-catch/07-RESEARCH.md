# Phase 7: Battle UI & Post-Battle Catch — Research

**Researched:** 2026-02-28
**Domain:** GBA battle engine — custom HUD, action-menu interception, post-faint catch sequence, Gigantamax delivery
**Confidence:** HIGH (all findings verified against live source)

---

## Summary

Phase 7 has three distinct sub-problems that each touch a different part of the engine:

**Ally HUD (UI-01/02):** The existing healthbox system creates one sprite pair per battler and positions each pair via `GetBattlerHealthboxCoords`. The 3v1 layout requires stacking all three ally healthboxes on the right side of the screen. The engine only has two player-side sprite templates (PLAYER1_TILE, PLAYER2_TILE); a third template is needed for battler 3. Battler 1 (boss, B_POSITION_OPPONENT_LEFT) already gets the standard opponent healthbox at the correct left-side position. Color thresholds (green/yellow/red) and HP numbers are handled automatically by `MoveBattleBarGraphically` and `UpdateHpTextInHealthbox` once the coords and templates are correct.

**Run confirmation (UI-03):** There is already a `BATTLE_TYPE_RAID` guard in `battle_main.c:4418` that intercepts `B_ACTION_RUN` and currently shows "BattleScript_PrintCantRunFromTrainer". Phase 7 replaces that path with a YesNo confirmation script. If confirmed, `TryRunFromBattle` / `B_OUTCOME_RAN` proceeds normally; the post-battle overworld script handles den-stay-active logic.

**Post-faint catch (CATCH-01/02/03):** When all opponents faint, `gBattleOutcome = B_OUTCOME_WON` triggers `HandleEndTurn_BattleWon`. Phase 7 adds a RAID branch there that routes to a new `BattleScript_RaidBossDefeated`. That script checks for no balls / Nuzlocke block (skip straight to "boss flees" path), then runs a new `Cmd_raidBallSelect` C command that draws a mini ball-list window, sets `gLastUsedItem` on confirm, and falls through to the existing `handleballthrow` machinery. Adding a RAID branch in `Cmd_handleballthrow` forces `BALL_3_SHAKES_SUCCESS` and routes to `BattleScript_SuccessBallThrow`. For GMAX bosses, `gEnemyParty[boss]` must have its species rewritten to `GET_BASE_SPECIES_ID(gmaxSpecies)` and `MON_DATA_GIGANTAMAX_FACTOR` set to 1 before `Cmd_givecaughtmon` executes.

**Primary recommendation:** Keep as much of the existing catch plumbing intact as possible. Intercept at three narrow points: `GetBattlerHealthboxCoords`, `HandleEndTurn_BattleWon`, and `Cmd_handleballthrow`.

---

## Standard Stack

These are the existing engine APIs all tasks must use — verified against source.

### Healthbox / HP-bar APIs (`src/battle_interface.c`, `include/battle_interface.h`)

| Symbol | Signature | Purpose |
|--------|-----------|---------|
| `CreateBattlerHealthboxSprites` | `(u8 battlerId) → u8 spriteId` | Allocates healthbox sprites; stores in `gHealthboxSpriteIds[battlerId]` |
| `InitBattlerHealthboxCoords` | `(u8 battlerId)` | Reads `GetBattlerHealthboxCoords` and moves the sprite |
| `GetBattlerHealthboxCoords` | `(u8 battlerId, s16 *x, s16 *y)` | Lookup table for healthbox pixel positions (override for RAID) |
| `SetBattleBarStruct` | `(u8 battler, u8 healthboxId, s32 maxVal, s32 currVal, s32 receivedVal)` | Seeds the HP-bar animation state |
| `MoveBattleBar` | `(u8 battler, u8 healthboxId, u8 whichBar, u8 unused) → s32` | Advances bar animation one step; auto-colors green/yellow/red |
| `UpdateHpTextInHealthbox` | `(u32 healthboxId, u32 which, s16 currHp, s16 maxHp)` | Renders "NNN/NNN" numbers in the healthbox OBJ tiles |
| `UpdateHealthboxAttribute` | `(u8 healthboxId, struct Pokemon *mon, u8 elementId)` | Updates a single attribute (nick, level, HP, status icon…) |
| `SetHealthboxSpriteInvisible` / `Visible` | `(u8 healthboxId)` | Show/hide the healthbox without destroying it |
| `GetScaledHPFraction` | `(s16 hp, s16 maxhp, u8 scale) → u8` | Proportional fill for a custom bar |
| `GetHPBarLevel` | `(s16 hp, s16 maxhp) → u8` | Returns `HP_BAR_GREEN/YELLOW/RED/EMPTY/FULL` |

#### Healthbox coordinate lookup (current values)

```c
// GetBattlerHealthboxCoords — doubles branch (WhichBattleCoords != 0):
B_POSITION_PLAYER_LEFT   → (159,  76)
B_POSITION_PLAYER_RIGHT  → (171, 101)
B_POSITION_OPPONENT_LEFT → ( 44,  19)
B_POSITION_OPPONENT_RIGHT→ ( 32,  44)
```

For RAID the three ally healthboxes must appear on the right side, stacked vertically. Add a
`BATTLE_TYPE_RAID` guard at the top of `GetBattlerHealthboxCoords`:

```c
if (gBattleTypeFlags & BATTLE_TYPE_RAID)
{
    switch (GetBattlerPosition(battler))
    {
    case B_POSITION_PLAYER_LEFT:   *x = 194; *y =  62; return; // player (top)
    case B_POSITION_PLAYER_RIGHT:  *x = 194; *y =  81; return; // ally 1 (mid)
    case B_POSITION_OPPONENT_RIGHT:*x = 194; *y = 100; return; // ally 2 (bot)
    case B_POSITION_OPPONENT_LEFT: *x =  44; *y =  19; return; // boss  (unchanged)
    }
}
```

*(Exact pixel values are Claude's discretion per CONTEXT.md — adjust during integration testing.)*

#### Third player-side healthbox tile tag

`sHealthboxPlayerSpriteTemplates[2]` does not exist. Battlers 2 and 3 both resolve to
`GetBattlerPosition() / 2 == 1` → same tag `TAG_HEALTHBOX_PLAYER2_TILE` → VRAM conflict.

Add to `include/battle_interface.h`:
```c
#define TAG_HEALTHBOX_PLAYER3_TILE  0xD703   // gap after OPPONENT2_TILE
```

Add a third entry to `sHealthboxPlayerSpriteTemplates[]` in `battle_interface.c` mirroring
the existing entries. Then gate battler 3 (OPPONENT_RIGHT, player side) to use template index 2:

```c
// In CreateBattlerHealthboxSprites, doubles-branch, player-side:
u8 templateIdx = GetBattlerPosition(battlerId) / 2;
if ((gBattleTypeFlags & BATTLE_TYPE_RAID) && GetBattlerPosition(battlerId) == B_POSITION_OPPONENT_RIGHT)
    templateIdx = 2;   // 3rd ally → 3rd template
healthboxLeftSpriteId = CreateSprite(&sHealthboxPlayerSpriteTemplates[templateIdx], ...);
```

### Run / flee APIs (`src/battle_util.c`, `src/battle_main.c`)

| Symbol | Location | Role |
|--------|----------|------|
| `HandleAction_Run` | `battle_util.c:603` | Called via `[B_ACTION_RUN]` dispatch table |
| `TryRunFromBattle` | `battle_util.c:514` | Speed-check; sets `B_OUTCOME_RAN` on success |
| `HandleEndTurn_RanFromBattle` | `battle_main.c:5494` | Picks escape script; routes to `HandleEndTurn_FinishBattle` |
| `BattleScript_GotAwaySafely` | `data/battle_scripts_2.s` | Message + `finishturn` for normal escape |

**Current RAID intercept** — `battle_main.c:4418`:
```c
else if ((gBattleTypeFlags & BATTLE_TYPE_RAID)
         && gBattleResources->bufferB[battler][1] == B_ACTION_RUN)
{
    BattleScriptExecute(BattleScript_PrintCantRunFromTrainer);   // ← REPLACE THIS
    gBattleCommunication[battler] = STATE_BEFORE_ACTION_CHOSEN;
}
```

Replace `BattleScript_PrintCantRunFromTrainer` with a new `BattleScript_RaidConfirmRun`.
That script shows "Abandon the raid?" YesNo; Yes → `B_OUTCOME_RAN` + escape message; No →
`STATE_BEFORE_ACTION_CHOSEN` to re-show the menu.

After battle: `gSpecialVar_Result` == `B_OUTCOME_RAN`; the overworld script leaves the den
active and returns the player to their last position (den-active logic lives in the script,
not the battle C code).

### Ball-throw / catch APIs (`src/battle_script_commands.c`, `data/battle_scripts_2.s`)

| Symbol | Location | Role |
|--------|----------|------|
| `Cmd_handleballthrow` | `battle_script_commands.c:15752` | Calculates catch rate, emits ball-throw anim, sets next script |
| `BtlController_EmitBallThrowAnim` | `battle_controllers.c:2658` | Sends `BALL_3_SHAKES_SUCCESS` (or other case) to controller |
| `BALL_3_SHAKES_SUCCESS` | `include/battle_controllers.h:85` | Forces success anim (3 shakes + lock) |
| `BattleScript_SuccessBallThrow` | `data/battle_scripts_2.s:179` | Full catch flow: exp, dex, nickname, `givecaughtmon` |
| `Cmd_givecaughtmon` | `battle_script_commands.c:16082` | Delivers `gEnemyParty[boss]` to player party/PC |
| `GiveMonToPlayer` | `src/pokemon.c` | Adds mon to party or PC |
| `HasAtLeastOnePokeBall` | `src/item.c` (declared `include/item.h`) | Returns TRUE if BALLS_POCKET is non-empty |
| `gBagPockets[BALLS_POCKET]` | EWRAM | Array of `{itemId, quantity}` for pokéballs |

#### RAID branch in `Cmd_handleballthrow` (after existing checks)

```c
else if (gBattleTypeFlags & BATTLE_TYPE_RAID)
{
    gBallToDisplay = gLastThrownBall = gLastUsedItem;
    BtlController_EmitBallThrowAnim(gBattlerAttacker, BUFFER_A, BALL_3_SHAKES_SUCCESS);
    MarkBattlerForControllerExec(gBattlerAttacker);
    gBattlescriptCurrInstr = BattleScript_SuccessBallThrow;
}
```

The `gBattlerTarget` must be set to the boss battler before this is reached (use
`gBattlerTarget = GetCatchingBattler()` — already called at line 15762 before the
TRAINER/WALLY/RAID branches).

### Gigantamax APIs (`src/pokemon.c`, `include/pokemon.h`, `include/data.h`)

| Symbol | Type | Purpose |
|--------|------|---------|
| `gSpeciesInfo[species].isGigantamax` | `u32:1` | TRUE if this species *is* a Gigantamax form |
| `GET_BASE_SPECIES_ID(species)` | macro → `GetFormSpeciesId(species, 0)` | Returns form-0 (base) from any form |
| `MON_DATA_GIGANTAMAX_FACTOR` | enum | GetMonData / SetMonData key for the 1-bit flag |
| `gigantamaxFactor:1` | `BoxPokemon.substruct3` bit | Stored in IV/misc word; persists to Pokémon HOME |

**GMAX delivery pattern** (call before `Cmd_givecaughtmon` executes):

```c
static void RewriteGmaxBossForCatch(void)
{
    u32 bossIdx = gBattlerPartyIndexes[GetCatchingBattler()];
    u16 species  = GetMonData(&gEnemyParty[bossIdx], MON_DATA_SPECIES, NULL);
    if (gSpeciesInfo[species].isGigantamax)
    {
        u16 baseSpecies = (u16)GET_BASE_SPECIES_ID(species);
        u8  gmaxFlag    = TRUE;
        SetMonData(&gEnemyParty[bossIdx], MON_DATA_SPECIES, &baseSpecies);
        SetMonData(&gEnemyParty[bossIdx], MON_DATA_GIGANTAMAX_FACTOR, &gmaxFlag);
        CalculateMonStats(&gEnemyParty[bossIdx]);   // recalc stats for base form
    }
}
```

Call this from a new battle-script command `Cmd_raidPrepGmaxCatch` (or inline it in the
success path via a hook) that runs immediately before `givecaughtmon` when
`BATTLE_TYPE_RAID`.

### Post-faint outcome hook (`src/battle_main.c:5400`)

```c
static void HandleEndTurn_BattleWon(void)
{
    gCurrentActionFuncId = 0;
    if (gBattleTypeFlags & BATTLE_TYPE_RAID)          // ← ADD AT THE TOP
    {
        BattleStopLowHpSound();
        PlayBGM(MUS_VICTORY_TRAINER);
        gBattlescriptCurrInstr = BattleScript_RaidBossDefeated;
        gBattleMainFunc = HandleEndTurn_FinishBattle;
        return;
    }
    if (gBattleTypeFlags & (BATTLE_TYPE_LINK ...))    // existing branches follow
```

`BattleScript_RaidBossDefeated` orchestrates:
1. Print "You defeated the raid boss!" (new string ID after 734)
2. Check nuzlocke + no-balls edge cases → `BattleScript_RaidNoCatch` path
3. Call `Cmd_raidBallSelect` (new C command that draws the ball list window)
4. On selection: set `gLastUsedItem`, call `handleballthrow`
5. On `SuccessBallThrow`: call `Cmd_raidPrepGmaxCatch` then `givecaughtmon`
6. Print GMAX message if applicable
7. `setbyte gBattleOutcome, B_OUTCOME_CAUGHT` + `finishturn`

### Den-flag APIs (`include/raid_den.h`, `include/constants/flags.h`)

| Symbol | Usage |
|--------|-------|
| `FLAG_DAILY_DEN_RAIDED(denId)` | macro expanding to a DAILY_FLAGS_START offset |
| `FlagSet(FLAG_DAILY_DEN_RAIDED(denId))` | marks den consumed |

The den ID must be accessible post-battle. **Recommended storage**: add `u8 denId` to
`struct RaidData` (already in `include/battle.h:590`) and populate it in `DoRaidBattle`:

```c
gBattleStruct->raid.denId = (u8)gSpecialVar_0x8000;
```

Then the overworld script (which resumes via `gMain.savedCallback`) can set the flag:
```
setvar VAR_TEMP_1, (outcome)
compare VAR_TEMP_1, B_OUTCOME_CAUGHT
call_if_eq RaidMarkDenInactive
```
Or call `FlagSet(FLAG_DAILY_DEN_RAIDED(gBattleStruct->raid.denId))` from C inside the
battle's `HandleEndTurn_FinishBattle` for RAID when outcome is `B_OUTCOME_CAUGHT` or `B_OUTCOME_WON`.

---

## Architecture Patterns

### Pattern 1 — Narrowly-scoped BATTLE_TYPE_RAID guards

All existing RAID hooks follow the pattern:
```c
if (gBattleTypeFlags & BATTLE_TYPE_RAID)
{
    // raid-specific path
    return; // or continue
}
// existing non-raid path unchanged
```
Insert new guards at the narrowest possible point; never restructure surrounding logic.

### Pattern 2 — Battle scripts as orchestrators

Multi-step sequences (catch flow, confirm dialog) live in `.s` battle scripts and call C
`Cmd_*` functions only for computations. State is threaded through
`gBattleCommunication[MULTIUSE_STATE]` (a u8 array indexed by `MULTIUSE_STATE` or
`MULTISTRING_CHOOSER`).

### Pattern 3 — `HandleBattleWindow` for transient pop-ups

Small bordered boxes (like the YesNo box at `YESNOBOX_X_Y`) are drawn by:
```c
HandleBattleWindow(xStart, yStart, xEnd, yEnd, 0);
// ... put text on the window ...
HandleBattleWindow(xStart, yStart, xEnd, yEnd, WINDOW_CLEAR); // to dismiss
```
The ball-selection list should use the same approach rather than creating a new full
`WindowTemplate`.

### Pattern 4 — Fainted-icon dimming

The ally icon sprite is stored in `gBattleStruct->raid.allyIconSpriteId[]`. To show a
fainted ally as greyed/dimmed, modify the sprite's palette slot to a desaturated copy:
```c
// In the respawn-timer tick (TryRaidAllyRespawn) or faint hook:
gSprites[iconId].oam.paletteNum = FAINTED_ALLY_PALETTE_SLOT;
```
Load the greyscale palette into a dedicated slot during battle init. The HP bar for a
fainted ally should be set to 0 via `UpdateHpTextInHealthbox(spriteId, HP_BOTH, 0, maxHp)`.

### Anti-Patterns to Avoid

- **Calling `GiveMonToPlayer` directly** — always go through `Cmd_givecaughtmon`; it
  handles the party-full → PC path and sets `gBattleResults`.
- **Zeroing `gEnemyParty[boss]` before `givecaughtmon`** — the boss data lives there even
  after its `gBattleMons[].hp` reaches 0; do not clear it during the faint script.
- **Using `BATTLE_TYPE_TRAINER` flag for RAID** — `Cmd_handleballthrow` checks TRAINER
  _first_ and blocks the catch; the RAID branch must come before the `else {}` block, not
  try to suppress the TRAINER flag.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead |
|---------|-------------|-------------|
| HP bar color thresholds | Custom threshold logic | `GetHPBarLevel` (>50% green, >20% yellow, ≤20% red) |
| HP bar animation | Custom pixel writer | `SetBattleBarStruct` + `MoveBattleBar` (one call per frame until returns -1) |
| Ball inventory check | Bag iteration | `HasAtLeastOnePokeBall()` for the empty-bag guard |
| Ball list enumeration | Custom structure | `gBagPockets[BALLS_POCKET].itemSlots[]` + `capacity` |
| Nuzlocke-block check | Custom flag check | `gNuzlockeCatchBlocked` (already set by engine before any throw) |
| GMAX→base species lookup | Form-table search | `GET_BASE_SPECIES_ID(species)` = `GetFormSpeciesId(species, 0)` |
| Mon delivery | Custom party insertion | `Cmd_givecaughtmon` → `GiveMonToPlayer` handles party-full, PC, name display |
| Catch outcome | Custom end-of-battle | `setbyte gBattleOutcome, B_OUTCOME_CAUGHT` + `finishturn` in the battle script |
| Greyed palette | Per-pixel darkening | Duplicate the icon palette into a new slot with all RGB channels halved |

---

## Common Pitfalls

### Pitfall 1 — Third healthbox tile tag VRAM conflict
**What goes wrong:** Battler 3 (B_POSITION_OPPONENT_RIGHT, player side) calls
`CreateBattlerHealthboxSprites` which computes template index `GetBattlerPosition(3)/2 = 1`
— same as battler 2. Both load tiles at `TAG_HEALTHBOX_PLAYER2_TILE`. The second call
overwrites the first, corrupting healthbox graphics.
**How to avoid:** Add `TAG_HEALTHBOX_PLAYER3_TILE` (value `0xD703`) and a 3rd
`SpriteTemplate` entry. Gate battler 3 to this template inside `CreateBattlerHealthboxSprites`
when `BATTLE_TYPE_RAID`.

### Pitfall 2 — `Cmd_handleballthrow` TRAINER check runs before RAID check
**What goes wrong:** `BATTLE_TYPE_RAID` battles have `gBattleTypeFlags & BATTLE_TYPE_TRAINER`
 as FALSE, so the trainer block won't fire. BUT if a future flag combination adds TRAINER,
it would eat the catch. More importantly, the existing code at line 15772 tests TRAINER
then WALLY then falls to the normal catch calculation — the RAID branch must be inserted
before the `else { ... }` normal calculation block, i.e. as a 3rd `else if`.
**How to avoid:** Insert the RAID branch between the WALLY check and the `else {` opening.

### Pitfall 3 — Boss species already zeroed when GMAX rewrite runs
**What goes wrong:** If `gBattleMons[boss].species` is zeroed during the faint sequence
before the catch script, `gSpeciesInfo[SPECIES_NONE].isGigantamax` will return FALSE and
the GMAX rewrite is silently skipped.
**How to avoid:** Read species from `gEnemyParty[gBattlerPartyIndexes[boss]].species` (the
party-mon copy), NOT `gBattleMons[boss].species`. The party copy is never zeroed by the
faint sequence.

### Pitfall 4 — `gBattleOutcome` set before ball selection completes
**What goes wrong:** `BattleScript_RaidBossDefeated` is reached from
`HandleEndTurn_BattleWon`, which normally sets `gBattleOutcome = B_OUTCOME_WON` before
`HandleEndTurn_FinishBattle` runs. If `gBattleOutcome` is already `B_OUTCOME_WON` when the
catch script resolves, `FreeResetData_ReturnToOvOrDoEvolutions` might branch on the wrong
outcome.
**How to avoid:** At the start of the RAID branch in `HandleEndTurn_BattleWon`, do NOT set
the outcome yet. Let `BattleScript_RaidBossDefeated` set it to either `B_OUTCOME_CAUGHT` or
`B_OUTCOME_WON` (boss-fled path) at the very end.

### Pitfall 5 — `WhichBattleCoords` returns 0 for battler 3
**What goes wrong:** `WhichBattleCoords(3)` checks `GetBattlerPosition(3) == B_POSITION_PLAYER_LEFT
&& gPlayerPartyCount == 1`. Battler 3 has B_POSITION_OPPONENT_RIGHT (3 ≠ 0) so this
returns `IsDoubleBattle()`, which is TRUE for RAID. This is correct, but if that check
ever changes, healthbox creation for battler 3 could fall into the singles branch and use
the wrong template.
**How to avoid:** Test `WhichBattleCoords` return value for all 4 battlers during
integration and add an assert comment.

### Pitfall 6 — Confirmation prompt state machine re-entry
**What goes wrong:** The YesNo confirmation for Run is implemented as a
`gSelectionBattleScripts` script with multi-state C handling. If the player presses B to
cancel, `gBattleCommunication[battler]` must return to `STATE_BEFORE_ACTION_CHOSEN`, not
`STATE_SELECTION_SCRIPT` — otherwise the menu is not re-shown.
**How to avoid:** Mirror the existing pattern at lines 4411–4417 (the forfeit-match
confirm) exactly, substituting the raid-specific script.

---

## Code Examples

### Example 1 — Seeding and advancing the HP bar for a new battler

```c
// src/battle_interface.c — called after CreateBattlerHealthboxSprites:
SetBattleBarStruct(battler, gHealthboxSpriteIds[battler],
                  gBattleMons[battler].maxHP,
                  gBattleMons[battler].hp, 0);
// Each frame in the bar-update task:
s32 result = MoveBattleBar(battler, gHealthboxSpriteIds[battler], HEALTH_BAR, 0);
// result == -1 when animation is complete
```

### Example 2 — Creating a YesNo prompt from a battle script command

```c
// Pattern from Cmd_givecaughtmon (lines 16102–16107):
case STATE_SHOW_YESNO:
    HandleBattleWindow(YESNOBOX_X_Y, 0);
    BattlePutTextOnWindow(gText_BattleYesNoChoice, B_WIN_YESNO);
    gBattleCommunication[MULTIUSE_STATE] = STATE_WAIT_INPUT;
    gBattleCommunication[CURSOR_POSITION] = 0;  // default Yes
    BattleCreateYesNoCursorAt(0);
    break;
case STATE_WAIT_INPUT:
    if (JOY_NEW(DPAD_UP)   && gBattleCommunication[CURSOR_POSITION] != 0) { ... }
    if (JOY_NEW(DPAD_DOWN) && gBattleCommunication[CURSOR_POSITION] == 0) { ... }
    if (JOY_NEW(A_BUTTON)) {
        if (gBattleCommunication[CURSOR_POSITION] == 0)
            gBattlescriptCurrInstr = BattleScript_RaidRunConfirmed;
        else
            gBattlescriptCurrInstr = BattleScript_RaidRunCancelled;
    }
    break;
```

### Example 3 — Ball list window (new Cmd_raidBallSelect pattern)

```c
// Pseudocode for the new C command:
static void Cmd_raidBallSelect(void)
{
    // state 0: draw window listing balls from gBagPockets[BALLS_POCKET]
    // state 1: wait for A/B input, move cursor up/down
    // state 2: on A, set gLastUsedItem = selected ball,
    //          gBattlescriptCurrInstr = BattleScript_BallThrow (re-uses existing path)
    // state 3: on B (cancel) — not applicable; prompt is mandatory after boss defeat
}
```

The window is drawn with `HandleBattleWindow(x0, y0, x1, y1, 0)` and each ball name
printed with `AddTextPrinterParameterized4` into a dedicated `B_WIN_*` slot. A cursor arrow
marks the selected entry; A confirms, B is disabled (catch is forced).

### Example 4 — Full GMAX delivery call

```c
// Before givecaughtmon in BattleScript_RaidBossDefeated:
static void Cmd_raidPrepGmaxCatch(void)
{
    CMD_ARGS();
    u32 bossIdx = gBattlerPartyIndexes[GetCatchingBattler()];
    u16 species  = GetMonData(&gEnemyParty[bossIdx], MON_DATA_SPECIES, NULL);
    if (gSpeciesInfo[species].isGigantamax)
    {
        u16 base = (u16)GET_BASE_SPECIES_ID(species);
        u8  flag = TRUE;
        SetMonData(&gEnemyParty[bossIdx], MON_DATA_SPECIES,           &base);
        SetMonData(&gEnemyParty[bossIdx], MON_DATA_GIGANTAMAX_FACTOR, &flag);
        CalculateMonStats(&gEnemyParty[bossIdx]);
        gBattleStruct->raid.isGmax = TRUE; // flag for GMAX message later
    }
    gBattlescriptCurrInstr = cmd->nextInstr;
}
```

---

## Open Questions

1. **Exact ally HUD pixel positions**
   - What we know: screen is 240×160; doubles healthboxes use (171,101) for PLAYER_RIGHT
   - What's unclear: exact pixel fit for 3 stacked 64×32-px healthboxes without overlap
   - Recommendation: Start with y-offsets of 19px each (one healthbox height); tune visually during integration

2. **Ball list window dimensions**
   - What we know: `HandleBattleWindow` works in tile units (8px each); action menu is at col 18–29, row 7–19
   - What's unclear: optimal placement that doesn't obscure the boss or ally sprites
   - Recommendation: Use the right half of the message box area (approx x=15–29, y=10–18); place it where the action menu normally appears

3. **Den-flag timing (in-battle vs post-battle script)**
   - What we know: `gBattleStruct->raid.denId` can hold the den ID; `FlagSet` is safe from anywhere
   - What's unclear: whether the post-battle script has reliable access to `gBattleStruct` after `FreeResetData_ReturnToOvOrDoEvolutions` clears it
   - Recommendation: Call `FlagSet(FLAG_DAILY_DEN_RAIDED(gBattleStruct->raid.denId))` inside `HandleEndTurn_FinishBattle` for RAID battles (before `FreeResetData` runs), not in the overworld script

4. **GMAX "has Gigantamax Factor!" message string ID**
   - Prior phases defined string IDs through 734 (BATTLESTRINGS_COUNT=734)
   - New message needs the next IDs; confirm count before assigning

---

## Sources

### Primary (HIGH confidence — directly read from source)

- `src/battle_interface.c:651–738` — `CreateBattlerHealthboxSprites` implementation
- `src/battle_interface.c:876–913` — `GetBattlerHealthboxCoords`, `InitBattlerHealthboxCoords`
- `src/battle_interface.c:2060–2130` — `MoveBattleBar`, `MoveBattleBarGraphically`, color thresholds
- `src/battle_controller_player.c:246–365` — `HandleInputChooseAction` (cursor→action map)
- `src/battle_message.c:1424` — `gText_BattleMenu` = "Battle{56}Bag\nPokémon{56}Run"
- `src/battle_main.c:4418–4423` — existing RAID `B_ACTION_RUN` intercept
- `src/battle_main.c:5400–5456` — `HandleEndTurn_BattleWon` (hook insertion point)
- `src/battle_main.c:5494–5526` — `HandleEndTurn_RanFromBattle`
- `src/battle_util.c:514–656` — `TryRunFromBattle`, `HandleAction_Run`
- `src/battle_util.c:3020–3134` — `HandleFaintedMonActions` (boss faint path)
- `src/battle_script_commands.c:15752–15800` — `Cmd_handleballthrow` (WALLY/TRAINER branches)
- `src/battle_script_commands.c:16082–16223` — `Cmd_givecaughtmon`
- `data/battle_scripts_2.s:165–212` — `BattleScript_BallThrow`, `BattleScript_SuccessBallThrow`, `BattleScript_WallyBallThrow`
- `include/battle.h:590–597` — `struct RaidData` (denId field not yet present; must add)
- `include/pokemon.h:192` — `gigantamaxFactor:1` bit location
- `include/pokemon.h:443` — `isGigantamax:1` in `SpeciesInfo`
- `src/pokemon.c:4452–4462` — `GetGMaxTargetSpecies`
- `include/pokemon.h:11` — `GET_BASE_SPECIES_ID` macro
- `include/constants/item.h:7,13` — `POCKET_POKE_BALLS`, `BALLS_POCKET`
- `include/item.h` — `HasAtLeastOnePokeBall()` declaration
- `include/raid_den.h:11` — `FLAG_DAILY_DEN_RAIDED(denId)` macro
- `src/raid_den.c:241–249` — `DoRaidBattle` (denId source = `gSpecialVar_0x8000`)
- `include/battle_interface.h` — full public API confirmed

---

## Metadata

**Confidence breakdown:**
- HP bar / healthbox system: HIGH — source read directly; clear API boundaries
- Run confirmation: HIGH — intercept point already exists (battle_main.c:4418)
- Ball-throw / catch pipeline: HIGH — Wally tutorial proves the pattern; RAID branch is additive
- GMAX delivery: HIGH — `GET_BASE_SPECIES_ID` + `MON_DATA_GIGANTAMAX_FACTOR` confirmed in source
- Ball selection mini-UI: MEDIUM — no existing ball-list widget; `HandleBattleWindow` pattern is clear but exact layout needs integration tuning
- Den-flag timing: MEDIUM — safe C path exists; script-side timing needs verification

**Research date:** 2026-02-28
**Valid until:** Stable — this codebase changes slowly; re-verify only if battle_interface.c or battle_main.c are heavily refactored
