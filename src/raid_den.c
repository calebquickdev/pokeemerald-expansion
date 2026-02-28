#include "global.h"
#include "main.h"
#include "raid_den.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "battle.h"
#include "battle_ai_util.h"
#include "battle_gimmick.h"
#include "battle_scripts.h"
#include "battle_setup.h"
#include "battle_transition.h"
#include "overworld.h"
#include "pokemon.h"
#include "random.h"
#include "script.h"
#include "constants/battle.h"
#include "constants/event_objects.h"
#include "gpu_regs.h"
#include "scanline_effect.h"
#include "task.h"
#include "sprite.h"
#include "palette.h"
#include "bg.h"
#include "window.h"
#include "text.h"
#include "menu.h"
#include "string_util.h"
#include "decompress.h"
#include "battle_gfx_sfx_util.h"
#include "pokemon_icon.h"
#include "party_menu.h"
#include "constants/rgb.h"
#include "constants/party_menu.h"

EWRAM_DATA u8 gRaidCurrentStarRating = 0;

struct LobbyState
{
    u8    denId;
    u8    selectedSlot;
    u8    bossSpriteId;
    u8    iconSpriteId;
    u8    menuWindowId;
    u8    infoWindowId;
    bool8 returnedFromParty;
};

static EWRAM_DATA struct LobbyState sLobbyState = {0};

static void Task_LobbyFadeIn(u8 taskId);
static void Task_LobbyRenderLeft(u8 taskId);
static void Task_LobbyRenderRight(u8 taskId);
static void Task_LobbyMain(u8 taskId);
static void Task_LobbyFadeOut(u8 taskId);
static void Task_LobbyChangePokemon(u8 taskId);
static void Task_LobbyChangePokemon_WaitFade(u8 taskId);
static void Task_LobbyInputLoop(u8 taskId);
static void Task_LobbyInviteOthers(u8 taskId);
static void Task_LobbyInviteOthers_WaitDismiss(u8 taskId);
static void Task_LobbyStartRaid(u8 taskId);
static void Task_LobbyStartRaid_WaitFade(u8 taskId);
static void Task_LobbyQuit(u8 taskId);
static void Task_LobbyQuit_WaitFade(u8 taskId);

static const u8 sText_InviteOthers[]     = _("Invite Others");
static const u8 sText_DontInviteOthers[] = _("Don't Invite Others");
static const u8 sText_ChangePokemon[]    = _("Change Pokemon");
static const u8 sText_Quit[]             = _("Quit");
static const u8 sText_InviteWIP[]        = _("This feature is not complete yet.");

static const struct MenuAction sLobbyMenuActions[] = {
    { sText_InviteOthers,     { .void_u8 = Task_LobbyInviteOthers  } },
    { sText_DontInviteOthers, { .void_u8 = Task_LobbyStartRaid     } },
    { sText_ChangePokemon,    { .void_u8 = Task_LobbyChangePokemon  } },
    { sText_Quit,             { .void_u8 = Task_LobbyQuit           } },
};

static const struct {
    u8 mapGroup;
    u8 mapNum;
    u8 localId;
    u8 denId;
} sDenObjectTable[] = {
    { MAP_GROUP(LITTLEROOT_TOWN), MAP_NUM(LITTLEROOT_TOWN), 9, 0 },
};

static u8 BstToStarRating(u32 bst)
{
    if (bst <= 299) return 1;
    if (bst <= 460) return 2;
    if (bst <= 494) return 3;
    if (bst <= 549) return 4;
    return 5;
}

u16 RollDynamaxDenPokemon(u8 denId)
{
    u16 species, picked = SPECIES_ZIGZAGOON;
    u16 count = 0, target, i;
    bool8 isGmax = FALSE;

    for (species = 1; species < NUM_SPECIES; species++)
    {
        if (GetTotalBaseStat(species) == 0) continue;
        if (gSpeciesInfo[species].isGigantamax) continue;
        if (GET_BASE_SPECIES_ID(species) != species) continue;
        count++;
    }

    if (count > 0)
    {
        target = (u16)(Random() % count);
        i = 0;
        for (species = 1; species < NUM_SPECIES; species++)
        {
            if (GetTotalBaseStat(species) == 0) continue;
            if (gSpeciesInfo[species].isGigantamax) continue;
            if (GET_BASE_SPECIES_ID(species) != species) continue;
            if (i++ == target)
            {
                picked = species;
                break;
            }
        }
    }

    if ((Random() % 10) == 0)
    {
        for (species = 1; species < NUM_SPECIES; species++)
        {
            if (gSpeciesInfo[species].isGigantamax && GET_BASE_SPECIES_ID(species) == picked)
            {
                picked = species;
                isGmax = TRUE;
                break;
            }
        }
    }

    gSaveBlock2Ptr->dynamaxDens[denId].species    = picked;
    gSaveBlock2Ptr->dynamaxDens[denId].isGmax     = isGmax;
    gSaveBlock2Ptr->dynamaxDens[denId].starRating = isGmax ? 5 : BstToStarRating(GetTotalBaseStat(picked));
    return picked;
}

