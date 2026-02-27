#include "global.h"
#include "raid_den.h"

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
