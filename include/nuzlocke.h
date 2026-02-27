#ifndef GUARD_NUZLOCKE_H
#define GUARD_NUZLOCKE_H

// Set to TRUE before a wild battle when the player is not allowed to catch on this route.
extern bool8 gNuzlockeCatchBlocked;

bool32 IsNuzlockeModeEnabled(void);
bool32 IsNuzlockeRouteEncountered(u32 headerId);
void SetNuzlockeRouteEncountered(u32 headerId);
void CheckAndUpdateNuzlockeEncounterState(u32 headerId);

#endif // GUARD_NUZLOCKE_H
