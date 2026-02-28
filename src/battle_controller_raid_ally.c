#include "global.h"
#include "battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_gfx_sfx_util.h"
#include "battle_message.h"
#include "battle_interface.h"
#include "battle_setup.h"
#include "battle_tower.h"
#include "battle_z_move.h"
#include "bg.h"
#include "data.h"
#include "decompress.h"
#include "item_use.h"
#include "link.h"
#include "main.h"
#include "m4a.h"
#include "palette.h"
#include "party_menu.h"
#include "pokeball.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "reshow_battle_screen.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "util.h"
#include "window.h"
#include "constants/battle_anim.h"
#include "constants/battle_partner.h"
#include "constants/songs.h"
#include "constants/party_menu.h"
#include "constants/trainers.h"

static void RaidAllyHandleLoadMonSprite(u32 battler);
static void RaidAllyHandleFaintAnimation(u32 battler);
static void RaidAllyHandleSwitchInAnim(u32 battler);
static void RaidAllyHandleDrawTrainerPic(u32 battler);
static void RaidAllyHandleTrainerSlideBack(u32 battler);
static void RaidAllyHandleMoveAnimation(u32 battler);
static void RaidAllyHandlePrintString(u32 battler);
static void RaidAllyHandleChooseAction(u32 battler);
static void RaidAllyHandleChooseMove(u32 battler);
static void RaidAllyHandleChoosePokemon(u32 battler);
static void RaidAllyHandleHealthBarUpdate(u32 battler);
static void RaidAllyHandleIntroTrainerBallThrow(u32 battler);
static void RaidAllyHandleDrawPartyStatusSummary(u32 battler);
static void RaidAllyHandleBattleAnimation(u32 battler);
static void RaidAllyHandleEndLinkBattle(u32 battler);

static void RaidAllyBufferRunCommand(u32 battler);
static void RaidAllyBufferExecCompleted(u32 battler);
static void SwitchIn_WaitAndEnd(u32 battler);

