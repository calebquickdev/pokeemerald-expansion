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
- **Investigation (not decisive)**
  Confusion is not on Swagger’s `additionalEffects`; it is applied only via `CheckSpecificMoveCondition` → `st->moveScript = BattleScript_SwaggerConfusion` → `seteffectprimary(..., MOVE_EFFECT_CONFUSION)`. Attack still rises when `CanBeConfused` is false (Safeguard / Misty Terrain / already confused), with no confusion fail text — matches “+2 Atk, no confusion.”
  Our `BattleScript_SwaggerConfusion` uses `BS_TARGET`; current pokeemerald-expansion uses `BS_SCRIPTING` (same pattern as Toxic Thread). `gBattleScripting.battler` is set to the defender on the Attack raise; `gBattlerTarget` is not updated in the stat-change loop. That divergence is a strong lead but not proven as the sole cause. Unit test `test/battle/move_effect/swagger.c` still expects confusion.
  Also note: `B_OVERWORLD_FOG = GEN_LATEST` turns overworld fog into Misty Terrain, which silently blocks Swagger confusion while still allowing the Attack boost.
- **Git history**
  Path landed in expansion import `653e686a` (stat-change rewrite). No later local commits touch Swagger/confusion application.
- **Status:** Known bugs — needs in-battle repro (Confuse Ray / Dynamic Punch / fog vs clear weather / Substitute) before a fix

### Petalburg Gym → Pokémon Center crash (after beating a trainer)
- **Steps to reproduce**
  1. Be past Mauville (`FLAG_SYS_TV_START` set; Norman’s gym is after Wattson).
  2. Beat a trainer in Petalburg Gym, leave the gym, walk toward / into the Petalburg Pokémon Center.
  3. Game crashes / black-screens on the indoor warp.
- **Expected Outcome**
  Outdoor → Pokémon Center warp loads normally.
- **Root cause (high confidence — same as Mauville indoor crash)**
  After Mauville, every indoor map load runs `UpdateTVScreensOnMap` → `GetRandomActiveShowIdx`. Truncated `TV_SHOWS_COUNT` (SaveBlock1) made that path OOB. Petalburg PC is the same class of warp; not gym-script specific.
- **Git history**
  Truncation present since import `653e686a`. Hardened in `f69d6d14` (same fix as Mauville).
- **Status:** Known bugs — treat as duplicate of Mauville indoor TV crash (patched in `f69d6d14`; root harden in `tv.c` gates mix/scratch paths and fixes airable slot-4 scans)

### Roxanne sends out Nosepass last instead of Larvitar
- **Steps to reproduce**
  1. Challenge Roxanne.
  2. Play through the battle and note her send-out / switch order.
- **Expected Outcome**
  Confirm whether AI send-out order is intentional (smarter switch logic) or incorrect vs intended team order. Document how the AI chooses who comes out.
- **Status:** Known bugs

---

## Known bugs - Patched

### Route 120 rain/puddles: “Out of sprite slots” fatal crash
- **Steps to reproduce**
  1. Leave Fortree after Winona and run east onto Route 120 into the rain zone.
  2. Run through puddle tiles (especially with a follower and several NPCs on-screen).
  3. Game hard-crashes: `src/sprite.c: Out of sprite slots` (stack included `FldEff_Splash`).
- **Expected Outcome**
  Overworld runs without exhausting the 64-sprite pool on busy routes; no fatal assert.
- **Root cause**
  Route 120 combines rain (~10 sprites), always-on object shadows (`OW_OBJECT_VANILLA_SHADOWS` FALSE → 2 sprites per object), follower, and puddle splashes. Puddle splash (`FldEff_Splash`) calls `CreateSpriteAtEnd`, which fatal-asserts when the pool is full.
- **Fix**
  Set `OW_OBJECT_VANILLA_SHADOWS` to `TRUE` in `include/config/overworld.h` so shadows are vanilla (jump-only), halving per-object sprite use and freeing headroom for rain/FX.
- **Status:** Known bugs - Patched

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
  Keep truncated arrays (restore costs ~768 bytes; SB1 only has ~108 free). Harden `tv.c` for no mix/scratch region: safe show scan, gate OOB mix compaction, no-op scratch-slot builders; gate mix helpers at entry; use `NUM_TV_AIRABLE_SLOTS` for all airable loops (includes slot 4 when truncated).
- **Status:** Known bugs - Patched  
  *(Commit `f69d6d14` initial fix; follow-up root harden in `tv.c` — also covers Petalburg PC / any post-Mauville indoor warp)*

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

### Field HM animation shows the wrong party Pokémon
- **Steps to reproduce**
  1. Have a party where one mon knows an HM (e.g. Surf / Cut / Rock Smash) but is not the lead.
  2. Use that field move from the overworld prompt (not necessarily via the party menu).
  3. Watch the field-move animation / “X used …!” text.
- **Expected Outcome**
  The Pokémon that knows the move is shown and named. If nobody knows it, lead fallback is intentional so an HM slave is not required.
- **Root cause**
  Overworld prompts (especially Surf / Waterfall / Dive in `field_control_avatar.c`) always used `GetFirstNonFaintedPartyIndex()` (the lead). Cut/Rock Smash had a partial script-side fix; Surf never selected the HM user. Dive also overwrote the show-mon party index with a species id after `checkfieldmove`.
- **Fix**
  Shared `GetPartyIndexForFieldMove` (knows move, else lead fallback) used by `checkfieldmove` and water-interaction setup; Surf script aligned; Dive species overwrite removed.
- **Status:** Known bugs - Patched

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
