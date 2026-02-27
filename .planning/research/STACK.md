# Technology Stack: Dynamax Raid Dens (4v1 Battle Format)

**Project:** pokeemerald-expansion  
**Researched:** 2025-02-27  
**Domain:** GBA ROM hack battle engine — custom 4v1 raid battle format

## Executive Summary

The pokeemerald-expansion codebase already has foundational pieces for raid battles: `BATTLE_TYPE_RAID` exists, Dynamax has raid-related TODOs, and the 2v1 ingame-partner pattern provides a template for multi-ally battles. However, **MAX_BATTLERS_COUNT is 4**, so a true 4v1 (4 player-side + 1 boss) requires either extending the battler limit or using a 3v1 layout (1 player + 2 allies vs 1 boss) that fits within the current engine.

---

## 1. Battle Type and Participant Setup

### Key Files and Symbols

| Location | Symbol / Pattern | Purpose |
|----------|-----------------|---------|
| `include/constants/battle.h` | `BATTLE_TYPE_RAID (1 << 12)` | Already defined, currently unused |
| `include/constants/battle.h` | `BATTLE_TYPE_INGAME_PARTNER (1 << 22)` | 2v1 pattern: player + partner vs opponent |
| `include/constants/battle.h` | `BATTLE_TWO_VS_ONE_OPPONENT` | Macro: `INGAME_PARTNER && opponentB == 0xFFFF` |
| `include/constants/battle.h` | `MAX_BATTLERS_COUNT` | **4** — hard limit on battlers |
| `include/constants/battle.h` | `B_POSITION_*` | Player left/right, Opponent left/right |
| `src/battle_main.c:144` | `gBattleTypeFlags` | EWRAM, set before battle init |
| `src/battle_main.c:150` | `gBattlersCount` | Set in `InitSinglePlayerBtlControllers` |
| `src/battle_controllers.c:119–405` | `InitSinglePlayerBtlControllers` | Assigns controllers and `gBattlersCount` |

### How Battle Types Are Distinguished

- `gBattleTypeFlags` is a bitmask; multiple flags can be set (e.g. `BATTLE_TYPE_DOUBLE | BATTLE_TYPE_RAID`).
- Checks use `gBattleTypeFlags & BATTLE_TYPE_*` throughout `battle_util.c`, `battle_controllers.c`, `battle_script_commands.c`, etc.
- `gBattlersCount` is 2 for single battles, `MAX_BATTLERS_COUNT` (4) for double/multi/ingame-partner.

### Recommended Approach for Raid

1. **Use `BATTLE_TYPE_RAID`** — already present; combine with `BATTLE_TYPE_DOUBLE` for multi-ally.
2. **3v1 layout** — 1 player + 2 CPU allies vs 1 boss fits within 4 battlers:
   - Battler 0: Player (B_POSITION_PLAYER_LEFT)
   - Battler 1: Boss (B_POSITION_OPPONENT_LEFT)
   - Battler 2: Ally 1 (B_POSITION_PLAYER_RIGHT)
   - Battler 3: Ally 2 — requires a new position or reuse. Current layout has only 4 positions. **Constraint:** Standard positions are 2v2. For 3v1 you could use B_POSITION_OPPONENT_RIGHT for ally 2 (treat as player-side by side check) — but that would conflict with `GetBattlerSide()`. Safer: extend with a raid-specific position mapping or use 2v1 (1 player + 1 ally) as the initial scope.
3. **Alternative: extend to 5 battlers** — would require changes to `MAX_BATTLERS_COUNT`, `gBattlerPositions`, `gBattleMons`, and all battler-indexed arrays. **High effort.**

**Confidence:** HIGH for using `BATTLE_TYPE_RAID`; MEDIUM for exact 4v1 layout (engine limits).

---

## 2. Battle Controllers

### Key Files and Symbols

| Location | Symbol | Purpose |
|----------|--------|---------|
| `include/battle_controllers.h` | `SetControllerToPlayer`, `SetControllerToOpponent`, `SetControllerToPlayerPartner`, `SetControllerToSafari`, `SetControllerToWally` | Controller assignment functions |
| `src/battle_controllers.c:119–405` | `InitSinglePlayerBtlControllers` | Assigns `gBattlerControllerFuncs[i]` and `gBattlerPositions[i]` |
| `src/battle_controller_player_partner.c` | `SetControllerToPlayerPartner`, `sPlayerPartnerBufferCommands` | AI partner: uses `PlayerPartnerHandleChooseAction` (AI), `PlayerPartnerHandleChooseMove` |
| `src/battle_controller_opponent.c` | `SetControllerToOpponent` | Standard opponent AI |
| `src/battle_controller_safari.c` | `SetControllerToSafari` | Safari-specific actions (Throw Rock/Bait, Run) |