static void (*const sRaidAllyBufferCommands[CONTROLLER_CMDS_COUNT])(u32 battler) =
{
    [CONTROLLER_GETMONDATA]               = BtlController_HandleGetMonData,
    [CONTROLLER_GETRAWMONDATA]            = BtlController_Empty,
    [CONTROLLER_SETMONDATA]               = BtlController_HandleSetMonData,
    [CONTROLLER_SETRAWMONDATA]            = BtlController_HandleSetRawMonData,
    [CONTROLLER_LOADMONSPRITE]            = RaidAllyHandleLoadMonSprite,
    [CONTROLLER_SWITCHINANIM]             = RaidAllyHandleSwitchInAnim,
    [CONTROLLER_RETURNMONTOBALL]          = BtlController_HandleReturnMonToBall,
    [CONTROLLER_DRAWTRAINERPIC]           = RaidAllyHandleDrawTrainerPic,
    [CONTROLLER_TRAINERSLIDE]             = BtlController_Empty,
    [CONTROLLER_TRAINERSLIDEBACK]         = RaidAllyHandleTrainerSlideBack,
    [CONTROLLER_FAINTANIMATION]           = RaidAllyHandleFaintAnimation,
    [CONTROLLER_PALETTEFADE]              = BtlController_Empty,
    [CONTROLLER_SUCCESSBALLTHROWANIM]     = BtlController_Empty,
    [CONTROLLER_BALLTHROWANIM]            = BtlController_Empty,
    [CONTROLLER_PAUSE]                    = BtlController_Empty,
    [CONTROLLER_MOVEANIMATION]            = RaidAllyHandleMoveAnimation,
    [CONTROLLER_PRINTSTRING]              = RaidAllyHandlePrintString,
    [CONTROLLER_PRINTSTRINGPLAYERONLY]    = BtlController_Empty,
    [CONTROLLER_CHOOSEACTION]             = RaidAllyHandleChooseAction,
    [CONTROLLER_YESNOBOX]                 = BtlController_Empty,
    [CONTROLLER_CHOOSEMOVE]               = RaidAllyHandleChooseMove,
    [CONTROLLER_OPENBAG]                  = BtlController_Empty,
    [CONTROLLER_CHOOSEPOKEMON]            = RaidAllyHandleChoosePokemon,
    [CONTROLLER_23]                       = BtlController_Empty,
    [CONTROLLER_HEALTHBARUPDATE]          = RaidAllyHandleHealthBarUpdate,
    [CONTROLLER_EXPUPDATE]                = PlayerHandleExpUpdate, // Partner's player gets experience the same way as the player.
    [CONTROLLER_STATUSICONUPDATE]         = BtlController_HandleStatusIconUpdate,
    [CONTROLLER_STATUSANIMATION]          = BtlController_HandleStatusAnimation,
    [CONTROLLER_STATUSXOR]                = BtlController_Empty,
    [CONTROLLER_DATATRANSFER]             = BtlController_Empty,
    [CONTROLLER_DMA3TRANSFER]             = BtlController_Empty,
    [CONTROLLER_PLAYBGM]                  = BtlController_Empty,
    [CONTROLLER_32]                       = BtlController_Empty,
    [CONTROLLER_TWORETURNVALUES]          = BtlController_Empty,
    [CONTROLLER_CHOSENMONRETURNVALUE]     = BtlController_Empty,
    [CONTROLLER_ONERETURNVALUE]           = BtlController_Empty,
    [CONTROLLER_ONERETURNVALUE_DUPLICATE] = BtlController_Empty,
    [CONTROLLER_HITANIMATION]             = BtlController_HandleHitAnimation,
    [CONTROLLER_CANTSWITCH]               = BtlController_Empty,
    [CONTROLLER_PLAYSE]                   = BtlController_HandlePlaySE,
    [CONTROLLER_PLAYFANFAREORBGM]         = BtlController_HandlePlayFanfareOrBGM,
    [CONTROLLER_FAINTINGCRY]              = BtlController_HandleFaintingCry,
    [CONTROLLER_INTROSLIDE]               = BtlController_HandleIntroSlide,
    [CONTROLLER_INTROTRAINERBALLTHROW]    = RaidAllyHandleIntroTrainerBallThrow,
    [CONTROLLER_DRAWPARTYSTATUSSUMMARY]   = RaidAllyHandleDrawPartyStatusSummary,
    [CONTROLLER_HIDEPARTYSTATUSSUMMARY]   = BtlController_HandleHidePartyStatusSummary,
    [CONTROLLER_ENDBOUNCE]                = BtlController_Empty,
    [CONTROLLER_SPRITEINVISIBILITY]       = BtlController_HandleSpriteInvisibility,
    [CONTROLLER_BATTLEANIMATION]          = RaidAllyHandleBattleAnimation,
    [CONTROLLER_LINKSTANDBYMSG]           = BtlController_Empty,
    [CONTROLLER_RESETACTIONMOVESELECTION] = BtlController_Empty,
    [CONTROLLER_ENDLINKBATTLE]            = RaidAllyHandleEndLinkBattle,
    [CONTROLLER_DEBUGMENU]                = BtlController_Empty,
    [CONTROLLER_TERMINATOR_NOP]           = BtlController_TerminatorNop
};

void SetControllerToRaidAlly(u32 battler)
{
    gBattlerControllerEndFuncs[battler] = RaidAllyBufferExecCompleted;
    gBattlerControllerFuncs[battler] = RaidAllyBufferRunCommand;
}

static void RaidAllyBufferRunCommand(u32 battler)
{
    if (gBattleControllerExecFlags & (1u << battler))
    {
        if (gBattleResources->bufferA[battler][0] < ARRAY_COUNT(sRaidAllyBufferCommands))
            sRaidAllyBufferCommands[gBattleResources->bufferA[battler][0]](battler);
        else
            RaidAllyBufferExecCompleted(battler);
    }
}

static void Intro_DelayAndEnd(u32 battler)
{
    if (--gBattleSpritesDataPtr->healthBoxesData[battler].introEndDelay == (u8)-1)
    {
        gBattleSpritesDataPtr->healthBoxesData[battler].introEndDelay = 0;
        BattleControllerComplete(battler);
    }
}

