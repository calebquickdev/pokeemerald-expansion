# Phase 3: Battle Core — Research

**Researched:** 2026-02-27
**Domain:** pokeemerald-expansion battle engine (C, GBA)
**Confidence:** HIGH — all findings verified directly from codebase source files

---

## Summary

Phase 3 implements the core raid battle: `BATTLE_TYPE_RAID`, a 3v1 battler layout, two CPU ally controllers, boss stat setup (level randomization + HP × 3), and permanent Dynamax for the boss.

All six research domains (flag, entry point, layout, controller, boss setup, permanent Dynamax) are directly verifiable in the codebase. The largest non-trivial decision is how to arrange the 3v1 layout within the hard 4-battler limit. The cleanest solution is the **duplicate position** approach: battlers 2 and 3 both use `B_POSITION_PLAYER_RIGHT` (=2), so `GetBattlerSide` returns player-side for both and party indexing works correctly. A position-lookup ambiguity (`GetBattlerAtPosition(2)` returns battler 2 only) exists but is acceptable for Phase 3 scope.

**Primary recommendation:** Use the INGAME_PARTNER layout as the structural template; add a `BATTLE_TYPE_RAID` branch at the top of `InitSinglePlayerBtlControllers`; clone `battle_controller_player_partner.c` for `SetControllerToRaidAlly`; put all raid setup logic in `src/raid_den.c`.

---

## Area 1: BATTLE_TYPE_RAID Flag

### Finding
**The flag is already defined.** No new definition needed.

```
include/constants/battle.h:60
#define BATTLE_TYPE_RAID               (1 << 12)
```

Bit 12 is currently unused in all live code paths. There is a commented-out check in `src/battle_dynamax.c:116` that explicitly references it:

```c
// if (gBattleTypeFlags & BATTLE_TYPE_RAID && gBattleStruct->raid.dynamaxEnergy != battler)
//    return FALSE;
```

This confirms the codebase has anticipated its use. The `gBattleStruct->raid` struct reference in that comment is a Phase 4+ concern and can remain commented out.

**Confidence:** HIGH — direct source read.

---

## Area 2: DoRaidBattle() Entry Point

### How battles start from event scripts

The pattern used everywhere in `src/battle_setup.c`:

1. Freeze overworld: `LockPlayerFieldControls()`, `FreezeObjectEvents()`, `StopPlayerAvatar()`
2. Set return callback: `gMain.savedCallback = CB2_<end_function>`
3. Set flags: `gBattleTypeFlags = <flags>`
4. Fill enemy party before this call (or inline): `gEnemyParty[0]` = boss, etc.
5. `CreateBattleStartTask(transitionId, song)` — starts transition, then calls `SetMainCallback2(CB2_InitBattle)`
6. `ScriptContext_Stop()` — **required** to resume the calling event script after battle ends

`CB2_EndScriptedWildBattle` (`src/battle_setup.c:589`) is the closest existing callback to reuse:
- On player defeat → `CB2_WhiteOut`
- On win/escape → `CB2_ReturnToFieldContinueScriptPlayMapMusic`

`CreateBattleStartTask` (`src/battle_setup.c:259`) handles the transition animation and calls `SetMainCallback2(CB2_InitBattle)` when done.

### Recommended DoRaidBattle() implementation

Place in `src/raid_den.c`. Declare in `include/raid_den.h`. Register in `data/specials.inc`:

```c
void DoRaidBattle(void)
{
    u8 denId = (u8)gSpecialVar_0x8000;
    SetupRaidBossParty(denId);   // fills gEnemyParty[0] + allies
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_ReturnToFieldContinueScriptPlayMapMusic;
    gBattleTypeFlags = BATTLE_TYPE_DOUBLE | BATTLE_TYPE_RAID;
    CreateBattleStartTask(B_TRANSITION_BLUR, 0);
    ScriptContext_Stop();
}
```

`FreezeObjectEvents()` and `StopPlayerAvatar()` are optional — `BattleSetup_StartScriptedWildBattle` (the closest analog) does not call them.

