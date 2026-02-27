# Architecture

**Analysis Date:** 2025-02-27

## Pattern Overview

**Overall:** Single-threaded event-driven architecture with callback-based state machines and a cooperative task system. No OS; direct GBA hardware control.

**Key Characteristics:**
- Single main loop (`AgbMainLoop`) drives all game logic per frame
- Two-tier callback system (`gMain.callback1`, `gMain.callback2`) for screen/state transitions
- Cooperative task system (16 tasks, priority-ordered) for multi-frame operations
- Battle engine uses bytecode scripts (`gBattlescriptCurrInstr`) dispatched via opcode table
- Map scripts use bytecode or native C functions via `ScriptContext`
- Interrupt-driven VBlank/HBlank for timing and DMA

## Layers

**Hardware / Bootstrap:**
- Purpose: Reset, memory init, interrupt setup, jump to C entry
- Location: `src/crt0.s`, `src/rom_header.s`
- Contains: `Init`, `IntrMain`, `AgbMain` entry
- Depends on: GBA hardware registers (`include/gba/`)
- Used by: All game code

**Main Loop / Callbacks:**
- Purpose: Frame driver, input, link sync, callback dispatch
- Location: `src/main.c`
- Contains: `AgbMain`, `AgbMainLoop`, `UpdateLinkAndCallCallbacks`, `CallCallbacks`, `SetMainCallback2`
- Depends on: `gMain` struct (`include/main.h`), link layer
- Used by: Every screen/state sets itself as `callback2`

**Task System:**
- Purpose: Long-running multi-frame processes (animations, loading, menus)
- Location: `src/task.c`, `include/task.h`
- Contains: `CreateTask`, `DestroyTask`, `RunTasks`, `gTasks[16]`
- Depends on: Nothing (standalone)
- Used by: Overworld, battle, menus, field effects, link, etc.

**Overworld Engine:**
- Purpose: Map rendering, player movement, object events, map scripts
- Location: `src/overworld.c`, `src/field_effect.c`, `src/field_screen_effect.c`
- Contains: `CB2_Overworld`, `CB1_Overworld`, `OverworldBasic`, `CB2_LoadMap`, `CB2_ContinueSavedGame`
- Depends on: Script context, sprites, camera, tilemaps
- Used by: Field gameplay, warps, fly, dig, etc.

**Script System:**
- Purpose: Map event scripts (bytecode + native)
- Location: `src/script.c`, `data/scripts/*.inc`, `asm/macros/event.inc`
- Contains: `ScriptContext_RunScript`, `ScriptContext_Init`, `gScriptCmdTable`
- Depends on: Event data, flags, vars
- Used by: Overworld, map scripts, NPCs

**Battle Engine:**
- Purpose: Turn-based combat, damage, status, animations
- Location: `src/battle_main.c`, `src/battle_script_commands.c`, `src/battle_controllers.c`
- Contains: `BattleMainCB2`, `CB2_InitBattle`, battle script execution, controller dispatch
- Depends on: Battle scripts (`asm/macros/battle_script.inc`), AI, animations
- Used by: Wild encounters, trainer battles, link battles

**Battle Controllers:**
- Purpose: Per-battler input/AI and action buffer
- Location: `src/battle_controller_*.c`, `include/battle_controllers.h`
- Contains: `gBattlerControllerFuncs`, player/opponent/safari/link/Wally/recorded controllers
- Depends on: Battle state, link buffers
- Used by: `battle_main.c` via `gBattlerControllerFuncs[battler]()`

## Data Flow

**Frame Loop (AgbMainLoop):**

1. `ReadKeys()` — update `gMain.heldKeys`, `gMain.newKeys`
2. Soft-reset check (A+B+Start+Select)
3. Link sync: `HandleLinkConnection()` — if TRUE, skip callbacks (link transfer)
4. `UpdateLinkAndCallCallbacks()` — `CallCallbacks()` when link allows
5. `CallCallbacks()` — `gMain.callback1()`, then `gMain.callback2()`
6. `PlayTimeCounter_Update()`, `MapMusicMain()`
7. `WaitForVBlank()` — block until next frame

**Overworld Frame (CB2_Overworld):**