static void Intro_WaitForHealthbox(u32 battler)
{
    bool32 finished = FALSE;

    if (!IsDoubleBattle() || (IsDoubleBattle() && (gBattleTypeFlags & BATTLE_TYPE_MULTI)))
    {
        if (gSprites[gHealthboxSpriteIds[battler]].callback == SpriteCallbackDummy)
            finished = TRUE;
    }
    else
    {
        if (gSprites[gHealthboxSpriteIds[battler]].callback == SpriteCallbackDummy
            && gSprites[gHealthboxSpriteIds[BATTLE_PARTNER(battler)]].callback == SpriteCallbackDummy)
        {
            finished = TRUE;
        }
    }

    if (IsCryPlayingOrClearCrySongs())
        finished = FALSE;

    if (finished)
    {
        gBattleSpritesDataPtr->healthBoxesData[battler].introEndDelay = 3;
        gBattlerControllerFuncs[battler] = Intro_DelayAndEnd;
    }
}

// Also used by the link partner.
void Controller_RaidAllyShowIntroHealthbox(u32 battler)
{
    if (!gBattleSpritesDataPtr->healthBoxesData[battler].ballAnimActive
        && !gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(battler)].ballAnimActive
        && gSprites[gBattleControllerData[battler]].callback == SpriteCallbackDummy
        && gSprites[gBattlerSpriteIds[battler]].callback == SpriteCallbackDummy
        && ++gBattleSpritesDataPtr->healthBoxesData[battler].introEndDelay != 1)
    {
        gBattleSpritesDataPtr->healthBoxesData[battler].introEndDelay = 0;
        TryShinyAnimation(battler, &gPlayerParty[gBattlerPartyIndexes[battler]]);

        if (IsDoubleBattle() && !(gBattleTypeFlags & BATTLE_TYPE_MULTI))
        {
            DestroySprite(&gSprites[gBattleControllerData[BATTLE_PARTNER(battler)]]);
            UpdateHealthboxAttribute(gHealthboxSpriteIds[BATTLE_PARTNER(battler)], &gPlayerParty[gBattlerPartyIndexes[BATTLE_PARTNER(battler)]], HEALTHBOX_ALL);
            StartHealthboxSlideIn(BATTLE_PARTNER(battler));
            SetHealthboxSpriteVisible(gHealthboxSpriteIds[BATTLE_PARTNER(battler)]);
        }

        DestroySprite(&gSprites[gBattleControllerData[battler]]);
        UpdateHealthboxAttribute(gHealthboxSpriteIds[battler], &gPlayerParty[gBattlerPartyIndexes[battler]], HEALTHBOX_ALL);
        StartHealthboxSlideIn(battler);
        SetHealthboxSpriteVisible(gHealthboxSpriteIds[battler]);

        gBattleSpritesDataPtr->animationData->introAnimActive = FALSE;

        gBattlerControllerFuncs[battler] = Intro_WaitForHealthbox;
    }
}

static void WaitForMonAnimAfterLoad(u32 battler)
{
    if (gSprites[gBattlerSpriteIds[battler]].animEnded && gSprites[gBattlerSpriteIds[battler]].x2 == 0)
        RaidAllyBufferExecCompleted(battler);
}

static void SwitchIn_ShowSubstitute(u32 battler)
{
    if (gSprites[gHealthboxSpriteIds[battler]].callback == SpriteCallbackDummy)
    {
        CopyBattleSpriteInvisibility(battler);
        if (gBattleSpritesDataPtr->battlerData[battler].behindSubstitute)
            InitAndLaunchSpecialAnimation(battler, battler, battler, B_ANIM_MON_TO_SUBSTITUTE);

        gBattlerControllerFuncs[battler] = SwitchIn_WaitAndEnd;
    }
}

static void SwitchIn_WaitAndEnd(u32 battler)
{
    if (!gBattleSpritesDataPtr->healthBoxesData[battler].specialAnimActive
        && gSprites[gBattlerSpriteIds[battler]].callback == SpriteCallbackDummy)
    {
        RaidAllyBufferExecCompleted(battler);
    }
}

