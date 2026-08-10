# 8.9 Feedback Bug Tracker

Statuses:
- **Known bugs** — reported / under investigation, no fix yet
- **Known bugs - Patched** — fix landed (or in progress locally), not yet playtested/approved
- **Known bugs - Tested and approved** — verified in-game and signed off

---

## Known bugs

### Swagger applies Attack boost but no confusion
- **Steps to reproduce**
  1. Enter a battle where an opponent uses Swagger on your Pokémon.
  2. Observe the battle effects after Swagger resolves.
- **Expected Outcome**
  Target’s Attack rises by 2 stages and the target becomes confused.
- **Status:** Known bugs

### Roxanne sends out Nosepass last instead of Larvitar
- **Steps to reproduce**
  1. Challenge Roxanne.
  2. Play through the battle and note her send-out / switch order.
- **Expected Outcome**
  Confirm whether AI send-out order is intentional (smarter switch logic) or incorrect vs intended team order. Document how the AI chooses who comes out.
- **Status:** Known bugs

---

## Known bugs - Patched

### Mauville indoor warp black-screen crash (PC / bike shop / Electric Gym)
- **Steps to reproduce**
  1. Travel to Mauville City (sets `FLAG_SYS_TV_START`).
  2. Walk into the Pokémon Center, bike shop, and/or Electric Gym.
  3. Observe intermittent permanent black screen after the door fade.
- **Expected Outcome**
  Indoor warps in Mauville (and elsewhere after visiting Mauville) load normally.
- **Root cause**
  `TV_SHOWS_COUNT` was truncated to 5 for SaveBlock1 space, but `tv.c` still assumed vanilla record-mix + scratch slots. After Mauville arms TV, every indoor load runs `UpdateTVScreensOnMap` → `GetRandomActiveShowIdx`; related writers (e.g. `DeleteExcessMixedShows`) could OOB.
- **Fix**
  Keep truncated arrays (restore costs ~768 bytes; SB1 only has ~108 free). Harden `tv.c` for no mix/scratch region: safe show scan, gate OOB mix compaction, no-op scratch-slot builders.
- **Status:** Known bugs - Patched  
  *(Local uncommitted changes in `src/tv.c` / `include/constants/tv.h`)*

### Cannot catch fishing encounters in Dewford (Nuzlocke route lock)
- **Steps to reproduce**
  1. Play a Nuzlocke run and arrive in Dewford for the first time.
  2. Fish and get a wild encounter (e.g. Palafin).
  3. Attempt to catch it.
  4. Compare with fishing on Route 107 (east of Dewford), where catching worked.
- **Expected Outcome**
  First eligible fishing encounter in Dewford can be caught. Route locks should key off region map section (`MAPSEC`), not raw map id, so Dewford Town / nearby water maps that share a section behave consistently.
- **Status:** Known bugs - Patched  
  *(Commit `f353c5b4` — Nuzlocke MAPSEC route locks)*

### Doubles + random moves: randomized moves revert to original moves
- **Steps to reproduce**
  1. Enable random moves.
  2. Enter a double battle.
  3. Observe move sets after switch-in / mid-battle state updates.
- **Expected Outcome**
  Randomized moves stay applied for the whole battle (including after switch-in); they should not snap back to the Pokémon’s non-random moves.
- **Status:** Known bugs - Patched  
  *(Commit `f353c5b4` — re-apply resolved moves on switch-in)*

### Field Cut animation shows the wrong party Pokémon
- **Steps to reproduce**
  1. Have a party where one mon knows Cut (e.g. Bisharp) but is not the lead (e.g. Scrafty is lead), or rely on HM lead fallback.
  2. Use Cut on a tree in the overworld.
  3. Watch the field-move animation / “X used Cut!” text.
- **Expected Outcome**
  The Pokémon that actually performs the field move is the one shown in the animation and named in the text. (Lead fallback when no HM user is present is intentional so an HM slave is not required.)
- **Status:** Known bugs - Patched  
  *(Commit `f353c5b4` — HM lead fallback + field-move script nick/text fix)*

### Soft reset / New Game does not keep prior challenge settings (starters look vanilla)
- **Steps to reproduce**
  1. Configure New Game settings (e.g. randomized starters / challenge flags).
  2. Soft reset or start New Game in a way that should preserve those settings.
  3. Check starter options — reporter saw all 9 actual (non-randomized) starters.
- **Expected Outcome**
  Challenge / randomizer settings chosen before reset are preserved and applied (including starter randomization mode and related flags).
- **Status:** Known bugs - Patched  
  *(Seed menu from save when present; confirm applies + flushes settings before Birch; NewGameInitData backs up/restores challenge flags)*

---

## Known bugs - Tested and approved

*(None yet — move items here after in-game verification.)*