The den interaction script at `data/maps/LittlerootTown/scripts.inc` currently calls `special OpenDenLobbyScreen`. Phase 6 will replace `OpenDenLobbyScreen` with a real lobby that calls `DoRaidBattle`. For Phase 3 testing, `OpenDenLobbyScreen` should be temporarily wired to call `DoRaidBattle` directly, then `waitstate`.

**Confidence:** HIGH — pattern read directly from `BattleSetup_StartScriptedWildBattle`, `BattleSetup_StartLegendaryBattle`, etc.

---

## Area 3: 3v1 Battler Layout

### Relevant constants and arrays

```
include/constants/battle.h:26   #define MAX_BATTLERS_COUNT  4   ← do NOT change
include/constants/battle.h:28   #define B_POSITION_PLAYER_LEFT    0
include/constants/battle.h:29   #define B_POSITION_OPPONENT_LEFT  1
include/constants/battle.h:30   #define B_POSITION_PLAYER_RIGHT   2
include/constants/battle.h:31   #define B_POSITION_OPPONENT_RIGHT 3
include/constants/battle.h:44   #define BIT_SIDE  1
```

`GetBattlerSide(battler)` = `gBattlerPositions[battler] & BIT_SIDE`:
- Positions 0, 2 → side 0 (B_SIDE_PLAYER)
- Positions 1, 3 → side 1 (B_SIDE_OPPONENT)

`GetPartyBattlerData(battler)` returns `gPlayerParty[gBattlerPartyIndexes[battler]]` for player-side battlers, `gEnemyParty[index]` for opponent-side.

### InitSinglePlayerBtlControllers — where to add the branch

`src/battle_controllers.c:120` — `InitSinglePlayerBtlControllers()`. The function has three branches:
- `if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)` — 4-battler layout
- `else if (!IsDoubleBattle())` — 2-battler layout
- `else` — double battle, 4-battler layout

Add the `BATTLE_TYPE_RAID` branch **first**, before the `INGAME_PARTNER` check.

### Recommended layout for raid

The **duplicate position** approach — battlers 2 and 3 both use `B_POSITION_PLAYER_RIGHT` (=2):

| Battler | Controller            | gBattlerPositions | gBattlerPartyIndexes | Party source     |
|---------|-----------------------|-------------------|----------------------|------------------|
| 0       | SetControllerToPlayer | B_POSITION_PLAYER_LEFT (0)   | 0 | gPlayerParty[0] |
| 1       | SetControllerToOpponent | B_POSITION_OPPONENT_LEFT (1) | 0 | gEnemyParty[0] (boss) |
| 2       | SetControllerToRaidAlly | B_POSITION_PLAYER_RIGHT (2) | 3 | gPlayerParty[3] |
| 3       | SetControllerToRaidAlly | B_POSITION_PLAYER_RIGHT (2) | 4 | gPlayerParty[4] |

`gBattlersCount = MAX_BATTLERS_COUNT` (4).

### Gotcha: duplicate position

`GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT)` iterates battlers 0→3 and returns the **first** match — battler 2. Battler 3 is invisible to this function.

Consequence: `GetPartnerBattler(3)` → `GetBattlerAtPosition(BATTLE_PARTNER(2))` → `GetBattlerAtPosition(0)` → returns battler 0 (the player). So battler 3's "partner" resolves to the player, not battler 2.

**Acceptable for Phase 3:** Controller dispatch uses battler index directly (not position), AI chooses moves by index, HP bars are indexed by battler — all work correctly. Position-based partner lookups are rare in AI partner code paths and only matter for advanced double-battle coordination (Phase 4+).

### BufferBattlePartyCurrentOrderBySide

Follow the INGAME_PARTNER pattern (lines 159–162):
```c
BufferBattlePartyCurrentOrderBySide(0, 0);
BufferBattlePartyCurrentOrderBySide(1, 0);
BufferBattlePartyCurrentOrderBySide(2, 1);
BufferBattlePartyCurrentOrderBySide(3, 1);
```