static void SwitchIn_ShowHealthbox(u32 battler)
{
    if (gBattleSpritesDataPtr->healthBoxesData[battler].finishedShinyMonAnim)
    {
        gBattleSpritesDataPtr->healthBoxesData[battler].triedShinyMonAnim = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[battler].finishedShinyMonAnim = FALSE;

        FreeSpriteTilesByTag(ANIM_TAG_GOLD_STARS);
        FreeSpritePaletteByTag(ANIM_TAG_GOLD_STARS);

        CreateTask(Task_PlayerController_RestoreBgmAfterCry, 10);
        HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[battler]], battler);
        StartSpriteAnim(&gSprites[gBattlerSpriteIds[battler]], 0);
        UpdateHealthboxAttribute(gHealthboxSpriteIds[battler], &gPlayerParty[gBattlerPartyIndexes[battler]], HEALTHBOX_ALL);
        StartHealthboxSlideIn(battler);
        SetHealthboxSpriteVisible(gHealthboxSpriteIds[battler]);

        gBattlerControllerFuncs[battler] = SwitchIn_ShowSubstitute;
    }
}

static void SwitchIn_TryShinyAnim(u32 battler)
{
    if (!gBattleSpritesDataPtr->healthBoxesData[battler].triedShinyMonAnim
        && !gBattleSpritesDataPtr->healthBoxesData[battler].ballAnimActive)
    {
        TryShinyAnimation(battler, &gPlayerParty[gBattlerPartyIndexes[battler]]);
    }

    if (gSprites[gBattleControllerData[battler]].callback == SpriteCallbackDummy
     && !gBattleSpritesDataPtr->healthBoxesData[battler].ballAnimActive)
    {
        DestroySprite(&gSprites[gBattleControllerData[battler]]);
        gBattlerControllerFuncs[battler] = SwitchIn_ShowHealthbox;
    }
}

static void RaidAllyBufferExecCompleted(u32 battler)
{
    gBattlerControllerFuncs[battler] = RaidAllyBufferRunCommand;
    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
    {
        u8 playerId = GetMultiplayerId();

        PrepareBufferDataTransferLink(battler, 2, 4, &playerId);
        gBattleResources->bufferA[battler][0] = CONTROLLER_TERMINATOR_NOP;
    }
    else
    {
        gBattleControllerExecFlags &= ~(1u << battler);
    }
}

static void RaidAllyHandleFaintAnimation(u32 battler)
{
    u8 iconIdx = battler - 2;
    if (gBattleStruct->raid.allyIconSpriteId[iconIdx] != MAX_SPRITES)
    {
        FreeAndDestroyMonIconSprite(&gSprites[gBattleStruct->raid.allyIconSpriteId[iconIdx]]);
        gBattleStruct->raid.allyIconSpriteId[iconIdx] = MAX_SPRITES;
    }
    BtlController_HandleFaintAnimation(battler);
}

static void RaidAllyHandleLoadMonSprite(u32 battler)
{
    struct Pokemon *party = GetBattlerParty(battler);
    u16 species = GetMonData(&party[gBattlerPartyIndexes[battler]], MON_DATA_SPECIES);
    u32 personality = GetMonData(&party[gBattlerPartyIndexes[battler]], MON_DATA_PERSONALITY);
    u32 position = GetBattlerPosition(battler);

    // Load palette first via BattleLoadMonSpriteGfx (handles shiny/illusion correctly),
    // then overwrite the pixel buffer with front sprite pixels.
    BattleLoadMonSpriteGfx(&party[gBattlerPartyIndexes[battler]], battler);
    HandleLoadSpecialPokePic(TRUE, gMonSpritesGfxPtr->spritesGfx[position], species, personality);

    // Create the full front sprite (invisible — shown only while Dynamaxed).
    SetMultiuseSpriteTemplateToPokemon(species, B_POSITION_OPPONENT_LEFT);
    gBattlerSpriteIds[battler] = CreateSprite(&gMultiuseSpriteTemplate,
                                              GetBattlerSpriteCoord(battler, BATTLER_COORD_X_2),
                                              GetBattlerSpriteDefault_Y(battler),
                                              GetBattlerSpriteSubpriority(battler));
    gSprites[gBattlerSpriteIds[battler]].oam.paletteNum = battler;
    gSprites[gBattlerSpriteIds[battler]].data[0] = battler;
    gSprites[gBattlerSpriteIds[battler]].data[2] = species;
    StartSpriteAnim(&gSprites[gBattlerSpriteIds[battler]], 0);
    gSprites[gBattlerSpriteIds[battler]].invisible = TRUE;

    // Create 32×32 icon sprite for normal (non-Dynamax) display.
    LoadMonIconPalette(species);
    gBattleStruct->raid.allyIconSpriteId[battler - 2] =
        CreateMonIcon(species, SpriteCB_MonIcon,
                      GetBattlerSpriteCoord(battler, BATTLER_COORD_X),
                      GetBattlerSpriteCoord(battler, BATTLER_COORD_Y),
                      0, personality);

    gBattlerControllerFuncs[battler] = WaitForMonAnimAfterLoad;
}

