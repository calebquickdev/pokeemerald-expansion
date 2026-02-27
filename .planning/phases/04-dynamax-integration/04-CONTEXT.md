# Phase 4: Dynamax Integration - Context

**Gathered:** 2026-02-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Implements the Dynamax rotation system (who can Dynamax each turn) and the ally sprite swap when an ally enters/exits Dynamax state. Does not include shields, storm turns, or ally respawn (Phase 5).

</domain>

<decisions>
## Implementation Decisions

### Dynamax Rotation
- Rotation order: player → CPU ally 1 → CPU ally 2 → back to player
- The rotation only advances when the current Dynamax expires; while any battler is Dynamaxed, no other battler may Dynamax
- Out-of-rotation battlers cannot Dynamax; the option is not shown in the attack selection menu
- `dynamaxEnergy` in `RaidData` tracks the rotation index (0 = player, 1 = ally 1, 2 = ally 2); advances when the active Dynamax ends

### Dynamax Duration
- 3 turns total including the activation turn (turns 1, 2, 3 — ends at close of turn 3)
- Standard `dynamaxTurns[]` counter handles this; boss `0xFF` sentinel from Phase 3 is untouched

### Rotation Interruption (fainted ally)
- If the eligible battler has fainted, the rotation stays on that battler's slot for that turn; nobody Dynamaxes that turn and the rotation counter does not advance
- On the following turn, the fainted battler is still skipped (same result) until they are revived (Phase 5 handles respawn)

### Ally Sprite Swap
- On Dynamax trigger: replace the icon sprite with the full front sprite
- On `UndoDynamax`: restore the icon sprite
- Applies to CPU ally battlers only (battlers 2 and 3); player uses standard battle sprite

### Claude's Discretion
- Exact hook point for advancing the rotation counter (when Dynamax expires vs. start of next turn) — planner decides
- Whether `dynamaxEnergy` is renamed to `rotationIndex` for clarity — planner decides

</decisions>

<specifics>
## Specific Ideas

- The `RaidData` struct (defined in 04-01) also holds `shieldHp` and `respawnTimer[]` for Phase 5; these fields are zeroed at battle start but otherwise untouched this phase
- Boss permanent Dynamax (`dynamaxTurns[1] = 0xFF`) was established in Phase 3 (03-03); this phase must not disturb that

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 04-dynamax-integration*
*Context gathered: 2026-02-27*
