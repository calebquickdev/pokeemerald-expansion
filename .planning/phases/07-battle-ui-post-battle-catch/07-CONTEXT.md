# Phase 7: Battle UI & Post-Battle Catch - Context

**Gathered:** 2026-02-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver the custom raid battle HUD (ally icon slots on the left, boss front sprite on the right) and the post-battle catch sequence (ball selection, guaranteed catch, GMAX delivery). No new battle mechanics — those are Phase 5. No lobby changes — those are Phase 6.

</domain>

<decisions>
## Implementation Decisions

### Ally HUD Layout
- HP bars show **both** the proportional bar **and** current/max HP numbers
- Bars use **dynamic color thresholds** (green → yellow → red) matching the main party screen behavior
- 3 ally slots arranged **stacked vertically** — player on top, CPU ally 1 in the middle, CPU ally 2 at the bottom
- A fainted ally in the "skip turn / waiting to respawn" state shows their icon **greyed/dimmed** with an empty HP bar

### Run Menu Behavior
- Selecting Run shows a **confirmation prompt** before exiting
- After confirmation, a **"You fled from the raid!"** (or equivalent) message plays before the screen fades
- Den **stays active** after the player runs — they can re-enter until the daily reset
- Player returns to **their last overworld position** (wherever they were before the lobby opened)

### Ball Selection UX
- After the boss is KO'd the ball selection prompt appears **after the battle result screen** ("You won!" or equivalent) fades out
- A **small custom list** of only the balls currently in the bag is shown (not the full Bag screen)
- The **first throw is always a guaranteed catch** — no failure path exists
- **No balls in bag:** boss flees after being beaten, den becomes inactive (FLAG_DAILY_DEN_RAIDED set), player returns to overworld with no catch opportunity
- **Nuzlocke rules block catching:** same outcome as no balls — boss flees, den inactive, return to overworld

### Post-Catch Delivery
- **Standard caught fanfare** plays after the catch
- **GMAX catch:** player receives the base (non-GMAX) species with Gigantamax Factor set; a message appears calling it out — e.g. "[Name] has the Gigantamax Factor!"
- **Party full:** caught Pokémon is automatically sent to the PC (same behavior as standard overworld catches — no prompt)
- Den becomes **inactive** (FLAG_DAILY_DEN_RAIDED set) after a successful catch

### Claude's Discretion
- Exact wording of "You fled from the raid!" message
- Exact wording of Run confirmation prompt
- Visual styling details of the custom ball selection list (font, border, positioning)
- Layout precision of the ally HUD (exact pixel positions, window/tile allocations)
- Battle result screen behavior before ball prompt (whether a score/reward summary exists is not scoped here)

</decisions>

<specifics>
## Specific Requirements

- GMAX factor message must be distinct and visible — the player should not have to discover it themselves in their party
- The "no catch" path (no balls or nuzlocke blocked) must mark the den inactive consistently — same as a successful catch, the den is consumed

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 07-battle-ui-post-battle-catch*
*Context gathered: 2026-02-28*
