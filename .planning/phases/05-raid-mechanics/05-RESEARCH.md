# Phase 5 Research: Raid Mechanics

## Summary

Phase 5 implements four interlocking systems: (1) shield damage intercept, (2) HP-threshold shield activation, (3) ally respawn timers, and (4) 10-turn storm counter with boss flee. All four systems fit cleanly into existing extension points in the battle engine. The biggest risk is the `respawnTimer` off-by-one in the plan and the "all-3-fainted" edge case that none of the 4 plans explicitly addresses.

---

## 1. Damage Pipeline and Shield Intercept Hook (Plan 01)

### The pipeline order

The standard attack script in `data/battle_scripts_1.s` executes these commands in sequence:

```
attackcanceler → accuracycheck → attackstring → ppreduce → critcalc →
damagecalc → typecalc → adjustdamage → attackanimation → waitanimation →
healthbarupdate → datahpupdate → critmessage → resultmessage → moveend
```

### Where each command lands in C

| Script command | C function | What it does |
|---|---|---|
| `damagecalc` | `Cmd_damagecalc` (line ~2015) | Writes calculated damage into `gBattleStruct->moveDamage[battlerDef]` |
| `adjustdamage` | `Cmd_adjustdamage` (line 2078) | Clamps for Endure/Focus Sash/Ice Face; can zero out `moveDamage` |
| `healthbarupdate` | `Cmd_healthbarupdate` (line 2502) | Sends HP bar animation to controller; reads `moveDamage` |
| `datahpupdate` | `Cmd_datahpupdate` (line 2545) | Actually subtracts `moveDamage` from `gBattleMons[battler].hp` |

### Best hook point for shield intercept: `Cmd_adjustdamage`

`Cmd_adjustdamage` iterates every `battlerDef` and can set `moveDamage[battlerDef] = 0` before the HP bar animates and before actual HP is deducted. Ice Face uses this exact pattern (lines 2110–2118):

```c
// src/battle_script_commands.c ~2110
if (GetBattlerAbility(battlerDef) == ABILITY_ICE_FACE && ...)
{
    gBattleStruct->moveResultFlags[battlerDef] &= ~(MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE);
    gBattleStruct->moveDamage[battlerDef] = 0;
    RecordAbilityBattle(gBattlerTarget, ABILITY_ICE_FACE);
    gDisableStructs[battlerDef].iceFaceActivationPrevention = TRUE;
    continue;
}
```

For the raid shield, add a similar block **before** the Ice Face check in `Cmd_adjustdamage`:

```c
if ((gBattleTypeFlags & BATTLE_TYPE_RAID)
 && battlerDef == 1                          // boss is always battler index 1
 && gBattleStruct->raid.shieldHp > 0)
{
    u8 consume = IsMaxMove(gCurrentMove) ? 2 : 1;
    gBattleStruct->raid.shieldHp = (gBattleStruct->raid.shieldHp > consume)
                                 ? gBattleStruct->raid.shieldHp - consume : 0;
    gBattleStruct->moveDamage[battlerDef] = 0;
    // Clear effectiveness flags so the result message reads neutral
    gBattleStruct->moveResultFlags[battlerDef] &=
        ~(MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE);
    continue;
}
```

`IsMaxMove()` lives in `src/battle_dynamax.c` (line 489). It returns `TRUE` for any move ID in the `[FIRST_MAX_MOVE, LAST_MAX_MOVE]` range.

### When `moveDamage = 0` is safe

`Cmd_datahpupdate` line 2619:
```c
if (gBattleMons[battler].hp > gBattleStruct->moveDamage[battler])
    gBattleMons[battler].hp -= gBattleStruct->moveDamage[battler]; // subtracts 0 → no change
```
The HP bar animation in `Cmd_healthbarupdate` passes `moveDamage` to `BtlController_EmitHealthBarUpdate`; a value of 0 simply shows no bar drop.

**Do NOT set `MOVE_RESULT_NO_EFFECT`** — that flag suppresses the attack animation and ability-interaction checks entirely. Zeroing `moveDamage` while leaving the flags intact is the correct minimal intervention.

### Pitfalls for Plan 01

