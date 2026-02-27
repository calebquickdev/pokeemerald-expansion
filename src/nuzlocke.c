#include "global.h"
#include "event_data.h"
#include "pokemon.h"
#include "pokedex.h"
#include "constants/flags.h"
#include "constants/pokedex.h"
#include "nuzlocke.h"

#define NUZLOCKE_MAX_ROUTE_HEADERS 128

bool8 gNuzlockeCatchBlocked = FALSE;

bool32 IsNuzlockeModeEnabled(void)
{
    return FlagGet(FLAG_NUZLOCKE_MODE);
}

bool32 IsNuzlockeRouteEncountered(u32 headerId)
{
    if (headerId >= NUZLOCKE_MAX_ROUTE_HEADERS)
        return FALSE;
    return (gSaveBlock3Ptr->nuzlockeRouteEncountered[headerId / 8] >> (headerId % 8)) & 1;
}

void SetNuzlockeRouteEncountered(u32 headerId)
{
    if (headerId >= NUZLOCKE_MAX_ROUTE_HEADERS)
        return;
    gSaveBlock3Ptr->nuzlockeRouteEncountered[headerId / 8] |= (1 << (headerId % 8));
}

// Called after TryGenerateWildMon succeeds. Evaluates nuzlocke catch rules and
// sets gNuzlockeCatchBlocked accordingly for the upcoming battle.
void CheckAndUpdateNuzlockeEncounterState(u32 headerId)
{
    bool32 isShiny;
    bool32 alreadyOwned;
    bool32 routeUsed;
    u16 species;

    gNuzlockeCatchBlocked = FALSE;

    if (!IsNuzlockeModeEnabled())
        return;
    if (headerId >= NUZLOCKE_MAX_ROUTE_HEADERS)
        return;
    if (!FlagGet(FLAG_ADVENTURE_STARTED))
        return;

    species = GetMonData(&gEnemyParty[0], MON_DATA_SPECIES, NULL);
    isShiny = IsMonShiny(&gEnemyParty[0]);
    alreadyOwned = GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT);
    routeUsed = IsNuzlockeRouteEncountered(headerId);

    if (isShiny)
        return; // Shinies can always be caught; no route state change

    if (!alreadyOwned && !routeUsed)
        SetNuzlockeRouteEncountered(headerId); // First valid encounter; mark route used
    else if (routeUsed)
        gNuzlockeCatchBlocked = TRUE; // Route already used; no catching allowed
    // If alreadyOwned && !routeUsed: owned species before first valid encounter; allow but don't mark route
}
