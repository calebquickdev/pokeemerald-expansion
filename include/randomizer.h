#ifndef GUARD_RANDOMIZER_H
#define GUARD_RANDOMIZER_H

#include "global.h"
#include "config/general.h"

#if RANDOMIZER_ENABLED == TRUE

u16 GetRandomizerSpecies(u8 mapGroup, u8 mapNum, u8 area, u8 slotIndex);
u16 GetRandomizerStarterSpecies(u8 slotIndex);

#endif // RANDOMIZER_ENABLED

#endif // GUARD_RANDOMIZER_H