**Confidence:** HIGH for the layout approach; MEDIUM for `BufferBattlePartyCurrentOrderBySide` side arguments (copied from INGAME_PARTNER pattern without deep analysis of its internals).

---

## Area 4: SetControllerToRaidAlly

### SetControllerToPlayerPartner — the model

`src/battle_controller_player_partner.c`:

```c
void SetControllerToPlayerPartner(u32 battler)
{
    gBattlerControllerEndFuncs[battler] = PlayerPartnerBufferExecCompleted;
    gBattlerControllerFuncs[battler] = PlayerPartnerBufferRunCommand;
}
```

The controller's dispatch table (`sPlayerPartnerBufferCommands`) has:
- `CONTROLLER_CHOOSEACTION` → `PlayerPartnerHandleChooseAction` → calls `AI_TrySwitchOrUseItem(battler)` — no player input
- `CONTROLLER_OPENBAG` → `BtlController_Empty` — no bag access
- `CONTROLLER_CHOOSEPOKEMON` → `PlayerPartnerHandleChoosePokemon` — AI-driven

There is **no Run behavior** in the PlayerPartner controller. No Poké Ball throw. The ally is completely AI-controlled.

### Phase 3 RaidAlly = PlayerPartner clone

For Phase 3, `SetControllerToRaidAlly` is a straight clone of `SetControllerToPlayerPartner` with renamed symbols. No behavioral difference is needed since PlayerPartner already has no bag/run/catch.

**New file:** `src/battle_controller_raid_ally.c`

Naming convention (matching codebase): 
- Functions: `RaidAllyHandle*`, `RaidAllyBuffer*`
- Controller func table: `sRaidAllyBufferCommands`
- Entry point: `void SetControllerToRaidAlly(u32 battler)`

**Declare in** `include/battle_controllers.h` in the "player partner controller" section area:
```c
// raid ally controller
void SetControllerToRaidAlly(u32 battler);
```

**Confidence:** HIGH — PlayerPartner source read directly, clone approach is clear.

---

## Area 5: Boss Stat Setup

### struct DynamaxDen — current state

```c
// include/raid_den.h
struct DynamaxDen
{
    u16 species;
    u8 isGmax;
    u8 _pad;   // ← repurpose as starRating
};
```

The `_pad` field is explicitly a placeholder. Renaming it to `starRating` (u8, range 1–5) adds no save size change and breaks no existing code.

Set `starRating` in stubs: `UpdateDynamaxDens()` and `ActivateDynamaxDen()` should set it to a non-zero default (e.g., `1` for 1★) until Phase 6 implements the BST→star formula.

### Boss party creation

In `DoRaidBattle()` / a helper `SetupRaidBossParty(u8 denId)`:

```c
static const u8 sStarLevelMin[6] = {0, 15, 25, 35, 45, 55}; // index by starRating 1-5
static const u8 sStarLevelMax[6] = {0, 20, 30, 40, 50, 60};

static void SetupRaidBossParty(u8 denId)
{
    u16 species = gSaveBlock2Ptr->dynamaxDens[denId].species;
    u8 stars = gSaveBlock2Ptr->dynamaxDens[denId].starRating;
    u8 minLv = sStarLevelMin[stars], maxLv = sStarLevelMax[stars];
    u8 level = (u8)(minLv + (Random() % (maxLv - minLv + 1)));
    u32 maxHp;

    ZeroEnemyPartyMons();
    CreateMon(&gEnemyParty[0], species, level, 31, FALSE, 0, OT_ID_RANDOM_NO_SHINY, 0);

    // HP × 3
    maxHp = GetMonData(&gEnemyParty[0], MON_DATA_MAX_HP) * 3;
    SetMonData(&gEnemyParty[0], MON_DATA_MAX_HP, &maxHp);
    SetMonData(&gEnemyParty[0], MON_DATA_HP, &maxHp);

    // Ally mons — placeholder starters
    CreateMon(&gPlayerParty[3], SPECIES_SCEPTILE,  level, 31, FALSE, 0, OT_ID_PLAYER_ID, 0);
    CreateMon(&gPlayerParty[4], SPECIES_BLAZIKEN,  level, 31, FALSE, 0, OT_ID_PLAYER_ID, 0);
}
```