static void RaidAllyHandleSwitchInAnim(u32 battler)
{
    BtlController_HandleSwitchInAnim(battler, TRUE, SwitchIn_TryShinyAnim);
}

// some explanation here
// in emerald it's possible to have a tag battle in the battle frontier facilities with AI
// which use the front sprite for both the player and the partner as opposed to any other battles (including the one with Steven) that use the back pic as well as animate it
static void RaidAllyHandleDrawTrainerPic(u32 battler)
{
    bool32 isFrontPic;
    s16 xPos, yPos;
    u32 trainerPicId;

    enum DifficultyLevel difficulty = GetBattlePartnerDifficultyLevel(gPartnerTrainerId);

    if (gPartnerTrainerId > TRAINER_PARTNER(PARTNER_NONE))
    {
        trainerPicId = gBattlePartners[difficulty][gPartnerTrainerId - TRAINER_PARTNER(PARTNER_NONE)].trainerPic;
        xPos = 90;
        yPos = (8 - gTrainerBacksprites[trainerPicId].coordinates.size) * 4 + 80;
    }
    else if (IsAiVsAiBattle())
    {
        trainerPicId = GetTrainerPicFromId(gPartnerTrainerId);
        xPos = 60;
        yPos = 80;
    }
    else
    {
        trainerPicId = GetFrontierTrainerFrontSpriteId(gPartnerTrainerId);
        xPos = 32;
        yPos = 80;
    }

    // Use back pic only if the partner Steven or is custom.
    if (gPartnerTrainerId > TRAINER_PARTNER(PARTNER_NONE))
        isFrontPic = FALSE;
    else
        isFrontPic = TRUE;

    BtlController_HandleDrawTrainerPic(battler, trainerPicId, isFrontPic, xPos, yPos, -1);
}

static void RaidAllyHandleTrainerSlideBack(u32 battler)
{
    BtlController_HandleTrainerSlideBack(battler, 35, FALSE);
}

static void RaidAllyHandleMoveAnimation(u32 battler)
{
    BtlController_HandleMoveAnimation(battler, FALSE);
}

static void RaidAllyHandlePrintString(u32 battler)
{
    BtlController_HandlePrintString(battler, FALSE, FALSE);
}

static void RaidAllyHandleChooseAction(u32 battler)
{
    AI_TrySwitchOrUseItem(battler);
    RaidAllyBufferExecCompleted(battler);
}

static void RaidAllyHandleChooseMove(u32 battler)
{
    u8 chosenMoveId;
    struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct *)(&gBattleResources->bufferA[battler][4]);

    chosenMoveId = gBattleStruct->aiMoveOrAction[battler];
    gBattlerTarget = gBattleStruct->aiChosenTarget[battler];
    u32 moveTarget = GetBattlerMoveTargetType(battler, moveInfo->moves[chosenMoveId]);

    if (moveTarget & MOVE_TARGET_USER)
        gBattlerTarget = battler;
    else if (moveTarget & MOVE_TARGET_BOTH)
    {
        gBattlerTarget = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        if (gAbsentBattlerFlags & (1u << gBattlerTarget))
            gBattlerTarget = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
    }
    // If partner can and should use a gimmick (considering trainer data), do it
    if (gBattleStruct->gimmick.usableGimmick[battler] != GIMMICK_NONE
        && !(gBattleStruct->gimmick.usableGimmick[battler] == GIMMICK_Z_MOVE
        && !ShouldUseZMove(battler, gBattlerTarget, moveInfo->moves[chosenMoveId])))
    {
        BtlController_EmitTwoReturnValues(battler, BUFFER_B, 10, (chosenMoveId) | (RET_GIMMICK) | (gBattlerTarget << 8));
    }
    else
    {
        BtlController_EmitTwoReturnValues(battler, BUFFER_B, 10, (chosenMoveId) | (gBattlerTarget << 8));
    }

    RaidAllyBufferExecCompleted(battler);
}