void UpdateDynamaxDens(u16 daysSince)
{
    u8 i;
    (void)daysSince;

    for (i = 0; i < MAX_DYNAMAX_DENS; i++)
    {
        RollDynamaxDenPokemon(i);
    }
}

void SetupDynamaxDenObjects(void)
{
    u8 i;
    u8 mapGroup = gSaveBlock1Ptr->location.mapGroup;
    u8 mapNum = gSaveBlock1Ptr->location.mapNum;
    u16 graphicsId;

    for (i = 0; i < ARRAY_COUNT(sDenObjectTable); i++)
    {
        if (sDenObjectTable[i].mapGroup != mapGroup || sDenObjectTable[i].mapNum != mapNum)
            continue;

        graphicsId = FlagGet(FLAG_DAILY_DEN_RAIDED(sDenObjectTable[i].denId))
                     ? OBJ_EVENT_GFX_RAID_DEN_INACTIVE
                     : OBJ_EVENT_GFX_RAID_DEN_ACTIVE;

        ObjectEventSetGraphicsIdByLocalIdAndMap(
            sDenObjectTable[i].localId,
            mapNum,
            mapGroup,
            graphicsId
        );
    }
}

void ActivateDynamaxDen(void)
{
    u8 denId = (u8)gSpecialVar_0x8000;
    FlagClear(FLAG_DAILY_DEN_RAIDED(denId));
    RollDynamaxDenPokemon(denId);
}

static const u8 sStarLevelMin[6] = {0, 15, 25, 35, 45, 55};
static const u8 sStarLevelMax[6] = {0, 20, 30, 40, 50, 60};

static void SetupRaidBossParty(u8 denId)
{
    u16 species = gSaveBlock2Ptr->dynamaxDens[denId].species;
    u8 stars    = gSaveBlock2Ptr->dynamaxDens[denId].starRating;
    u8 minLv, maxLv, level;
    u32 maxHp;

    if (stars == 0 || stars > 5)
        stars = 1;
    gRaidCurrentStarRating = stars;
    if (species == 0)
        species = SPECIES_ZIGZAGOON;

    minLv = sStarLevelMin[stars];
    maxLv = sStarLevelMax[stars];
    level = (u8)(minLv + (Random() % (maxLv - minLv + 1)));

    ZeroEnemyPartyMons();
    CreateMon(&gEnemyParty[0], species, level, 31, FALSE, 0, OT_ID_RANDOM_NO_SHINY, 0);

    maxHp = GetMonData(&gEnemyParty[0], MON_DATA_MAX_HP, NULL) * 3;
    SetMonData(&gEnemyParty[0], MON_DATA_MAX_HP, &maxHp);
    SetMonData(&gEnemyParty[0], MON_DATA_HP, &maxHp);

    CreateMon(&gPlayerParty[3], SPECIES_SCEPTILE, level, 31, FALSE, 0, OT_ID_PLAYER_ID, 0);
    CreateMon(&gPlayerParty[4], SPECIES_BLAZIKEN, level, 31, FALSE, 0, OT_ID_PLAYER_ID, 0);
}

void TryAdvanceRaidRotation(void)
{
    static const u8 sRaidRotation[] = {0, 2, 3};
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sRaidRotation); i++)
    {
        if (GetActiveGimmick(sRaidRotation[i]) == GIMMICK_DYNAMAX)
            return;
    }

    switch (gBattleStruct->raid.dynamaxEnergy)
    {
        case 0:  gBattleStruct->raid.dynamaxEnergy = 2; break;
        case 2:  gBattleStruct->raid.dynamaxEnergy = 3; break;
        default:
        case 3:  gBattleStruct->raid.dynamaxEnergy = 0; break;
    }
}

void DoRaidBattle(void)
{
    u8 denId = (u8)gSpecialVar_0x8000;
    SetupRaidBossParty(denId);
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_ReturnToFieldContinueScriptPlayMapMusic;
    gBattleTypeFlags = BATTLE_TYPE_DOUBLE | BATTLE_TYPE_RAID;
    CreateBattleStartTask(B_TRANSITION_BLUR, 0);
    ScriptContext_Stop();
}