### Controller Assignment Pattern

```c
// From battle_controllers.c:124–155 (INGAME_PARTNER)
gBattlerControllerFuncs[0] = SetControllerToPlayer;
gBattlerControllerFuncs[1] = SetControllerToOpponent;
gBattlerControllerFuncs[2] = SetControllerToPlayerPartner;  // AI ally
gBattlerControllerFuncs[3] = SetControllerToOpponent;
gBattlersCount = MAX_BATTLERS_COUNT;
```

### Recommended Approach for Raid Allies

1. **Add `SetControllerToRaidAlly`** — new controller similar to `SetControllerToPlayerPartner` but with raid-specific behavior:
   - No bag, no run, no catch
   - AI chooses moves (can reuse or extend `BattleAI_ChooseMove` / `BattleAI_ChooseAction`)
   - Dynamax rotation: only the battler with "Dynamax energy" this turn can Dynamax
2. **Register in `InitSinglePlayerBtlControllers`** — add a `BATTLE_TYPE_RAID` branch that assigns `SetControllerToRaidAlly` for ally battlers.
3. **Reuse `battle_controller_player_partner.c`** — clone and adapt, or add a mode flag so the same controller behaves differently when `BATTLE_TYPE_RAID` is set.

**Confidence:** HIGH — pattern is clear; implementation is straightforward.

---

## 3. Dynamax Integration

### Key Files and Symbols

| Location | Symbol / Pattern | Purpose |
|----------|------------------|---------|
| `include/config/battle.h:169` | `B_FLAG_DYNAMAX_BATTLE` | Enables Dynamax when set |
| `src/battle_dynamax.c:73–122` | `CanDynamax` | Checks Dynamax eligibility |
| `src/battle_dynamax.c:115–116` | TODO comment | `gBattleStruct->raid.dynamaxEnergy` — not yet implemented |
| `src/battle_dynamax.c:132–146` | `ApplyDynamaxHPMultiplier` | Applies HP multiplier for Dynamaxed mons and raid bosses |
| `src/battle_dynamax.c:238–241` | `IsMoveBlockedByDynamax` | Weight-based moves blocked; TODO for raid move bans |
| `include/battle.h:755` | `struct DynamaxData dynamax` | In BattleStruct; no `raid` substruct yet |
| `src/battle_gimmick.c` | `CanActivate`, `ActivateGimmick` | Gimmick (Dynamax) activation flow |

### Boss Permanent Dynamax

- Boss should start Dynamaxed and never call `UndoDynamax`. Options:
  1. Set `SetActiveGimmick(bossBattler, GIMMICK_DYNAMAX)` at battle start and skip `UndoDynamax` in end-of-battle cleanup when `BATTLE_TYPE_RAID`.
  2. Add a flag in `gBattleStruct` (e.g. `raid.bossPermanentDynamax`) and guard `UndoDynamax` / turn-end logic.

### Dynamax Rotation (Turns 1–4)

- Add `gBattleStruct->raid.dynamaxEnergy` (or similar) to track which battler can Dynamax each turn.
- In `CanDynamax`, add:
  ```c
  if (gBattleTypeFlags & BATTLE_TYPE_RAID && gBattleStruct->raid.dynamaxEnergy != battler)
      return FALSE;
  ```
- Update `dynamaxEnergy` each turn (cycle through player + allies).

### Boss HP × 4

- `ApplyDynamaxHPMultiplier` already applies a multiplier from `GetDynamaxLevelHPMultiplier`. For raid bosses, use a custom multiplier (e.g. 4×) or a separate code path in that function when `BATTLE_TYPE_RAID` and battler is boss.

**Confidence:** HIGH — structure exists; TODOs point to the right places.

---

## 4. Turn Limit and End-of-Turn Message

### Key Files and Symbols

| Location | Symbol / Pattern | Purpose |
|----------|------------------|---------|
| `src/battle_main.c:3916–3937` | `HandleEndTurn_ContinueBattle` | Runs when turn ends (no outcome yet) |
| `src/battle_main.c:3940–3995` | `BattleTurnPassed` | Increments turn, runs `DoFieldEndTurnEffects`, `DoBattlerEndTurnEffects` |
| `src/battle_util.c:1710–2225` | `DoFieldEndTurnEffects` | Turn-end effects; `gBattleTurnCounter++` at `ENDTURN_ORDER` |
| `src/battle_main.c:3979–3982` | `gBattleResults.battleTurnCounter++` | Turn counter for results |
| `src/battle_main.c:5373` | `gCurrentTurnActionNumber >= gBattlersCount` | Condition for "turn finished" |

### Hook for Turn Limit and Message