static void RaidAllyHandleChoosePokemon(u32 battler)
{
    s32 chosenMonId;
    // Choosing Revival Blessing target
    if ((gBattleResources->bufferA[battler][1] & 0xF) == PARTY_ACTION_CHOOSE_FAINTED_MON)
    {
        chosenMonId = gSelectedMonPartyId = GetFirstFaintedPartyIndex(battler);
    }
    // Switching out
    else if (gBattleStruct->monToSwitchIntoId[battler] >= PARTY_SIZE || !IsValidForBattle(&gPlayerParty[gBattleStruct->monToSwitchIntoId[battler]]))
    {
        chosenMonId = GetMostSuitableMonToSwitchInto(battler, SWITCH_AFTER_KO);

        if (chosenMonId == PARTY_SIZE || !IsValidForBattle(&gPlayerParty[chosenMonId])) // just switch to the next mon
        {
            s32 firstId = (IsAiVsAiBattle()) ? 0 : (PARTY_SIZE / 2);
            u32 battler1 = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
            u32 battler2 = IsDoubleBattle() ? GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT) : battler1;

            for (chosenMonId = firstId; chosenMonId < PARTY_SIZE; chosenMonId++)
            {
                if (GetMonData(&gPlayerParty[chosenMonId], MON_DATA_HP) != 0
                    && chosenMonId != gBattlerPartyIndexes[battler1]
                    && chosenMonId != gBattlerPartyIndexes[battler2])
                {
                    break;
                }
            }
        }
        gBattleStruct->monToSwitchIntoId[battler] = chosenMonId;
    }
    else // Mon to switch out has been already chosen.
    {
        chosenMonId = gBattleStruct->monToSwitchIntoId[battler];
        gBattleStruct->AI_monToSwitchIntoId[battler] = PARTY_SIZE;
        gBattleStruct->monToSwitchIntoId[battler] = chosenMonId;
    }
    BtlController_EmitChosenMonReturnValue(battler, BUFFER_B, chosenMonId, NULL);
    RaidAllyBufferExecCompleted(battler);
}

static void RaidAllyHandleHealthBarUpdate(u32 battler)
{
    BtlController_HandleHealthBarUpdate(battler, FALSE);
}

static void RaidAllyHandleIntroTrainerBallThrow(u32 battler)
{
    const u32 *trainerPal;
    enum DifficultyLevel difficulty = GetBattlePartnerDifficultyLevel(gPartnerTrainerId);

    if (gPartnerTrainerId > TRAINER_PARTNER(PARTNER_NONE))
        trainerPal = gTrainerBacksprites[gBattlePartners[difficulty][gPartnerTrainerId - TRAINER_PARTNER(PARTNER_NONE)].trainerPic].palette.data;
    else if (IsAiVsAiBattle())
        trainerPal = gTrainerSprites[GetTrainerPicFromId(gPartnerTrainerId)].palette.data;
    else
        trainerPal = gTrainerSprites[GetFrontierTrainerFrontSpriteId(gPartnerTrainerId)].palette.data; // 2 vs 2 multi battle in Battle Frontier, load front sprite and pal.

    BtlController_HandleIntroTrainerBallThrow(battler, 0xD6F9, trainerPal, 24, Controller_RaidAllyShowIntroHealthbox);
}

static void RaidAllyHandleDrawPartyStatusSummary(u32 battler)
{
    BtlController_HandleDrawPartyStatusSummary(battler, B_SIDE_PLAYER, TRUE);
}

static void RaidAllyHandleBattleAnimation(u32 battler)
{
    BtlController_HandleBattleAnimation(battler, FALSE, FALSE);
}

static void RaidAllyHandleEndLinkBattle(u32 battler)
{
    gBattleOutcome = gBattleResources->bufferA[battler][1];
    FadeOutMapMusic(5);
    BeginFastPaletteFade(3);
    RaidAllyBufferExecCompleted(battler);
    gBattlerControllerFuncs[battler] = SetBattleEndCallbacks;
}
