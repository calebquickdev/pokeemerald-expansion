#ifndef GUARD_NUZLOCKE_H
#define GUARD_NUZLOCKE_H

#include "global.h"
#include "constants/species.h"

enum NuzlockeBallBlockReason
{
    NUZLOCKE_BALL_BLOCK_NONE,
    NUZLOCKE_BALL_BLOCK_ROUTE,
    NUZLOCKE_BALL_BLOCK_SPECIES_CLAUSE,
};

bool8 Nuzlocke_IsEvolutionFamilyCaught(enum Species species);
bool8 Nuzlocke_IsScriptedWildBattle(void);
bool8 Nuzlocke_IsWildOpponentShiny(void);
bool8 Nuzlocke_IsBattlerShiny(enum BattlerId battler);
bool8 Nuzlocke_CanThrowBall(void);
enum NuzlockeBallBlockReason Nuzlocke_GetBallBlockReason(void);
u16 Nuzlocke_GetRouteKey(void);
void Nuzlocke_RecordWildEncounterState(void);
void Nuzlocke_ApplyRouteLockAfterWild(void);

#endif // GUARD_NUZLOCKE_H