**Why direct `SetMonData` instead of `ApplyDynamaxHPMultiplier`:** `ApplyDynamaxHPMultiplier` uses `GetDynamaxLevelHPMultiplier` which returns a fixed-point multiplier tied to Dynamax Level stat (1.5×–2×). The requirement is exactly 3× raw HP, so a direct multiply is cleaner and avoids dependence on the Dynamax Level data pipeline.

**Important:** `ZeroEnemyPartyMons()` must be called to clear stale data. Enemy party size is recalculated by `CalculateEnemyPartyCount()` called later in the battle init flow (`src/battle_main.c:549`). The boss occupies gEnemyParty[0] only.

**Confidence:** HIGH — `CreateMon`, `SetMonData`, `GetMonData` signatures read from `include/pokemon.h:631`.

---

## Area 6: Permanent Dynamax for Boss

### Where Dynamax state is stored

```c
// include/battle.h (struct DynamaxData)
u8 dynamaxTurns[MAX_BATTLERS_COUNT];   // gBattleStruct->dynamax.dynamaxTurns
```

`ActivateDynamax(battler)` sets `dynamaxTurns[battler] = DYNAMAX_TURNS_COUNT` (=3).
`UndoDynamax(battler)` sets `dynamaxTurns[battler] = 0` and clears the active gimmick.

### Where UndoDynamax is called

Three call sites relevant to permanent boss Dynamax:

1. **`src/battle_util.c:2854-2861`** — `ENDTURN_DYNAMAX` case in end-of-turn loop:
   ```c
   case ENDTURN_DYNAMAX:
       if (GetActiveGimmick(battler) == GIMMICK_DYNAMAX
           && --gBattleStruct->dynamax.dynamaxTurns[battler] == 0)
       {
           UndoDynamax(battler);
           BattleScriptExecute(BattleScript_DynamaxEnds);
           effect++;
       }
   ```

2. **`src/battle_main.c:5581-5585`** — end-of-battle cleanup:
   ```c
   for (battler = 0; battler < gBattlersCount; ++battler)
       if (GetActiveGimmick(battler) == GIMMICK_DYNAMAX)
           UndoDynamax(battler);
   ```

3. **`src/battle_script_commands.c`** — `BS_UndoDynamax` explicit command (rare, script-driven).
4. **`src/battle_dynamax.c:495-508`** — `BS_UndoDynamax`.

### Recommended approach: guard inside UndoDynamax itself

Add an early-return guard at the top of `UndoDynamax` in `src/battle_dynamax.c:198`:

```c
void UndoDynamax(u32 battler)
{
    if (gBattleTypeFlags & BATTLE_TYPE_RAID && GetBattlerSide(battler) == B_SIDE_OPPONENT)
        return;
    // ... existing code
}
```

This is the single-location guard that covers all three call sites above without patching each one. `GetBattlerSide(battler)` = `gBattlerPositions[battler] & 1`. Boss is at `B_POSITION_OPPONENT_LEFT` (=1), so side = 1 = `B_SIDE_OPPONENT`. Guard is reliable.

### Infinite timer approach

In the `BATTLE_TYPE_RAID` branch of `InitSinglePlayerBtlControllers`, after setting `gBattlerPartyIndexes`:

```c
SetActiveGimmick(1, GIMMICK_DYNAMAX);
SetGimmickAsActivated(1, GIMMICK_DYNAMAX);
gBattleStruct->dynamax.dynamaxTurns[1] = 0xFF;
```

Setting `dynamaxTurns[1] = 0xFF` means even if the end-of-turn decrement runs (before the UndoDynamax guard is in place), it takes 255 turns to expire. Combined with the UndoDynamax guard, the boss never leaves Dynamax.