- **Multi-hit moves**: each hit calls the damage pipeline independently. Each hit correctly goes through `Cmd_adjustdamage`, so each hit will consume 1 shield unit. This is the desired behavior (a 5-hit move could drain 5 shields). Ensure the shield is not over-consumed (use the clamped decrement above).
- **Spread moves** (`IsDoubleSpreadMove()`): in doubles spreads, `Cmd_adjustdamage` loops over all targets. The guard `battlerDef == 1` ensures only the boss slot is intercepted.
- **HITMARKER_PASSIVE_DAMAGE**: passive damage (poison, burn, weather) does not go through `Cmd_adjustdamage` at all. No risk of accidental shield consumption from passive damage.
- **`gBattlerTarget` vs `battlerDef`**: inside the spread-move loop, `battlerDef` is the iteration variable, not necessarily `gBattlerTarget`. Always use `battlerDef` here.

---

## 2. Shield Activation at 75%/50% HP Thresholds (Plan 02)

### Where HP is updated

The only place that actually subtracts damage from `gBattleMons[battler].hp` during an attack is `Cmd_datahpupdate`, lines 2619–2627:

```c
if (gBattleMons[battler].hp > gBattleStruct->moveDamage[battler])
    gBattleMons[battler].hp -= gBattleStruct->moveDamage[battler];
else {
    gBattleStruct->moveDamage[battler] = gBattleMons[battler].hp;
    gBattleMons[battler].hp = 0;
}
```

### Recommended hook: end of `Cmd_datahpupdate`

After the block above, add a threshold check. Since this runs mid-script (before `resultmessage`), triggering a new battle script here requires `BattleScriptPush` + returning, similar to how Substitute fade is handled (lines 2568–2573):

```c
// After HP subtraction, check raid shield thresholds
if ((gBattleTypeFlags & BATTLE_TYPE_RAID)
 && battler == 1    // boss
 && !(gBattleStruct->moveResultFlags[battler] & MOVE_RESULT_NO_EFFECT))
{
    u32 hp    = gBattleMons[1].hp;
    u32 maxHp = gBattleMons[1].maxHP;
    // First shield set: HP just dropped below 75%
    if (gBattleStruct->raid.shieldPhase == 0 && hp * 4 < maxHp * 3)
    {
        gBattleStruct->raid.shieldHp    = 1 + gRaidBattleData.starRating;
        gBattleStruct->raid.shieldPhase = 1;
        // Also check 50% in same hit
        if (hp * 2 < maxHp)
        {
            gBattleStruct->raid.shieldHp    = 1 + gRaidBattleData.starRating; // overwrite/grant second set
            gBattleStruct->raid.shieldPhase = 2;
        }
    }
    // Second shield set: HP just dropped below 50%
    else if (gBattleStruct->raid.shieldPhase == 1 && hp * 2 < maxHp)
    {
        gBattleStruct->raid.shieldHp    = 1 + gRaidBattleData.starRating;
        gBattleStruct->raid.shieldPhase = 2;
    }
}
```

### Required struct addition: `shieldPhase`

`struct RaidData` currently has `shieldHp` but no field tracking how many threshold crossings have occurred. Add to `include/battle.h`:

```c
struct RaidData
{
    u8 dynamaxEnergy;
    u8 allyIconSpriteId[2];
    u8 shieldHp;
    u8 respawnTimer[3];
    u8 shieldPhase;   // 0 = neither threshold crossed, 1 = 75% granted, 2 = 50% granted
};
```

Initialize `shieldPhase = 0` alongside the existing zero-initialization in Phase 4 Plan 01.

### `gRaidBattleData` access

`gRaidBattleData` is declared in `src/raid_den.c` (a file-scope global) and accessible from `battle_script_commands.c` if declared `extern` in `include/raid_den.h`. Confirm the extern declaration exists or add:
```c
extern struct RaidBattleData gRaidBattleData;
```

### Pitfalls for Plan 02

