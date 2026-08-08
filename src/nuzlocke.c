#include "global.h"
#include "nuzlocke.h"
#include "achievements.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_script_commands.h"
#include "constants/battle.h"
#include "constants/flags.h"
#include "event_data.h"
#include "pokedex.h"
#include "pokemon.h"

#define NUZLOCKE_MAX_FAMILY_MEMBERS 16

static enum Species Nuzlocke_GetEvolutionRoot(enum Species species)
{
    enum Species pre;

    while ((pre = GetSpeciesPreEvolution(species)) != SPECIES_NONE)
        species = pre;

    return species;
}

static u8 Nuzlocke_GetFamilyMembers(enum Species root, enum Species *membersOut)
{
    u8 count = 0;
    u8 head = 0;

    membersOut[count++] = root;

    while (head < count && count < NUZLOCKE_MAX_FAMILY_MEMBERS)
    {
        const struct Evolution *evolutions = GetSpeciesEvolutions(membersOut[head++]);
        u8 i;

        if (evolutions == NULL)
            continue;

        for (i = 0; evolutions[i].method != EVOLUTIONS_END && count < NUZLOCKE_MAX_FAMILY_MEMBERS; i++)
        {
            enum Species target = SanitizeSpeciesId(evolutions[i].targetSpecies);
            u8 j;
            bool8 alreadyPresent = FALSE;

            if (target == SPECIES_NONE)
                continue;

            for (j = 0; j < count; j++)
            {
                if (membersOut[j] == target)
                {
                    alreadyPresent = TRUE;
                    break;
                }
            }
            if (!alreadyPresent)
                membersOut[count++] = target;
        }
    }

    return count;
}

bool8 Nuzlocke_IsEvolutionFamilyCaught(enum Species species)
{
    enum Species members[NUZLOCKE_MAX_FAMILY_MEMBERS];
    enum Species root = Nuzlocke_GetEvolutionRoot(species);
    u8 count = Nuzlocke_GetFamilyMembers(root, members);
    u8 i;

    for (i = 0; i < count; i++)
    {
        enum NationalDexOrder dexNum = SpeciesToNationalPokedexNum(members[i]);

        if (dexNum != NATIONAL_DEX_NONE && GetSetPokedexFlag(dexNum, FLAG_GET_CAUGHT))
            return TRUE;
    }

    return FALSE;
}

bool8 Nuzlocke_IsScriptedWildBattle(void)
{
    return (gBattleTypeFlags & (BATTLE_TYPE_SCRIPTED_WILD | BATTLE_TYPE_LEGENDARY)) != 0;
}

bool8 Nuzlocke_IsWildOpponentShiny(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        return FALSE;

    return IsMonShiny(GetBattlerMon(GetCatchingBattler()));
}

bool8 Nuzlocke_IsBattlerShiny(enum BattlerId battler)
{
    return IsMonShiny(GetBattlerMon(battler));
}

static bool8 Nuzlocke_IsCatchModeActive(void)
{
    return gSaveBlock1Ptr->nuzlockeModeEnabled && FlagGet(FLAG_NUZLOCKE_CATCH_MODE);
}

enum NuzlockeBallBlockReason Nuzlocke_GetBallBlockReason(void)
{
    enum Species species;
    u16 route;

    if (!Nuzlocke_IsCatchModeActive())
        return NUZLOCKE_BALL_BLOCK_NONE;

    if (Nuzlocke_IsWildOpponentShiny())
        return NUZLOCKE_BALL_BLOCK_NONE;

    if (Nuzlocke_IsScriptedWildBattle())
        return NUZLOCKE_BALL_BLOCK_NONE;

    route = GetCurrentMapId();
    if (GET_NUZLOCKE_FLAG(route))
        return NUZLOCKE_BALL_BLOCK_ROUTE;

    species = GetMonData(GetBattlerMon(GetCatchingBattler()), MON_DATA_SPECIES);
    if (Nuzlocke_IsEvolutionFamilyCaught(species))
        return NUZLOCKE_BALL_BLOCK_SPECIES_CLAUSE;

    return NUZLOCKE_BALL_BLOCK_NONE;
}

bool8 Nuzlocke_CanThrowBall(void)
{
    return Nuzlocke_GetBallBlockReason() == NUZLOCKE_BALL_BLOCK_NONE;
}

void Nuzlocke_ApplyRouteLockAfterWild(void)
{
    u16 route;
    enum Species species;

    if (!Nuzlocke_IsCatchModeActive())
        return;

    species = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES);
    if (Nuzlocke_IsEvolutionFamilyCaught(species))
        return;

    route = GetCurrentMapId();
    if (gBattleOutcome != B_OUTCOME_CAUGHT
     && AchievementBoost_HasNuzlockeSecondChance()
     && !GET_NUZLOCKE_EXTRA_FLAG(route))
        SET_NUZLOCKE_EXTRA_FLAG(route);
    else
        SET_NUZLOCKE_FLAG(route);
}
