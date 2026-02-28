---
phase: 06-den-lobby-screen
plan: 04
subsystem: ui
tags: [gba, pokemon-icon, party-menu, sprite, palette, text-printer]

requires:
  - phase: 06-03
    provides: Task_LobbyRenderLeft chain point, sLobbyState struct with iconSpriteId/selectedSlot/returnedFromParty, right panel window 1

provides:
  - Task_LobbyRenderRight: player name text in right panel + animated party icon sprite
  - Task_LobbyChangePokemon + WaitFade: fade-out -> ChooseMonForTradingBoard -> CB2_DenLobbyScreen re-entry
  - CB2_DenLobbyScreen re-entry path: GetCursorSelectionMonId before reset, selectedSlot update

affects:
  - 06-05: wires menu to Task_LobbyChangePokemon; menu invocation point exists after this plan

tech-stack:
  added: []
  patterns:
    - "CreateMonIcon(species, SpriteCB_MonIcon, x, y, subpriority, personality) for animated party icons"
    - "GetCursorSelectionMonId() called before ResetSpriteData/ResetTasks in re-entry path"
    - "returnedFromParty flag guards denId re-read vs selectedSlot update on CB2 re-entry"

key-files:
  created: []
  modified:
    - src/raid_den.c

key-decisions:
  - "AddTextPrinterParameterized4 color arg must be a const u8[] — plan code had nameBuf as color and NULL as string (fixed)"
  - "selectedSlot initialized to 0 on fresh entry; GetCursorSelectionMonId() only called on returnedFromParty re-entry"

patterns-established:
  - "Party icon lifecycle: LoadMonIconPalette -> CreateMonIcon on entry; FreeAndDestroyMonIconSprite + FreeMonIconPalette on exit"

duration: ~10min
completed: 2026-02-27
---

# Phase 6 Plan 04: Right Panel Render and Change Pokémon Flow Summary

**Player name + animated party icon rendered in right panel via AddTextPrinterParameterized4 and CreateMonIcon; Change Pokémon round-trip via ChooseMonForTradingBoard with GetCursorSelectionMonId re-entry**

## Performance

- **Duration:** ~10 min
- **Started:** 2026-02-27T22:57:25Z
- **Completed:** 2026-02-27T23:15:00Z
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments
- `Task_LobbyRenderRight` renders player name (right panel window 1) and creates animated party icon at (200,72) for `sLobbyState.selectedSlot`
- `Task_LobbyChangePokemon` destroys the icon sprite, sets `returnedFromParty`, fades out, then opens party screen via `ChooseMonForTradingBoard(PARTY_MENU_TYPE_FIELD, CB2_DenLobbyScreen)`
- `CB2_DenLobbyScreen` re-entry path calls `GetCursorSelectionMonId()` before `ResetSpriteData()`/`ResetTasks()`, updates `selectedSlot` if valid, clears the flag
- `Task_LobbyRenderLeft` state 2 now chains to `Task_LobbyRenderRight` before `Task_LobbyMain`

## Task Commits

1. **Tasks 1+2: Right panel render and Change Pokemon flow** - `2fcc85927b` (feat)

**Plan metadata:** (pending — committed with tasks)

## Files Created/Modified
- `src/raid_den.c` - Task_LobbyRenderRight, Task_LobbyChangePokemon, Task_LobbyChangePokemon_WaitFade, CB2_DenLobbyScreen re-entry update

## Decisions Made
- Player name passed directly as 9th arg (`str`) to `AddTextPrinterParameterized4` with a static `sNameColor[]` array as the 7th arg; no intermediate `StringCopy` needed since `gSaveBlock2Ptr->playerName` is already an encoded GBA string
- `selectedSlot` set to 0 only on fresh (non-returnedFromParty) entry to preserve the slot across the party screen round-trip

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed inverted arguments to AddTextPrinterParameterized4**
- **Found during:** Task 1 (render player name)
- **Issue:** Plan code passed `nameBuf` (a string buffer) as the `color` arg (7th) and `NULL` as the `str` arg (9th). Signature is `(windowId, fontId, x, y, letterSpacing, lineSpacing, color, speed, str)`.
- **Fix:** Used `static const u8 sNameColor[] = {0, 1, 2}` for the color arg; passed `gSaveBlock2Ptr->playerName` directly as the string arg; removed unnecessary `StringCopy` + `nameBuf`.
- **Files modified:** src/raid_den.c
- **Verification:** ROM builds clean with correct argument types
- **Committed in:** 2fcc85927b (task commit)

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Essential correctness fix — incorrect args would display garbage or crash. No scope creep.

## Issues Encountered
- Git `--amend` unavailable due to undiagnosed option parsing conflict in WSL git environment; committed via script with simple message instead. Code and functionality unaffected.

## Next Phase Readiness
- `Task_LobbyChangePokemon` stub ready for Plan 06-05 to invoke from the menu
- `Task_LobbyRenderRight` chains correctly to `Task_LobbyMain` for Plan 06-05's menu logic
- Both `iconSpriteId` and `selectedSlot` lifecycle fully managed; Plan 06-05 can call `Task_LobbyChangePokemon` directly

---
*Phase: 06-den-lobby-screen*
*Completed: 2026-02-27*