1. `OverworldBasic()`: `ScriptContext_RunScript()` → `RunTasks()` → `AnimateSprites()` → `CameraUpdate()` → `UpdateCameraPanning()` → `BuildOamBuffer()` → `UpdatePaletteFade()` → `DoScheduledBgTilemapCopiesToVram()`
2. `CB1_Overworld` runs on key input: `ProcessPlayerFieldInput` or `PlayerStep`

**Battle Frame (BattleMainCB2):**

1. `RunTasks()`
2. Controller dispatch: `gBattlerControllerFuncs[battler]()` for each battler
3. Battle script execution when buffer ready: `gBattleScriptingCommandsTable[opcode]()`
4. `BattleMain` state machine: intro → action selection → turn execution → end

**State Transitions:**

- `SetMainCallback2(callback)` — sets `gMain.callback2` and resets `gMain.state` to 0
- Callbacks use `gMain.state` for multi-step init (e.g. load → fade → run)
- Return to overworld: `SetMainCallback2(CB2_Overworld)` with `SetMainCallback1(CB1_Overworld)`
- Return from battle: `SetMainCallback2(gMain.savedCallback)` (e.g. `CB2_LoadMap`)

## Key Abstractions

**Main struct (`include/main.h`):**
- `callback1`, `callback2` — main frame callbacks
- `savedCallback` — restore point (e.g. after battle)
- `vblankCallback`, `hblankCallback`, `vcountCallback`, `serialCallback`
- `state` — sub-step for multi-step callbacks
- `heldKeys`, `newKeys`, `newAndRepeatedKeys`

**Task (`include/task.h`):**
- `TaskFunc func` — `void (*)(u8 taskId)`
- `data[16]` — s16 array for task-local state
- Priority-ordered linked list; lower priority value runs first

**Battle Script:**
- Bytecode in `asm/macros/battle_script.inc` (e.g. `.byte 0x5` = damagecalc)
- `gBattlescriptCurrInstr` — current instruction pointer
- `gBattleScriptingCommandsTable[opcode]()` — C handlers in `src/battle_script_commands.c`
- Macros like `accuracycheck`, `damagecalc`, `attackanimation` emit opcodes + args

**Script Context (`include/script.h`):**
- `SCRIPT_MODE_BYTECODE` or `SCRIPT_MODE_NATIVE`
- `scriptPtr` / `nativePtr`, `stack`, `data`
- `gScriptCmdTable` — map script command handlers

## Entry Points

**ROM Entry:**
- Location: `ld_script_modern.ld` — `ENTRY(Start)`; `src/rom_header.s` + `src/crt0.s`
- Triggers: GBA boot
- Responsibilities: Init stacks, `InitializeWorkingMemory`, set `INTR_VECTOR`, branch to `AgbMain`

**AgbMain:**
- Location: `src/main.c`
- Triggers: Called from `crt0.s` after init
- Responsibilities: GPU init, keys, interrupts, sound, RTC, flash check, heap, `SetMainCallback2(gInitialMainCB2)`, enter `AgbMainLoop`

**gInitialMainCB2:**
- Location: `ld_script_modern.ld` — `gInitialMainCB2 = CB2_InitCopyrightScreenAfterBootup`
- Test build: `gInitialMainCB2 = CB2_TestRunner` (`ld_script_test.ld`)
- Flow: Copyright → Title → Main Menu → Continue/New → `CB2_ContinueSavedGame` or `CB2_NewGame` → `CB2_LoadMap` → `CB2_Overworld`

**CB2_LoadMap / CB2_ContinueSavedGame:**
- Location: `src/overworld.c`
- Triggers: Warp, fly, continue game, return from battle
- Responsibilities: `DoMapLoadLoop`, `SetFieldVBlankCallback`, `SetMainCallback1(CB1_Overworld)`, `SetMainCallback2(CB2_Overworld)`

**CB2_InitBattle:**
- Location: `src/battle_main.c`
- Triggers: Wild encounter, trainer battle, link battle
- Responsibilities: Init battle vars, controllers, sprites; `SetMainCallback2(BattleMainCB2)`, `gMain.callback1 = BattleMainCB1`

## Game Loop

