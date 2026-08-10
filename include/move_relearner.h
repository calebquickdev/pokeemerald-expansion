#ifndef GUARD_MOVE_RELEARNER_H
#define GUARD_MOVE_RELEARNER_H

#include "constants/move_relearner.h"

struct ListMenu;

void TeachMoveRelearnerMove(void);
void MoveRelearnerShowHideHearts(s32 move);
void MoveRelearnerShowHideCategoryIcon(s32);
s32 MoveRelearnerGetDisplayMove(s32 itemIndex, const struct ListMenu *list);
void CB2_InitLearnMove(void);
bool32 CanBoxMonRelearnAnyMove(struct BoxPokemon *boxMon);
bool32 CanBoxMonRelearnMoves(struct BoxPokemon *boxMon, enum MoveRelearnerStates state);
bool32 HasMoveToRelearn(struct BoxPokemon *boxMon, enum MoveRelearnerStates state);

extern enum MoveRelearnerStates gMoveRelearnerState;
extern enum RelearnMode gRelearnMode;

#endif //GUARD_MOVE_RELEARNER_H