1. **Turn limit check** — In `BattleTurnPassed` or at the start of `HandleEndTurn_ContinueBattle`, add:
   ```c
   if (gBattleTypeFlags & BATTLE_TYPE_RAID && gBattleResults.battleTurnCounter >= 10) {
       gBattleOutcome = B_OUTCOME_LOST;  // or custom outcome
       // ... transition to loss
   }
   ```
2. **End-of-turn message** — After `DoFieldEndTurnEffects` / `DoBattlerEndTurnEffects`, before continuing:
   ```c
   if (gBattleTypeFlags & BATTLE_TYPE_RAID) {
       BattleScriptExecute(BattleScript_RaidTurnEndMessage);  // New script
       return;  // Wait for script/controller
   }
   ```
   Or use `BtlController_EmitPrintString` with a "Turn X of 10" string.

**Confidence:** HIGH — turn flow is well-defined; hook points are clear.

---

## 5. Shield Mechanic

### Approach

- **Storage:** Add `gBattleStruct->raid.shieldHp` (or `shieldUnits`) for the boss.
- **Thresholds:** When boss HP crosses 75% and 50%, set shield units (e.g. 2 each, or configurable).
- **Damage interception:** In the damage path (e.g. `CalculateMoveDamage` or the script command that applies damage), when target is raid boss and `shieldHp > 0`:
  - Reduce damage by shield units (each hit removes 1 unit)
  - Only apply remaining damage to HP once shield is 0
- **Script integration:** A new script command (e.g. `raid_applydamage`) or modification of the existing damage application to branch on `BATTLE_TYPE_RAID` and shield state.

### Key Damage Path

- `src/battle_script_commands.c` — `Cmd_critcalc` / damage application
- `src/battle_util.c` — `CalculateMoveDamage`
- Shield logic should sit where damage is finally applied to `gBattleMons[target].hp`.

**Confidence:** MEDIUM — requires tracing the exact damage application path and ensuring compatibility with spread moves, etc.

---

## 6. Faint / Revive (Boss Respawns Allies)

### Key Files and Symbols

| Location | Pattern | Purpose |
|----------|---------|---------|
| `src/battle_script_commands.c` | `HandleFaintedMonActions`, faint scripts | Faint handling |
| `src/battle_util.c` | `DoBattlerEndTurnEffects` | Turn-end effects including faint checks |
| `gAbsentBattlerFlags` | Bit per battler | Marks absent/fainted battlers |

### Approach

- Fainted allies are normally removed from the field. For "respawn after 1 turn":
  1. Mark ally as "fainted but will respawn" (e.g. `gBattleStruct->raid.respawnTimer[ally] = 1`).
  2. Skip normal "send next mon" flow for that ally.
  3. On the next turn end, decrement timer; when 0, restore ally at full HP and bring them back (similar to switch-in).
- Implementation will likely need:
  - New script commands or battle script hooks
  - Modifications to `HandleFaintedMonActions` or the faint-handling script to branch on `BATTLE_TYPE_RAID` for ally battlers.

**Confidence:** MEDIUM — faint/switch flow is complex; respawn is non-standard and will need careful integration.

---

## 7. Post-Battle Catch (100% Catch, Any Ball)

### Key Files and Symbols

| Location | Symbol / Pattern | Purpose |
|----------|------------------|---------|
| `src/battle_script_commands.c:15715–15990` | `Cmd_handleballthrow` | Catch logic |
| `src/battle_script_commands.c:15755` | `gBattleStruct->safariCatchFactor * 1275 / 100` | Safari catch rate |
| `src/battle_script_commands.c:15743` | `BALL_3_SHAKES_SUCCESS` | Wally tutorial forces success |
| `src/battle_script_commands.c:15735–15739` | `BATTLE_TYPE_TRAINER` | Blocks catch |
| `include/constants/battle.h:98` | `B_OUTCOME_CAUGHT` | Catch outcome |

### Recommended Approach

In `Cmd_handleballthrow`, add a branch for `BATTLE_TYPE_RAID`:

```c
else if (gBattleTypeFlags & BATTLE_TYPE_RAID)
{
    // 100% catch, any ball
    BtlController_EmitBallThrowAnim(gBattlerAttacker, BUFFER_A, BALL_3_SHAKES_SUCCESS);
    MarkBattlerForControllerExec(gBattlerAttacker);
    gBattlescriptCurrInstr = BattleScript_RaidBallThrow;  // or reuse Wally path
}
```

- Use `BALL_3_SHAKES_SUCCESS` to force success.
- Ensure post-battle flow goes to catch screen (like wild/Safari) and that `gBattleOutcome = B_OUTCOME_CAUGHT` is set.
- Custom catch screen (e.g. "Choose a Poké Ball") can be handled in the battle setup or a dedicated raid-end callback.

**Confidence:** HIGH — pattern from Wally/Safari is directly applicable.

---

## 8. Battle Init and Setup

### Key Files