```
AgbMainLoop (infinite)
  ├── ReadKeys
  ├── [Soft reset check]
  ├── HandleLinkConnection
  │     └── if FALSE: CallCallbacks
  │           ├── gMain.callback1()   e.g. CB1_Overworld, BattleMainCB1
  │           └── gMain.callback2()  e.g. CB2_Overworld, BattleMainCB2
  ├── PlayTimeCounter_Update
  ├── MapMusicMain
  └── WaitForVBlank
```

VBlank interrupt (`VBlankIntr` in `main.c`): RfuVSync/LinkVSync, vblank callbacks, `CopyBufferedValuesToGpuRegs`, `ProcessDma3Requests`, `m4aSoundMain`, `TryReceiveLinkBattleData`, `AdvanceRandom`, `UpdateWirelessStatusIndicatorSprite`.

## Battle Engine Architecture

**Phases:**
1. Init: `CB2_InitBattle` → `CB2_InitBattleInternal` → battle setup, controllers, sprites
2. Intro: `DoBattleIntro`, send-out animations
3. Action selection: `HandleTurnActionSelectionState` — each controller fills `gChosenActionByBattler`
4. Turn execution: `RunTurnActionsFunctions` — set turn order, run actions, execute battle scripts
5. End turn: `HandleEndTurn_*` — win/loss/flee, evolution, return to overworld

**Battle Script Execution:**
- `gBattlescriptCurrInstr` points to bytecode
- Opcode byte indexes `gBattleScriptingCommandsTable`; handler advances `gBattlescriptCurrInstr`
- Commands: `attackcanceler`, `accuracycheck`, `damagecalc`, `attackanimation`, `printstring`, etc.
- `BattleScriptExecute(script)` sets pointer and runs until script yields or ends

**Controllers:**
- `gBattlerControllerFuncs[battler]` — one per battler (player, opponent, safari, link, etc.)
- Each controller: read input/AI → write to link buffer or local `gChosenActionByBattler`
- `BattleControllerDummy` for inactive battlers

**Key Files:**
- `src/battle_main.c` — main loop, turn logic, script dispatch
- `src/battle_script_commands.c` — script command implementations
- `src/battle_controllers.c` — controller init and link tasks
- `src/battle_controller_player.c`, `battle_controller_opponent.c`, etc.
- `asm/macros/battle_script.inc` — script macro definitions

## Task / Callback System

**Tasks:**
- `CreateTask(func, priority)` — allocates slot, inserts by priority
- `RunTasks()` — walks active list, calls `func(taskId)` for each
- `DestroyTask(taskId)` — marks inactive, unlinks
- Tasks store state in `gTasks[id].data[0..15]`
- `SetTaskFuncWithFollowupFunc` / `SwitchTaskToFollowupFunc` — chain to next handler

**Callbacks:**
- `SetMainCallback2(cb)` — set primary screen callback, reset `gMain.state`
- `gMain.savedCallback` — stored before battle/menus; restored on exit
- `SetMainCallback1(cb)` — optional; runs before callback2 (e.g. `CB1_Overworld` for input)
- `SetVBlankCallback`, `SetHBlankCallback`, `SetVCountCallback` — interrupt-time hooks

**Typical Screen Pattern:**
```c
void CB2_MyScreen(void) {
    switch (gMain.state) {
    case 0: /* load resources */ gMain.state++; break;
    case 1: /* fade in */ gMain.state++; break;
    case 2: SetMainCallback2(CB2_MyScreenUpdate); break;
    }
}
void CB2_MyScreenUpdate(void) {
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    /* handle input, exit via SetMainCallback2(exitCallback) */
}
```

## Error Handling

**Strategy:** No exceptions. Functions return `bool8`/`bool32` or use global flags. Critical paths check and branch.

**Patterns:**
- `HandleLinkConnection()` returns TRUE to skip callbacks when link is busy
- Battle script commands advance or jump; no try/catch
- Flash/save errors: `SetMainCallback2(CB2_FlashNotDetectedScreen)` or similar

## Cross-Cutting Concerns

**Logging:** `#ifndef NDEBUG` — `MgbaOpen()` / `AGBPrintfInit()` for debug builds; no runtime logging in release.

**Validation:** Config headers (`include/config/`) define limits; data tables must match. No runtime schema validation.

**Authentication:** None; single-player and local link only.

---

*Architecture analysis: 2025-02-27*