static const struct BgTemplate sLobbyBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
    {
        .bg = 1,
        .charBaseIndex = 2,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0,
    },
};

static const struct WindowTemplate sLobbyWindowTemplates[] = {
    [0] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 15,
        .height = 20,
        .paletteNum = 1,
        .baseBlock = 1,
    },
    [1] = {
        .bg = 0,
        .tilemapLeft = 15,
        .tilemapTop = 0,
        .width = 15,
        .height = 20,
        .paletteNum = 2,
        .baseBlock = 301,
    },
    DUMMY_WIN_TEMPLATE
};

static void VBlankCB_Lobby(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void MainCB2_Lobby(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

void CB2_DenLobbyScreen(void)
{
    if (sLobbyState.returnedFromParty)
    {
        // GetCursorSelectionMonId must be called before ResetSpriteData/ResetTasks clear party menu context.
        u8 selected = GetCursorSelectionMonId();
        if (selected < PARTY_SIZE)
            sLobbyState.selectedSlot = selected;
        sLobbyState.returnedFromParty = FALSE;
    }
    else
    {
        sLobbyState.denId        = (u8)gSpecialVar_0x8000;
        sLobbyState.selectedSlot = 0;
    }

    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();

    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sLobbyBgTemplates, ARRAY_COUNT(sLobbyBgTemplates));
    InitWindows(sLobbyWindowTemplates);
    DeactivateAllTextPrinters();
    sLobbyState.menuWindowId = 1;
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP | DISPCNT_BG0_ON);
    ShowBg(0);

    FillPalette(RGB(31, 16, 0), BG_PLTT_ID(1) + 1, PLTT_SIZEOF(1));
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);

    FillPalette(RGB(28, 24, 20), BG_PLTT_ID(2) + 1, PLTT_SIZEOF(1));
    FillWindowPixelBuffer(1, PIXEL_FILL(1));
    PutWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_FULL);

    BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    EnableInterrupts(1);
    SetVBlankCallback(VBlankCB_Lobby);
    SetMainCallback2(MainCB2_Lobby);
    CreateTask(Task_LobbyFadeIn, 0);
}

void OpenDenLobbyScreen(void)
{
    SetMainCallback2(CB2_DenLobbyScreen);
}

static void Task_LobbyFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_LobbyRenderLeft;
}

static void Task_LobbyRenderLeft(u8 taskId)
{
    switch (gTasks[taskId].data[0])
    {
    case 0:
        {
            u16 species = gSaveBlock2Ptr->dynamaxDens[sLobbyState.denId].species;
            AllocateMonSpritesGfx();
            HandleLoadSpecialPokePic(TRUE,
                gMonSpritesGfxPtr->spritesGfx[B_POSITION_OPPONENT_LEFT],
                species, 0);
            gTasks[taskId].data[0] = 1;
        }
        break;
    case 1:
        {
            u16 species = gSaveBlock2Ptr->dynamaxDens[sLobbyState.denId].species;
            u8 palNum;
            if (IsDma3ManagerBusyWithBgCopy())
                break;
            LoadCompressedSpritePaletteWithTag(
                GetMonSpritePalFromSpecies(species, FALSE, FALSE), species);
            SetMultiuseSpriteTemplateToPokemon(species, B_POSITION_OPPONENT_LEFT);
            sLobbyState.bossSpriteId = CreateSprite(&gMultiuseSpriteTemplate, 60, 72, 1);
            palNum = gSprites[sLobbyState.bossSpriteId].oam.paletteNum;
            FillPalette(RGB_BLACK, OBJ_PLTT_ID(palNum) + 1, PLTT_SIZE_4BPP - 2);
            gTasks[taskId].data[0] = 2;
        }
        break;
    case 2:
        {
            u8 stars = gSaveBlock2Ptr->dynamaxDens[sLobbyState.denId].starRating;
            static u8 sStarText[12];
            static const u8 sStarColor[] = {0, 1, 2};
            u8 i;
            u8 *ptr = sStarText;
            // CHAR_EXCL_MARK (0xAB) used as star placeholder — no ★ in GBA font
            for (i = 0; i < stars && i < 5; i++)
                *ptr++ = 0xAB;
            *ptr = EOS;
            AddTextPrinterParameterized4(0, FONT_NORMAL, 8, 8, 0, 0, sStarColor, TEXT_SKIP_DRAW, sStarText);
            CopyWindowToVram(0, COPYWIN_GFX);
        }
        gTasks[taskId].func = Task_LobbyRenderRight;
        break;
    }
}