- **Single-hit kill through 75% into below 50%**: must grant BOTH shield sets in the same call (handled by the nested check above).
- **Triggering mid-spread move**: the loop guard `battler == 1` ensures only the boss slot triggers this.
- **Passive damage (poison, etc.) also calls `Cmd_datahpupdate`**: the `HITMARKER_PASSIVE_DAMAGE` flag is set during passive damage. Add a check: `!(gHitMarker & HITMARKER_PASSIVE_DAMAGE)` to avoid activating shields from end-of-turn poison.
- **`shieldHp` is non-zero when threshold is crossed again**: if shield from 75% was already depleted and boss is now hit below 50%, `shieldPhase == 1` and `shieldHp == 0`. The grant correctly sets `shieldHp = 1 + starRating` fresh.

---

## 3. Ally Faint / Respawn Cycle (Plan 03)

### Where faint is detected

`HandleFaintedMonActions()` in `src/battle_util.c`, starting at line 3013. Case 4 (lines 3076–3094) iterates all battlers, finds those with `hp == 0` and not yet absent, and calls `BattleScript_HandleFaintedMon` which prompts a switch.

### Intercept point for raid allies

Before the `BattleScriptExecute(BattleScript_HandleFaintedMon)` call (line 3089), add:

```c
if ((gBattleTypeFlags & BATTLE_TYPE_RAID)
 && GetBattlerSide(gBattleStruct->faintedActionsBattlerId) == B_SIDE_PLAYER)
{
    u32 allyIdx = gBattleStruct->faintedActionsBattlerId; // 0, 2, or 3
    gBattleStruct->raid.respawnTimer[allyIdx] = 2; // skip 1 turn, respawn on the next
    gAbsentBattlerFlags |= 1u << allyIdx;           // keep absent (no switch prompt)
    gBattleStruct->faintedActionsState = 5;         // advance state as if switch happened
    return TRUE;
}
```

The battler remains absent (`gAbsentBattlerFlags` bit set); the normal switch prompt is bypassed.

### Turn-start respawn in `HandleEndTurn_ContinueBattle`

`HandleEndTurn_ContinueBattle()` in `src/battle_main.c` (lines ~4001–4007) is called at the start of every new turn, BEFORE action selection. The existing `TryAdvanceRaidRotation()` call is here. Add a companion call:

```c
if (gBattleTypeFlags & BATTLE_TYPE_RAID)
{
    TryAdvanceRaidRotation();
    TryRaidAllyRespawn();   // New function in src/raid_den.c
}
```

`TryRaidAllyRespawn()` iterates ally battler indices (0, 2, 3):

```c
void TryRaidAllyRespawn(void)
{
    static const u8 sAllyBattlers[] = {0, 2, 3};
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sAllyBattlers); i++)
    {
        u8 battler = sAllyBattlers[i];
        if (gBattleStruct->raid.respawnTimer[battler] == 0)
            continue;
        gBattleStruct->raid.respawnTimer[battler]--;
        if (gBattleStruct->raid.respawnTimer[battler] == 0)
        {
            // Restore to full HP and clear absent flag
            gBattleMons[battler].hp = gBattleMons[battler].maxHP;
            gAbsentBattlerFlags &= ~(1u << battler);
            BattleScriptExecute(BattleScript_RaidAllyRespawned);
        }
    }
}
```

### Critical pitfall: Plan 03 timer value is off-by-one

Plan 05-03 states "on faint set timer to 1." With `timer = 1`:
- Turn N: faint → `timer = 1`
- Turn N+1 start: `timer > 0` → decrement to 0 → **respawn fires immediately** (skip = 0 turns)

This contradicts **BATTLE-11** which requires skipping the *following* turn and respawning at the *start of the next turn after that*. The correct initial value is **2**:
- Turn N: faint → `timer = 2`
- Turn N+1 start: timer=2 → decrement to 1 → nothing (battler stays absent, action skipped)
- Turn N+2 start: timer=1 → decrement to 0 → respawn, clear absent flag

Use `respawnTimer[battler] = 2` in the intercept.

### Pitfall: `respawnTimer` array indexing

`respawnTimer[3]` is a 3-element array. The battler indices for allies are 0, 2, and 3. Direct indexing by battler index works (`respawnTimer[0]`, `respawnTimer[2]`, `respawnTimer[3]` would be out of bounds). **Either:**
- Declare `respawnTimer[4]` (indexed by battler) and waste index 1 (boss slot, never written), or
- Map battler → timer index: battler 0→index 0, battler 2→index 1, battler 3→index 2

