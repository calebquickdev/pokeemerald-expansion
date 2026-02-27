#include "global.h"
#include "battle.h"
#include "event_data.h"
#include "caps.h"
#include "pokemon.h"
#include "data.h"
#include "constants/opponents.h"


// Returns the highest level among all party members of the given trainer.
static u32 GetGymLeaderHighestLevel(u16 trainerId)
{
    const struct Trainer *trainer = GetTrainerStructFromId(trainerId);
    u32 maxLevel = 1;
    u32 i;

    for (i = 0; i < trainer->partySize; i++)
    {
        if (trainer->party[i].lvl > maxLevel)
            maxLevel = trainer->party[i].lvl;
    }
    return maxLevel;
}

// After all 8 badges, the cap is the LOWEST of the highest levels across all Elite Four members
// and the Champion. This ensures the player must grind to match the weakest E4 member.
static u32 GetE4LevelCap(void)
{
    static const u16 sEliteFourTrainers[] = {
        TRAINER_SIDNEY,
        TRAINER_PHOEBE,
        TRAINER_GLACIA,
        TRAINER_DRAKE,
        TRAINER_WALLACE,
    };
    u32 minMax = 100;
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sEliteFourTrainers); i++)
    {
        u32 highest = GetGymLeaderHighestLevel(sEliteFourTrainers[i]);
        if (highest < minMax)
            minMax = highest;
    }
    return minMax;
}

u32 GetCurrentLevelCap(void)
{
    if (B_LEVEL_CAP_TYPE == LEVEL_CAP_FLAG_LIST)
    {
        if (!FlagGet(FLAG_BADGE01_GET)) return GetGymLeaderHighestLevel(TRAINER_ROXANNE_1);
        if (!FlagGet(FLAG_BADGE02_GET)) return GetGymLeaderHighestLevel(TRAINER_BRAWLY_1);
        if (!FlagGet(FLAG_BADGE03_GET)) return GetGymLeaderHighestLevel(TRAINER_WATTSON_1);
        if (!FlagGet(FLAG_BADGE04_GET)) return GetGymLeaderHighestLevel(TRAINER_FLANNERY_1);
        if (!FlagGet(FLAG_BADGE05_GET)) return GetGymLeaderHighestLevel(TRAINER_NORMAN_1);
        if (!FlagGet(FLAG_BADGE06_GET)) return GetGymLeaderHighestLevel(TRAINER_WINONA_1);
        if (!FlagGet(FLAG_BADGE07_GET)) return GetGymLeaderHighestLevel(TRAINER_TATE_AND_LIZA_1);
        if (!FlagGet(FLAG_BADGE08_GET)) return GetGymLeaderHighestLevel(TRAINER_JUAN_1);
        if (!FlagGet(FLAG_IS_CHAMPION)) return GetE4LevelCap();
    }
    else if (B_LEVEL_CAP_TYPE == LEVEL_CAP_VARIABLE)
    {
        return VarGet(B_LEVEL_CAP_VARIABLE);
    }

    return MAX_LEVEL;
}

u32 GetSoftLevelCapExpValue(u32 level, u32 expValue)
{
    static const u32 sExpScalingDown[5] = { 4, 8, 16, 32, 64 };
    static const u32 sExpScalingUp[5]   = { 16, 8, 4, 2, 1 };

    u32 levelDifference;
    u32 currentLevelCap = GetCurrentLevelCap();

    if (B_EXP_CAP_TYPE == EXP_CAP_NONE)
        return expValue;

    if (level < currentLevelCap)
    {
        if (B_LEVEL_CAP_EXP_UP)
        {
            levelDifference = currentLevelCap - level;
            if (levelDifference > ARRAY_COUNT(sExpScalingUp) - 1)
                return expValue + (expValue / sExpScalingUp[ARRAY_COUNT(sExpScalingUp) - 1]);
            else
                return expValue + (expValue / sExpScalingUp[levelDifference]);
        }
        else
        {
            return expValue;
        }
    }
    else if (B_EXP_CAP_TYPE == EXP_CAP_HARD)
    {
        return 0;
    }
    else if (B_EXP_CAP_TYPE == EXP_CAP_SOFT)
    {
        levelDifference = level - currentLevelCap;
        if (levelDifference > ARRAY_COUNT(sExpScalingDown) - 1)
            return expValue / sExpScalingDown[ARRAY_COUNT(sExpScalingDown) - 1];
        else
            return expValue / sExpScalingDown[levelDifference];
    }
    else
    {
       return expValue;
    }
}

u32 GetCurrentEVCap(void)
{

    static const u16 sEvCapFlagMap[][2] = {
        // Define EV caps for each milestone
        {FLAG_BADGE01_GET, 30},
        {FLAG_BADGE02_GET, 90},
        {FLAG_BADGE03_GET, 150},
        {FLAG_BADGE04_GET, 210},
        {FLAG_BADGE05_GET, 270},
        {FLAG_BADGE06_GET, 330},
        {FLAG_BADGE07_GET, 390},
        {FLAG_BADGE08_GET, 450},
        {FLAG_IS_CHAMPION, MAX_TOTAL_EVS},
    };

    if (B_EV_CAP_TYPE == EV_CAP_FLAG_LIST)
    {
        for (u32 evCap = 0; evCap < ARRAY_COUNT(sEvCapFlagMap); evCap++)
        {
            if (!FlagGet(sEvCapFlagMap[evCap][0]))
                return sEvCapFlagMap[evCap][1];
        }
    }
    else if (B_EV_CAP_TYPE == EV_CAP_VARIABLE)
    {
        return VarGet(B_EV_CAP_VARIABLE);
    }
    else if (B_EV_CAP_TYPE == EV_CAP_NO_GAIN)
    {
        return 0;
    }

    return MAX_TOTAL_EVS;
}