static void Task_LobbyRenderRight(u8 taskId)
{
    u8  slot = sLobbyState.selectedSlot;
    u16 species     = GetMonData(&gPlayerParty[slot], MON_DATA_SPECIES, NULL);
    u32 personality = GetMonData(&gPlayerParty[slot], MON_DATA_PERSONALITY, NULL);
    static const u8 sNameColor[] = {0, 1, 2};

    AddTextPrinterParameterized4(1, FONT_NORMAL, 4, 4, 0, 0, sNameColor, TEXT_SKIP_DRAW, gSaveBlock2Ptr->playerName);
    CopyWindowToVram(1, COPYWIN_GFX);

    LoadMonIconPalette(species);
    sLobbyState.iconSpriteId = CreateMonIcon(species, SpriteCB_MonIcon, 200, 72, 4, personality);

    gTasks[taskId].func = Task_LobbyMain;
}

static void Task_LobbyMain(u8 taskId)
{
    SetStandardWindowBorderStyle(sLobbyState.menuWindowId, FALSE);
    PrintMenuTable(sLobbyState.menuWindowId,
                   ARRAY_COUNT(sLobbyMenuActions), sLobbyMenuActions);
    InitMenuInUpperLeftCornerNormal(sLobbyState.menuWindowId,
                                   ARRAY_COUNT(sLobbyMenuActions), 0);
    CopyWindowToVram(sLobbyState.menuWindowId, COPYWIN_FULL);
    gTasks[taskId].func = Task_LobbyInputLoop;
}

static void Task_LobbyInputLoop(u8 taskId)
{
    s8 selection = Menu_ProcessInputNoWrap();

    if (selection == MENU_NOTHING_CHOSEN)
        return;

    if (selection == MENU_B_PRESSED)
    {
        gTasks[taskId].func = Task_LobbyQuit;
        return;
    }

    gTasks[taskId].func = sLobbyMenuActions[selection].func.void_u8;
}

static void Task_LobbyInviteOthers(u8 taskId)
{
    static const u8 sWipColor[] = {0, 1, 2};
    FillWindowPixelBuffer(sLobbyState.menuWindowId, PIXEL_FILL(0));
    AddTextPrinterParameterized4(sLobbyState.menuWindowId, FONT_NORMAL,
                                 4, 4, 0, 0, sWipColor,
                                 TEXT_SKIP_DRAW, sText_InviteWIP);
    CopyWindowToVram(sLobbyState.menuWindowId, COPYWIN_FULL);
    gTasks[taskId].func = Task_LobbyInviteOthers_WaitDismiss;
}

static void Task_LobbyInviteOthers_WaitDismiss(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
        gTasks[taskId].func = Task_LobbyMain;
}

static void Task_LobbyStartRaid(u8 taskId)
{
    gSpecialVar_0x8000 = sLobbyState.denId;
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_LobbyStartRaid_WaitFade;
}

static void Task_LobbyStartRaid_WaitFade(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        FreeMonSpritesGfx();
        DoRaidBattle();
        DestroyTask(taskId);
    }
}

static void Task_LobbyQuit(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_LobbyQuit_WaitFade;
}

static void Task_LobbyQuit_WaitFade(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        FreeMonSpritesGfx();
        DestroyTask(taskId);
        SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
    }
}

static void Task_LobbyFadeOut(u8 taskId)
{
    if (!gPaletteFade.active)
        DestroyTask(taskId);
}

static void Task_LobbyChangePokemon(u8 taskId)
{
    u16 species = GetMonData(&gPlayerParty[sLobbyState.selectedSlot], MON_DATA_SPECIES, NULL);
    FreeAndDestroyMonIconSprite(&gSprites[sLobbyState.iconSpriteId]);
    FreeMonIconPalette(species);

    sLobbyState.returnedFromParty = TRUE;

    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_LobbyChangePokemon_WaitFade;
}

static void Task_LobbyChangePokemon_WaitFade(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        ChooseMonForTradingBoard(PARTY_MENU_TYPE_FIELD, CB2_DenLobbyScreen);
        DestroyTask(taskId);
    }
}

bool32 TryRaidStormTick(void)
{
    if (gBattleTurnCounter >= 10)
    {
        gBattleOutcome = B_OUTCOME_PLAYER_TELEPORTED;
        BattleScriptExecute(BattleScript_RaidStormExpired);
        return TRUE;
    }
    gBattleCommunication[MULTISTRING_CHOOSER] = (gBattleTurnCounter == 9) ? 1 : 0;
    BattleScriptExecute(BattleScript_RaidStormMessage);
    return TRUE;
}

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
            gBattleMons[battler].hp = gBattleMons[battler].maxHP;
            gAbsentBattlerFlags &= ~(1u << battler);
        }
    }
}
