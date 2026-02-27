#include "global.h"
#include "main.h"
#include "raid_den.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "battle.h"
#include "battle_setup.h"
#include "battle_transition.h"
#include "overworld.h"
#include "script.h"
#include "constants/battle.h"
#include "constants/event_objects.h"

static void SetupRaidBossParty(u8 denId);

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

static void SetupRaidBossParty(u8 denId)
{
    (void)denId;
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