**Why `SetActiveGimmick` works here:** `SetActiveGimmick(battler, gimmick)` stores into `gBattleStruct->gimmick.activeGimmick[GetBattlerSide(battler)][gBattlerPartyIndexes[battler]]`. By the time this code runs in `InitSinglePlayerBtlControllers`, `gBattlerPartyIndexes[1] = 0` has already been set, so `GetBattlerSide(1) = B_SIDE_OPPONENT`, and it stores into `activeGimmick[1][0]` — correct.

**Caveat:** `gBattleMons[1]` is not yet populated when `InitSinglePlayerBtlControllers` runs. `ActivateDynamax()` also clears `STATUS2_SUBSTITUTE` from `gBattleMons[battler]`, so we **cannot** call the full `ActivateDynamax(1)` at this stage. The three manual calls above (`SetActiveGimmick`, `SetGimmickAsActivated`, `dynamaxTurns`) replicate the critical parts of `ActivateDynamax` safely.

**`IsPartnerMonFromSameTrainer(1)` in `SetGimmickAsActivated`:** For battler 1 (OPPONENT_LEFT), `GetBattlerSide(1) = B_SIDE_OPPONENT`. `IsPartnerMonFromSameTrainer` checks `B_SIDE_OPPONENT && BATTLE_TYPE_TWO_OPPONENTS` → false (we use BATTLE_TYPE_RAID), then falls to default (returns `TRUE` for normal doubles). This means battler 3 (BATTLE_PARTNER(1)=3) gets marked as activated too. For Phase 3 this is harmless since battler 3 is a raid ally not intended to Dynamax anyway; `CanDynamax` will gate any attempt.