| Location | Purpose |
|----------|---------|
| `src/battle_setup.c` | `DoStandardWildBattle`, `DoTrainerBattle`, etc. |
| `src/battle_main.c:449–447` | `CB2_InitBattle` — branches on `BATTLE_TYPE_MULTI`, `BATTLE_TYPE_INGAME_PARTNER` |
| `src/field_specials.c` | Script commands that start battles |

### Approach

- Add a new setup path (e.g. `DoRaidBattle`) that:
  1. Sets `gBattleTypeFlags = BATTLE_TYPE_DOUBLE | BATTLE_TYPE_RAID` (and any other flags).
  2. Fills `gEnemyParty` with the raid boss (with 4× HP applied).
  3. Fills ally parties (from `data/battle_partners.h` or a new raid-ally table).
  4. Sets `TRAINER_BATTLE_PARAM` or equivalent for raid-specific params.
- Call this from a map script or special (e.g. interacting with a raid den).

**Confidence:** HIGH — follows existing battle setup patterns.

---

## 9. What NOT to Do

| Anti-Pattern | Reason |
|--------------|--------|
| Assume 5+ battlers without extending `MAX_BATTLERS_COUNT` | Engine is built for 4; arrays and logic assume 4. |
| Use `BATTLE_TYPE_INGAME_PARTNER` alone for raid | That path is 2v1; raid needs its own controller and layout. |
| Add `malloc` for raid state | Project rules forbid heap in battle code; use `gBattleStruct` or static. |
| Modify `CalculateMoveDamage` without considering spread moves | Shield logic must work for all move target patterns. |
| Skip `BATTLE_TYPE_RAID` checks in Dynamax cleanup | Boss must stay Dynamaxed; `UndoDynamax` in `HandleEndTurn_FinishBattle` would break that. |
| Reuse Safari controller for raid | Safari has Throw Rock/Bait; raid needs Fight/Pokémon (no bag/run). |

---

## 10. Implementation Order Recommendation

1. **Phase 1 — Core setup**
   - Add `BATTLE_TYPE_RAID` branch in `InitSinglePlayerBtlControllers` (3v1: player + 2 allies vs boss).
   - Add `SetControllerToRaidAlly` (clone `SetControllerToPlayerPartner`).
   - Add `DoRaidBattle` or equivalent in `battle_setup.c`.

2. **Phase 2 — Dynamax**
   - Add `struct RaidData` (or similar) to `BattleStruct` with `dynamaxEnergy`, `shieldHp`, `respawnTimer[]`.
   - Implement permanent boss Dynamax and rotation for allies.
   - Apply 4× HP for boss.

3. **Phase 3 — Mechanics**
   - Shield mechanic in damage application.
   - Turn limit and end-of-turn message.
   - Ally respawn (skip 1 turn, return at full HP).

4. **Phase 4 — Catch**
   - 100% catch in `Cmd_handleballthrow` for `BATTLE_TYPE_RAID`.
   - Custom catch screen if desired.

---

## 11. Confidence Summary

| Area | Confidence | Notes |
|------|------------|-------|
| Battle type / flags | HIGH | `BATTLE_TYPE_RAID` exists; pattern is clear |
| Controllers | HIGH | `SetControllerToPlayerPartner` is a good template |
| Dynamax | HIGH | TODOs and `ApplyDynamaxHPMultiplier` support it |
| Turn limit / message | HIGH | Hook points are well-defined |
| Shield | MEDIUM | Needs damage-path tracing |
| Respawn | MEDIUM | Non-standard; faint flow is complex |
| Catch | HIGH | Wally/Safari pattern applies |
| 4v1 layout | LOW | Engine limit of 4 battlers; 3v1 is safer |

---

## 12. Files to Modify (Summary)

| File | Changes |
|------|---------|
| `include/constants/battle.h` | Possibly new position/helper macros for raid |
| `include/battle.h` | Add `struct RaidData` to `BattleStruct` |
| `src/battle_controllers.c` | `BATTLE_TYPE_RAID` branch in `InitSinglePlayerBtlControllers` |
| `src/battle_controller_raid_ally.c` | New file — `SetControllerToRaidAlly` |
| `src/battle_dynamax.c` | Raid checks in `CanDynamax`, boss HP mult, permanent Dynamax |
| `src/battle_main.c` | Turn limit check, `BATTLE_TYPE_RAID` in end-of-battle |
| `src/battle_script_commands.c` | `Cmd_handleballthrow` raid branch, damage/shield logic |
| `src/battle_setup.c` | `DoRaidBattle` or equivalent |
| `src/battle_util.c` | Possibly shield in damage path, respawn in faint handling |
| `include/battle_controllers.h` | Declare `SetControllerToRaidAlly` |
| `data/scripts/*.inc` or `field_specials.c` | Script command to start raid battle |
