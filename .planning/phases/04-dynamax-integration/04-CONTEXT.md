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
- Rotation advances each turn regardless of whether Dynamax was used
- The Dynamax option appears in the attack selection menu only for the currently eligible battler; all other battlers never see it
- No energy mechanic — eligibility is purely rotation-based
- Only one battler may be Dynamaxed at a time; while any battler is actively Dynamaxed, the rotation does not advance and no other ally may Dynamax until it ends

### Rotation Interruption (fainted ally)
- If the eligible battler has fainted on their Dynamax turn, the slot is skipped — nobody Dynamaxes that turn; rotation advances normally on the next turn

### Dynamax Duration
- 3 turns total, counting the activation turn (so the battler attacks as Dynamaxed on turns 1, 2, and 3, then reverts at the start of turn 4)

### Ally Sprite Swap
- On Dynamax trigger: replace the icon sprite with the full front sprite (same as the boss uses)
- On `UndoDynamax`: restore the icon sprite
- Applies to CPU ally battlers only (battlers 2 and 3); player uses standard battle sprite

### Claude's Discretion
- Exact hook point for advancing the rotation counter (end of turn vs. start of next turn) — planner decides
- Field naming for the rotation index in `RaidData` — planner decides

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