The plan's current array size of 3 implies the compact mapping approach. Use a helper:
```c
static inline u8 RaidAllyTimerIndex(u8 battler)
{
    // battler 0 → 0, battler 2 → 1, battler 3 → 2
    return battler == 0 ? 0 : battler - 1;
}
```

### Pitfall: "all 3 allies simultaneously fainted"

Requirements success criterion #3 states: "When all 3 allies faint *or* turn 10 ends, the boss flees." If all three allies faint on the same turn (all have `respawnTimer > 0` and all are absent), the battle should immediately end rather than waiting for respawn. Check this condition in `TryRaidAllyRespawn` or in the `HandleFaintedMonActions` intercept — if all three ally timers are non-zero after setting the last one, trigger the storm-expiry sequence instead.

---

## 4. Storm Turn Counter and Messages (Plan 04)

### Existing global: `gBattleTurnCounter`

`gBattleTurnCounter` is declared in `src/battle_main.c` line 228 and is zero-initialized at battle start (line 3774). It increments exactly once per turn in `DoFieldEndTurnEffects → ENDTURN_ORDER` (line 1739). It does NOT need to be replicated in `RaidData`; you can read it directly.

If the plan requires a field in `RaidData` for isolation, add `u8 turnCounter` to `struct RaidData` and increment it in the ENDTURN_RAID_STORM case.

### Best hook: new case in `DoFieldEndTurnEffects`

`DoFieldEndTurnEffects()` in `src/battle_util.c` drives a state machine over the enum `{ ENDTURN_ORDER, ENDTURN_REFLECT, ..., ENDTURN_FIELD_COUNT }`. Add a new state just before the sentinel:

```c
// In the enum (src/battle_util.c, ~line 1619):
ENDTURN_RAID_STORM,   // <- insert before ENDTURN_FIELD_COUNT
ENDTURN_FIELD_COUNT,
```

```c
// In the switch (src/battle_util.c, ~line 2144):
case ENDTURN_RAID_STORM:
    if (gBattleTypeFlags & BATTLE_TYPE_RAID)
    {
        effect = TryRaidStormTick(); // returns 1 if script was dispatched
        gBattleStruct->turnCountersTracker++;
        break;
    }
    gBattleStruct->turnCountersTracker++;
    break;
```

`TryRaidStormTick()`:

```c
static bool32 TryRaidStormTick(void)
{
    // gBattleTurnCounter was already incremented in ENDTURN_ORDER this same tick
    if (gBattleTurnCounter >= 10)
    {
        gBattleOutcome = B_OUTCOME_PLAYER_TELEPORTED;
        BattleScriptExecute(BattleScript_RaidStormExpired);
        return TRUE;
    }
    // Normal turn: choose string 0 (growing stronger) or 1 (growing unbearable on turn 9)
    gBattleCommunication[MULTISTRING_CHOOSER] = (gBattleTurnCounter == 9) ? 1 : 0;
    BattleScriptExecute(BattleScript_RaidStormMessage);
    return TRUE;
}
```

### Pattern reference: `TryEndTurnWeather`

This is the direct model. It reads `gBattleCommunication[MULTISTRING_CHOOSER]` to pick between "continues" and "faded" strings, then calls `BattleScriptExecute`. Follow the same `BattleScriptExecute + effect++` idiom.

```c
// src/battle_util.c ~1700 (model)
gBattleCommunication[MULTISTRING_CHOOSER] = sBattleWeatherInfo[currBattleWeather].continuesMessage;
gBattleScripting.animArg1 = sBattleWeatherInfo[currBattleWeather].animation;
BattleScriptExecute(BattleScript_WeatherContinues);
effect++;
```

### Battle scripts needed

Add to `data/battle_scripts_1.s`:

```asm
BattleScript_RaidStormMessage::
    printfromtable gRaidStormStringIds      @ index from cMULTISTRING_CHOOSER
    waitmessage B_WAIT_TIME_LONG
    end2

BattleScript_RaidStormExpired::
    printstring STRINGID_STORM_HURLED_OUT_OF_DEN
    waitmessage B_WAIT_TIME_LONG
    end2
```

