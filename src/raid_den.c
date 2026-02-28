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

void OpenDenLobbyScreen(void)
{
    DoRaidBattle();
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
