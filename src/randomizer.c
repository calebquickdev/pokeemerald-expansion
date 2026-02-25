#include "global.h"
#include "config/general.h"

#if RANDOMIZER_ENABLED == TRUE

#include "random.h"
#include "randomizer.h"
#include "pokemon.h"
#include "constants/species.h"

// Forms that only exist in battle (Mega, G-Max, Primal, etc.) cannot be wild encounters.
static bool32 IsWildLegalForm(u16 species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];
    return !(info->isMegaEvolution
          || info->isGigantamax
          || info->isPrimalReversion
          || info->isUltraBurst
          || info->isTeraForm
          || info->isTotem);
}

// Returns TRUE if this species entry is a base form (form 0 of its family).
// Alternate form entries (Alolan Vulpix, Mega Charizard, etc.) all return FALSE.
static bool32 IsBaseSpecies(u16 species)
{
    return GET_BASE_SPECIES_ID(species) == species;
}

// Picks a random wild-legal form from a base species' form table.
// If only one legal form exists (or no table), returns the base species itself.
static u16 PickRandomForm(u16 baseSpecies, rng_value_t *rng)
{
    const u16 *formTable = gSpeciesInfo[baseSpecies].formSpeciesIdTable;
    u16 i;
    u8 legalCount = 0;

    if (formTable == NULL)
        return baseSpecies;

    for (i = 0; formTable[i] != FORM_SPECIES_END; i++)
    {
        if (IsWildLegalForm(formTable[i]))
            legalCount++;
    }

    if (legalCount <= 1)
        return baseSpecies;

    u8 formRoll = (u8)(LocalRandom32(rng) % legalCount);
    u8 count = 0;
    for (i = 0; formTable[i] != FORM_SPECIES_END; i++)
    {
        if (IsWildLegalForm(formTable[i]))
        {
            if (count == formRoll)
                return formTable[i];
            count++;
        }
    }

    return baseSpecies;
}

// Deterministically picks one of the three starter species using the save file's seed.
// Uses 0xFF sentinel values for the map fields so this key space never collides with
// wild encounter keys (real mapGroup/mapNum values are always < 0xFF).
u16 GetRandomizerStarterSpecies(u8 slotIndex)
{
    return GetRandomizerSpecies(0xFF, 0xFF, 0xFF, slotIndex);
}

// Deterministically maps an encounter slot to a species using the save file's seed.
// Each unique (seed, mapGroup, mapNum, area, slotIndex) tuple always yields the same species.
// Uses LocalRandomSeed/LocalRandom32 so the global RNG state is never touched.
//
// Algorithm:
//   1. Rejection-sample a base species (form 0 of its family, wild-legal).
//      ~59% of species IDs are base forms, so expected iterations ~1.7.
//   2. From that base species' form table, pick a random wild-legal form.
u16 GetRandomizerSpecies(u8 mapGroup, u8 mapNum, u8 area, u8 slotIndex)
{
    u32 key = gSaveBlock2Ptr->randomizerSeed
            ^ ((u32)mapGroup << 24)
            ^ ((u32)mapNum   << 16)
            ^ ((u32)area     <<  8)
            ^ slotIndex;
    rng_value_t rng = LocalRandomSeed(key);

    u16 base;
    do {
        base = 1 + (u16)(LocalRandom32(&rng) % (NUM_SPECIES - 1));
    } while (!IsBaseSpecies(base) || !IsWildLegalForm(base));

    return PickRandomForm(base, &rng);
}

#endif // RANDOMIZER_ENABLED
