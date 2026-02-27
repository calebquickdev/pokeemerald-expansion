#include "global.h"
#include "main.h"
#include "raid_den.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "battle.h"
#include "battle_setup.h"
#include "battle_transition.h"
#include "overworld.h"
#include "pokemon.h"
#include "random.h"
#include "script.h"
#include "constants/battle.h"
#include "constants/event_objects.h"

static const struct {
    u8 mapGroup;
    u8 mapNum;
    u8 localId;
    u8 denId;
} sDenObjectTable[] = {
    { MAP_GROUP(LITTLEROOT_TOWN), MAP_NUM(LITTLEROOT_TOWN), 9, 0 },
};

u16 RollDynamaxDenPokemon(u8 denId)
{
    (void)denId;
    return SPECIES_RALTS;
}

void UpdateDynamaxDens(u16 daysSince)
{
    u8 i;
    (void)daysSince;

    for (i = 0; i < MAX_DYNAMAX_DENS; i++)
    {
        gSaveBlock2Ptr->dynamaxDens[i].species = RollDynamaxDenPokemon(i);
        gSaveBlock2Ptr->dynamaxDens[i].isGmax = 0;
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
    gSaveBlock2Ptr->dynamaxDens[denId].species = RollDynamaxDenPokemon(denId);
    gSaveBlock2Ptr->dynamaxDens[denId].isGmax = 0;
    gSaveBlock2Ptr->dynamaxDens[denId].starRating = 1;
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
