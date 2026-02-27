#ifndef GUARD_RAID_DEN_H
#define GUARD_RAID_DEN_H

#include "constants/flags.h"
#include "constants/species.h"

#define MAX_DYNAMAX_DENS 20

// Maps den IDs 0–19 to FLAG_UNUSED_0x935–FLAG_UNUSED_0x948 in the daily flags range.
// ClearDailyFlags() resets these automatically at midnight.
#define FLAG_DAILY_DEN_RAIDED(denId)  (DAILY_FLAGS_START + 0x15 + (denId))

struct DynamaxDen
{
    u16 species;
    u8 isGmax;
    u8 starRating;
};

void UpdateDynamaxDens(u16 daysSince);
u16 RollDynamaxDenPokemon(u8 denId);
void SetupDynamaxDenObjects(void);
void ActivateDynamaxDen(void);
void OpenDenLobbyScreen(void);
void DoRaidBattle(void);

#endif // GUARD_RAID_DEN_H
