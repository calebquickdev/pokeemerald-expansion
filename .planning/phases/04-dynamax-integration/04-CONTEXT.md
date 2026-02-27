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
- Strict cycle: Turn 1 → player, Turn 2 → CPU ally 1, Turn 3 → CPU ally 2, Turn 4 → back to player
- Out-of-rotation battlers cannot Dynamax; the option is not shown (hidden, not grayed out)
- `dynamaxEnergy` in `RaidData` tracks the current rotation index (0 = player, 1 = ally 1, 2 = ally 2)

### Rotation Interruption (fainted ally)
- Claude's Discretion: If the eligible battler has fainted on their Dynamax turn, skip to the next live battler in the rotation order; do not hold the turn or let nobody Dynamax

### Dynamax Duration
- Claude's Discretion: Standard 3-turn Dynamax applies to all battlers (player and allies); `dynamaxTurns[]` already tracks this for the boss at `0xFF`; allies use the normal counter

### Ally Sprite Swap
- On Dynamax trigger: replace the icon sprite with the full front sprite (same as the boss uses)
- On `UndoDynamax`: restore the icon sprite
- Applies to CPU ally battlers only (battlers 2 and 3); player uses standard battle sprite

### Claude's Discretion
- Exact hook point for advancing the rotation counter (end of turn vs. start of next turn) — planner decides
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