String table in `src/battle_message.c`:
```c
static const u8 *const gRaidStormStringIds[] = {
    [0] = COMPOUND_STRING("The storm is growing stronger."),
    [1] = COMPOUND_STRING("The storm is growing unbearable!"),
};
```

New string ID in `include/constants/battle_string_ids.h`:
```c
#define STRINGID_STORM_HURLED_OUT_OF_DEN   731
#define BATTLESTRINGS_COUNT                732
```

### Boss flee → overworld sequence

The outcome `B_OUTCOME_PLAYER_TELEPORTED` (value 5) maps to `HandleEndTurn_FinishBattle` in `sEndTurnFuncsTable` (`src/battle_main.c` line 392). `HandleEndTurn_FinishBattle` then calls `ReturnFromBattleToOverworld` → `CB2_ReturnToFieldContinueScriptPlayMapMusic` (already set as `gMain.savedCallback` in `DoRaidBattle`). This is exactly the correct sequence: fade to black, return to overworld, no extra messages.

**Do NOT use `B_OUTCOME_MON_FLED`** — that maps to `HandleEndTurn_MonFled` which sets up `BattleScript_WildMonFled` and prints a second unwanted message.

### Pitfalls for Plan 04

- **Turn number semantics**: `gBattleTurnCounter` increments in `ENDTURN_ORDER` which runs *before* `ENDTURN_RAID_STORM`. By the time `TryRaidStormTick` fires, the counter has already been incremented for the current turn. So after turn 1 completes, `gBattleTurnCounter == 1`. The expiry condition is `>= 10` (fires after the 10th turn completes). The "unbearable" message fires when `gBattleTurnCounter == 9` (end of turn 9) to warn players the next turn is the last — but read the game closely; S/V shows "unbearable" on the actual final turn AND then immediately expels. Adjust the threshold as needed.
- **`gBattleOutcome` check in `BattleTurnPassed`**: the outer loop at line 3947 only calls `DoFieldEndTurnEffects()` when `gBattleOutcome == 0`. Setting `gBattleOutcome` inside the VARIOUS handler is fine because it's checked AFTER the field effects finish.
- **Script stack depth**: `BattleScriptExecute` pushes `gBattleMainFunc` onto the callback stack and takes over. All `end2` calls in the new scripts properly reach `B_ACTION_TRY_FINISH`. Do not use `end3` (that pops the callback stack, bypassing `B_ACTION_TRY_FINISH`).
- **"All 3 allies fainted" expiry**: this separate expiry condition (besides turn 10) is not covered by any of the 4 plans. Handle it by checking in `TryRaidAllyRespawn` after setting the timer: if all three timers are now non-zero, call the same expiry sequence.

---

## 5. RaidData Struct — Current State and Required Additions

### Current definition (`include/battle.h` line 590):

```c
struct RaidData
{
    u8 dynamaxEnergy;        // index of the battler eligible to Dynamax this turn (0, 2, or 3)
    u8 allyIconSpriteId[2];  // icon sprite IDs for battlers 2 and 3; MAX_SPRITES = sentinel
    u8 shieldHp;             // current shield units on the boss
    u8 respawnTimer[3];      // per-ally faint countdown
};
```

### Fields to add for Phase 5:

| Field | Type | Purpose |
|---|---|---|
| `shieldPhase` | `u8` | 0=neither threshold triggered, 1=75% granted, 2=50% granted |

`turnCounter` can be omitted if using `gBattleTurnCounter` directly (recommended).

All added fields must be zeroed during the `memset` / struct initialization in Phase 4 Plan 01 (`gBattleStruct->raid = {0}`).

---

## 6. Existing Patterns Summary