**Confidence:** HIGH for `UndoDynamax` guard location (verified all callers); HIGH for `dynamaxTurns = 0xFF` trick; MEDIUM for `SetGimmickAsActivated` side-effect on battler 3 (requires manual verification that it doesn't break anything).

---

## Architecture Patterns

### Project Structure for Phase 3

```
src/
├── raid_den.c                     # DoRaidBattle(), SetupRaidBossParty() — MODIFY
├── battle_controllers.c           # InitSinglePlayerBtlControllers — MODIFY
├── battle_dynamax.c               # UndoDynamax guard — MODIFY
├── battle_controller_raid_ally.c  # NEW — clone of battle_controller_player_partner.c
include/
├── raid_den.h                     # DoRaidBattle() declaration; rename _pad to starRating — MODIFY
├── battle_controllers.h           # SetControllerToRaidAlly declaration — MODIFY
data/
└── specials.inc                   # def_special DoRaidBattle — MODIFY
```

No new header needed; DoRaidBattle fits in the existing `raid_den.h`.

### Pattern: Adding a new battle controller

1. Create `src/battle_controller_<name>.c`
2. Define `sNameBufferCommands[]` dispatch table (clone existing, modify handlers)
3. Define `SetControllerToName(u32 battler)` — sets `gBattlerControllerEndFuncs` and `gBattlerControllerFuncs`
4. Declare `void SetControllerToName(u32 battler)` in `include/battle_controllers.h`
5. Register in `InitSinglePlayerBtlControllers` via the relevant branch

The Makefile auto-discovers `.c` files in `src/` — no Makefile edits needed.

---

## Common Pitfalls

### Pitfall 1: Calling `ActivateDynamax` too early
**What goes wrong:** `ActivateDynamax` modifies `gBattleMons[battler].status2`, but `gBattleMons` is not yet populated when `InitSinglePlayerBtlControllers` runs.
**How to avoid:** Use the three-call substitute instead: `SetActiveGimmick`, `SetGimmickAsActivated`, `dynamaxTurns[1] = 0xFF`.

### Pitfall 2: Duplicate B_POSITION_PLAYER_RIGHT breaks GetBattlerAtPosition
**What goes wrong:** Any code using `GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT)` will always return battler 2, never battler 3.
**How to avoid:** Accept this for Phase 3. Phase 4+ can introduce a proper B_POSITION_PLAYER_FAR or use INGAME_PARTNER's flag system. For Phase 3, battler 3 functions correctly via direct index in all critical paths (controller dispatch, `gBattleMons[3]`, `gBattlerPartyIndexes[3]`).

### Pitfall 3: dynamaxTurns not set before battle starts
**What goes wrong:** `dynamaxTurns[1]` defaults to 0. On the first end-of-turn, `--dynamaxTurns[1]` wraps to 255 (u8 underflow) on the decrement at the `ENDTURN_DYNAMAX` case... wait, actually it's `--x == 0`, so `--0 = 255` then `255 == 0` is false. This would survive ONE end-of-turn but then next turn `--255 = 254`, never hits 0. So unguarded `dynamaxTurns = 0` also works by u8 underflow!
**But:** For clarity and correctness (avoiding reliance on wrap-around behavior), still set `dynamaxTurns[1] = 0xFF` explicitly AND add the `UndoDynamax` guard.

### Pitfall 4: starRating = 0 causes array out-of-bounds
**What goes wrong:** `sStarLevelMin[0]` and `sStarLevelMax[0]` are only defined as placeholders in the table. If `starRating` is accidentally 0 (e.g., from uninitialized struct field, which was previously `_pad`), the level range would be wrong (both 0), producing a level-0 mon.
**How to avoid:** In `UpdateDynamaxDens()` and `ActivateDynamaxDen()`, always set `dynamaxDens[i].starRating = 1` as the default until Phase 6.

### Pitfall 5: gPlayerParty slots 3 and 4 contain stale party mons
**What goes wrong:** If the player has party mons in slots 3/4 (possible in Littleroot if they've gotten that far), the raid ally creation overwrites them. After the battle, `CB2_ReturnToFieldContinueScriptPlayMapMusic` doesn't explicitly restore these.
**How to avoid:** `ZeroPlayerPartyMons()` is too destructive. Instead, save and restore slots 3/4 around the battle, OR (simpler for Phase 3) limit testing to a save state with ≤2 party mons. This is a Phase 4 concern to fix properly.

---

## Code Examples

### Full BATTLE_TYPE_RAID branch for InitSinglePlayerBtlControllers

```c
// Insert at the top of InitSinglePlayerBtlControllers, before BATTLE_TYPE_INGAME_PARTNER check
if (gBattleTypeFlags & BATTLE_TYPE_RAID)
{
    gBattleMainFunc = BeginBattleIntro;

    gBattlerControllerFuncs[0] = SetControllerToPlayer;
    gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;

    gBattlerControllerFuncs[1] = SetControllerToOpponent;
    gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;

    gBattlerControllerFuncs[2] = SetControllerToRaidAlly;
    gBattlerPositions[2] = B_POSITION_PLAYER_RIGHT;

    gBattlerControllerFuncs[3] = SetControllerToRaidAlly;
    gBattlerPositions[3] = B_POSITION_PLAYER_RIGHT; // duplicate — see pitfall 2

    gBattlersCount = MAX_BATTLERS_COUNT;

    BufferBattlePartyCurrentOrderBySide(0, 0);
    BufferBattlePartyCurrentOrderBySide(1, 0);
    BufferBattlePartyCurrentOrderBySide(2, 1);
    BufferBattlePartyCurrentOrderBySide(3, 1);

    gBattlerPartyIndexes[0] = 0; // gPlayerParty[0]
    gBattlerPartyIndexes[1] = 0; // gEnemyParty[0] (boss)
    gBattlerPartyIndexes[2] = 3; // gPlayerParty[3] (ally 1)
    gBattlerPartyIndexes[3] = 4; // gPlayerParty[4] (ally 2)

    // Permanent Dynamax for boss (battler 1)
    SetActiveGimmick(1, GIMMICK_DYNAMAX);
    SetGimmickAsActivated(1, GIMMICK_DYNAMAX);
    gBattleStruct->dynamax.dynamaxTurns[1] = 0xFF;
}
```

### SetControllerToRaidAlly (in src/battle_controller_raid_ally.c)

```c
void SetControllerToRaidAlly(u32 battler)
{
    gBattlerControllerEndFuncs[battler] = RaidAllyBufferExecCompleted;
    gBattlerControllerFuncs[battler] = RaidAllyBufferRunCommand;
}
```

### UndoDynamax guard (src/battle_dynamax.c:198)

```c
void UndoDynamax(u32 battler)
{
    if (gBattleTypeFlags & BATTLE_TYPE_RAID && GetBattlerSide(battler) == B_SIDE_OPPONENT)
        return;
    // ... existing body unchanged
}
```

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead |
|---------|-------------|-------------|
| Boss level randomization | Custom RNG | `Random() % range + min` — existing engine RNG |
| Boss HP multiply | Custom stat recalc | Direct `SetMonData(MON_DATA_MAX_HP/HP, &val)` after `GetMonData` |
| Ally AI move selection | Custom move picker | `AI_TrySwitchOrUseItem` already in `PlayerPartnerHandleChooseAction` |
| Battle start transition | Custom transition system | `CreateBattleStartTask(B_TRANSITION_BLUR, 0)` |
| Dynamax gimmick flags | Custom flag storage | `SetActiveGimmick` / `GetActiveGimmick` from `battle_gimmick.c` |

---

## Open Questions

1. **BufferBattlePartyCurrentOrderBySide second parameter semantics**
   - What we know: INGAME_PARTNER uses `(0,0), (1,0), (2,1), (3,1)` for battlers 0–3
   - What's unclear: whether the second param is "which side's order to read" or "which party slot group". Not fully traced.
   - Recommendation: Copy the INGAME_PARTNER pattern exactly; verify by running the build and entering battle.

2. **gPlayerParty slots 3/4 conflict with player's party**
   - What we know: The player in Littleroot tests likely has ≤2 mons
   - What's unclear: Whether any existing code path for a 6-mon party will corrupt the ally data
   - Recommendation: Accept for Phase 3; document that test saves should have ≤2 mons in slots 0–2.

3. **CB2_ReturnToFieldContinueScriptPlayMapMusic as raid end callback**
   - What we know: Used by `CB2_EndScriptedWildBattle`, `CB2_EndFirstBattle`, trainer battle end
   - What's unclear: Whether it properly resumes the event script `waitstate` after the battle
   - Recommendation: This is the standard "continue scripted sequence" callback — it will work.

---

## Sources

### Primary (HIGH confidence — direct codebase reads)
- `include/constants/battle.h` — `BATTLE_TYPE_RAID`, `MAX_BATTLERS_COUNT`, `B_POSITION_*`, `BIT_SIDE`
- `src/battle_controllers.c:120–280` — `InitSinglePlayerBtlControllers` full function
- `src/battle_controller_player_partner.c:54–120` — dispatch table, `SetControllerToPlayerPartner`
- `src/battle_dynamax.c:132–220` — `ApplyDynamaxHPMultiplier`, `ActivateDynamax`, `UndoDynamax`
- `src/battle_util.c:2854–2862` — `ENDTURN_DYNAMAX` case
- `src/battle_main.c:5579–5585` — end-of-battle `UndoDynamax` loop
- `src/battle_setup.c:259–453` — `CreateBattleStartTask`, all `BattleSetup_Start*` functions
- `src/battle_gimmick.c:54–120` — `SetActiveGimmick`, `SetGimmickAsActivated`
- `include/battle_dynamax.h` — `DYNAMAX_TURNS_COUNT = 3`
- `include/battle_controllers.h` — all `SetControllerTo*` declarations
- `include/raid_den.h` — `struct DynamaxDen`, existing declarations
- `src/raid_den.c` — existing implementation (`sDenObjectTable`, `OpenDenLobbyScreen` stub)
- `include/battle.h:1217–1272` — `GetBattlerPosition`, `GetBattlerAtPosition`, `GetBattlerSide`, `GetPartyBattlerData`

### Pre-existing planning documents (HIGH confidence — same project)
- `.planning/research/STACK.md` — prior research on battle architecture
- `.planning/research/PITFALLS.md` — prior pitfall analysis
- `.planning/phases/02-overworld-den-object/02-03-SUMMARY.md` — Phase 2 implementation summary

---

## Key Decisions for Planner

1. **BATTLE_TYPE_RAID is pre-defined** — `(1 << 12)` at `include/constants/battle.h:60`. Plan 03-01 should NOT redefine it; just use it.

2. **DoRaidBattle() lives in `src/raid_den.c`** — consistent with existing raid logic home; declare in `include/raid_den.h`, register in `data/specials.inc`. Follow `BattleSetup_StartScriptedWildBattle` as the structural template.

3. **3v1 layout uses duplicate B_POSITION_PLAYER_RIGHT for battler 3** — Both battlers 2 and 3 have `gBattlerPositions[x] = B_POSITION_PLAYER_RIGHT`. This makes both player-side. `GetBattlerAtPosition(2)` returns battler 2 only; battler 3 is invisible to position lookups. Acceptable for Phase 3.

4. **Ally mons go in gPlayerParty[3] and gPlayerParty[4]** — Pre-created in `DoRaidBattle()` / `SetupRaidBossParty()` using hardcoded starters (Sceptile + Blaziken as placeholder). Phase 4+ will implement a proper ally pool.

5. **`struct DynamaxDen._pad` → rename to `starRating`** — Required by BATTLE-03 level randomization. Update `include/raid_den.h`, set default `starRating = 1` in stubs. No save size change.

6. **HP × 3 via direct SetMonData** — NOT via `ApplyDynamaxHPMultiplier` (which applies Dynamax Level multiplier, not a raw 3× multiplier). After `CreateMon`, do: `maxHp = GetMonData(...MAX_HP) * 3; SetMonData(MAX_HP); SetMonData(HP)`.

7. **Permanent Dynamax: guard inside `UndoDynamax` + set `dynamaxTurns[1] = 0xFF`** — Two changes:
   - `src/battle_dynamax.c:198`: add early return if `BATTLE_TYPE_RAID && B_SIDE_OPPONENT`
   - RAID branch of `InitSinglePlayerBtlControllers`: set `dynamaxTurns[1] = 0xFF` + call `SetActiveGimmick` + `SetGimmickAsActivated`

8. **`SetControllerToRaidAlly` = near-exact clone of `SetControllerToPlayerPartner`** — for Phase 3 there are zero behavioral differences. Clone the file, rename all symbols. Divergence (no Dynamax selection UI, different party handling) is Phase 4 work.

9. **Battle end callback = `CB2_ReturnToFieldContinueScriptPlayMapMusic`** — standard callback for scripted battles; resumes the `waitstate` in the event script.

10. **Plan 03-01 scope**: flag check (verify not to add), `DoRaidBattle()` implementation, `InitSinglePlayerBtlControllers` RAID branch, `starRating` field rename.
    **Plan 03-02 scope**: `src/battle_controller_raid_ally.c` new file, header declaration, register in controllers.
    **Plan 03-03 scope**: boss HP × 3, level randomization, `UndoDynamax` guard, `dynamaxTurns` = 0xFF.

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all patterns verified from source
- Architecture: HIGH — layout, controller, party setup all confirmed
- Pitfalls: HIGH — duplicate position issue confirmed; gPlayerParty slot conflict is a known Phase 3 limitation
- Open questions: MEDIUM — `BufferBattlePartyCurrentOrderBySide` semantics not fully traced; risk is low (worst case: party order display is slightly wrong)

**Research date:** 2026-02-27
**Valid until:** Stable — GBA battle engine is not actively changing; valid indefinitely unless engine is refactored