| Mechanic | Pattern location | What to reuse |
|---|---|---|
| Per-turn message with string table | `TryEndTurnWeather` → `BattleScript_WeatherContinues` using `printfromtable` | Copy the `gBattleCommunication[MULTISTRING_CHOOSER]` + `BattleScriptExecute` pattern |
| End-of-turn field state machine | `DoFieldEndTurnEffects` enum + switch | Add `ENDTURN_RAID_STORM` before `ENDTURN_FIELD_COUNT` |
| Per-battler end-of-turn state machine | `DoBattlerEndTurnEffects` enum + switch | Add `ENDTURN_RAID_RESPAWN` before `ENDTURN_BATTLER_COUNT` (alternative to `TryRaidAllyRespawn`) |
| Zero-damage intercept in adjust phase | Ice Face in `Cmd_adjustdamage` | Copy the `moveDamage[battlerDef] = 0; continue;` pattern |
| Force battle end to overworld | `B_OUTCOME_PLAYER_TELEPORTED` + `HandleEndTurn_FinishBattle` | Set outcome, print custom string, `end2` |
| Turn counter that persists | `gBattleTurnCounter` (initialized to 0 at line 3774) | Read directly; no RaidData field needed |
| Start-of-turn hook for raid | `HandleEndTurn_ContinueBattle` alongside `TryAdvanceRaidRotation` | Add `TryRaidAllyRespawn()` call there |

---

## 7. File Change Map

| File | Changes needed |
|---|---|
| `include/battle.h` | Add `shieldPhase` to `struct RaidData` |
| `include/constants/battle_string_ids.h` | Add `STRINGID_STORM_HURLED_OUT_OF_DEN`, increment `BATTLESTRINGS_COUNT` |
| `src/battle_script_commands.c` | Shield intercept in `Cmd_adjustdamage`; threshold check in `Cmd_datahpupdate` |
| `src/battle_util.c` | Add `ENDTURN_RAID_STORM` to field effects enum+switch; intercept in `HandleFaintedMonActions` case 4 |
| `src/battle_main.c` | Call `TryRaidAllyRespawn()` in `HandleEndTurn_ContinueBattle` |
| `src/raid_den.c` | Implement `TryRaidAllyRespawn()`, `TryRaidStormTick()` |
| `include/raid_den.h` | Declare `TryRaidAllyRespawn()`, confirm `gRaidBattleData` extern |
| `src/battle_message.c` | Add `gRaidStormStringIds` table; add `STRINGID_STORM_HURLED_OUT_OF_DEN` string |
| `data/battle_scripts_1.s` | Add `BattleScript_RaidStormMessage`, `BattleScript_RaidStormExpired`, `BattleScript_RaidAllyRespawned` |

---

## 8. Critical Pitfalls by Plan

### Plan 01 (Shield intercept)
- Do NOT set `MOVE_RESULT_NO_EFFECT` — it suppresses animations and ability checks.
- Multi-hit: each hit goes through `Cmd_adjustdamage` independently; each hit drains 1 shield. Clamp `shieldHp` with `> consume ? shieldHp - consume : 0`.
- Ensure the guard uses `battlerDef` (the loop variable) not `gBattlerTarget`.

### Plan 02 (Threshold activation)
- Add `shieldPhase` field to `struct RaidData` — it's not there yet.
- Guard against passive damage (check `!(gHitMarker & HITMARKER_PASSIVE_DAMAGE)`).
- Handle single-hit jump through both thresholds in one check.

### Plan 03 (Respawn timer)
- **Timer must be 2, not 1.** Plan says 1 but BATTLE-11 requires skipping one full turn. `timer = 1` respawns immediately at the start of the NEXT turn (0 turns skipped).
- `respawnTimer[3]` is indexed compactly (0,1,2), not by battler index (0,2,3). Use a mapping helper or resize to `respawnTimer[4]` indexed by battler.
- The "all 3 fainted simultaneously" condition is not covered by any plan — needs an explicit check.

### Plan 04 (Storm counter)
- `gBattleTurnCounter` is already incremented in `ENDTURN_ORDER` before `ENDTURN_RAID_STORM` fires; the expiry check sees the already-incremented value.
- Use `B_OUTCOME_PLAYER_TELEPORTED` (not `MON_FLED`) to avoid a double message.
- The "unbearable" vs "stronger" message timing: decide whether "unbearable" is the last message before expiry (shown on turn 10) or a warning message (shown on turn 9). Both interpretations are defensible.
- All new battle scripts must use `end2`, not `end3`.
