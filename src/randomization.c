#include "global.h"
#include "randomization.h"
#include "battle.h"
#include "caps.h"
#include "event_data.h"
#include "new_game.h"
#include "pokemon.h"
#include "random.h"
#include "ui_birch_case.h"
#include "constants/characters.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokedex.h"
#include "constants/pokemon.h"
#include "constants/region_map_sections.h"
#include "constants/species.h"
#include "constants/vars.h"

// This module is the single place that decides WHETHER a mon's type or move
// is randomized and HOW the pieces (dual types, per-slot movesets) combine.
// The underlying RNG primitives (GetRandomType/GetRandomMove/GetRandomMoveType)
// still live in ui_birch_case.c for now; callers should migrate to the
// functions below instead of checking FLAG_RANDOMIZE_TYPE / FLAG_RANDOMIZE_MOVES
// and calling those primitives directly.

void GetResolvedTypePair(u16 species, u8 *outType1, u8 *outType2)
{
    u8 originalType1 = gSpeciesInfo[species].types[0];
    u8 originalType2 = gSpeciesInfo[species].types[1];
    bool8 isOriginalDualType = (originalType2 != TYPE_NONE && originalType2 != originalType1);

    if (FlagGet(FLAG_RANDOMIZE_TYPE))
    {
        *outType1 = GetRandomType(species, 0);
        // Only randomize type2 if the original species had a dual type, so a
        // naturally single-typed species stays single-typed after resolving.
        if (isOriginalDualType)
            *outType2 = GetRandomType(species, 1);
        else
            *outType2 = *outType1;
    }
    else
    {
        *outType1 = originalType1;
        *outType2 = originalType2;
    }
}

u16 GetResolvedMove(u16 species, u16 originalMove)
{
    if (originalMove == MOVE_NONE)
        return originalMove;

    return GetEffectiveMove(originalMove, species);
}

u8 GetResolvedMoveType(u16 move, u8 baseType)
{
    if (FlagGet(FLAG_RANDOMIZE_TYPE))
        return GetRandomMoveType(move);

    return baseType;
}

void ResolveMonMoves(u16 species, const u16 *originalMoves, u16 *outMoves)
{
    u16 resolvedMoves[MAX_MON_MOVES];
    u32 moveIdx;
    u32 outCount = 0;

    for (moveIdx = 0; moveIdx < MAX_MON_MOVES; moveIdx++)
        resolvedMoves[moveIdx] = MOVE_NONE;

    for (moveIdx = 0; moveIdx < MAX_MON_MOVES; moveIdx++)
    {
        u16 originalMove = originalMoves[moveIdx];
        u16 resolvedMove;
        u32 dupIdx;
        bool8 isDuplicate = FALSE;

        if (originalMove == MOVE_NONE)
            continue;

        resolvedMove = GetResolvedMove(species, originalMove);

        // Two different original moves can resolve to the same randomized
        // move. Skip duplicates rather than wasting a move slot on a repeat,
        // matching the dedup behavior trainer-party building used to do
        // inline before it was centralized here.
        for (dupIdx = 0; dupIdx < outCount; dupIdx++)
        {
            if (resolvedMoves[dupIdx] == resolvedMove)
            {
                isDuplicate = TRUE;
                break;
            }
        }

        if (!isDuplicate)
            resolvedMoves[outCount++] = resolvedMove;
    }

    // Copy from a local buffer (not directly into outMoves) so this remains
    // safe to call with outMoves == originalMoves.
    for (moveIdx = 0; moveIdx < MAX_MON_MOVES; moveIdx++)
        outMoves[moveIdx] = resolvedMoves[moveIdx];
}

void ResolveMonData(u16 species, const u16 *originalMoves, struct ResolvedMonData *out)
{
    GetResolvedTypePair(species, &out->type1, &out->type2);
    ResolveMonMoves(species, originalMoves, out->moves);
}

// --- Species randomization -------------------------------------------------

EWRAM_DATA static enum SpeciesRandomContext sSpeciesRandomContext = SPECIES_RAND_CTX_NORMAL;

static const enum Species sFossilReviveBases[] =
{
    SPECIES_OMANYTE,
    SPECIES_KABUTO,
    SPECIES_AERODACTYL,
    SPECIES_LILEEP,
    SPECIES_ANORITH,
    SPECIES_CRANIDOS,
    SPECIES_SHIELDON,
    SPECIES_TIRTOUGA,
    SPECIES_ARCHEN,
    SPECIES_TYRUNT,
    SPECIES_AMAURA,
    SPECIES_DRACOZOLT,
    SPECIES_ARCTOZOLT,
    SPECIES_DRACOVISH,
    SPECIES_ARCTOVISH,
};

static const enum Species sStarterBaseSpecies[] =
{
    SPECIES_BULBASAUR, SPECIES_CHARMANDER, SPECIES_SQUIRTLE,
    SPECIES_CHIKORITA, SPECIES_CYNDAQUIL, SPECIES_TOTODILE,
    SPECIES_TREECKO, SPECIES_TORCHIC, SPECIES_MUDKIP,
    SPECIES_TURTWIG, SPECIES_CHIMCHAR, SPECIES_PIPLUP,
    SPECIES_SNIVY, SPECIES_TEPIG, SPECIES_OSHAWOTT,
    SPECIES_CHESPIN, SPECIES_FENNEKIN, SPECIES_FROAKIE,
    SPECIES_ROWLET, SPECIES_LITTEN, SPECIES_POPPLIO,
    SPECIES_GROOKEY, SPECIES_SCORBUNNY, SPECIES_SOBBLE,
    SPECIES_SPRIGATITO, SPECIES_FUECOCO, SPECIES_QUAXLY,
};

void SetSpeciesRandomContext(enum SpeciesRandomContext context)
{
    sSpeciesRandomContext = context;
}

enum SpeciesRandomContext GetSpeciesRandomContext(void)
{
    return sSpeciesRandomContext;
}

static u32 MixSpeciesRandomSeed(u32 a, u32 b, u32 c, u32 d)
{
    return a ^ (b * 0x9E3779B9u) ^ (c * 0x85EBCA6Bu) ^ (d * 0xC2B2AE3Du);
}

static bool32 IsLegendaryLikeSpecies(enum Species species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];

    return info->isRestrictedLegendary
        || info->isSubLegendary
        || info->isMythical
        || info->isUltraBeast
        || info->isParadox;
}

static bool32 IsBattleOnlyTransformSpecies(enum Species species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];

    return info->isMegaEvolution
        || info->isPrimalReversion
        || info->isUltraBurst
        || info->isGigantamax
        || info->isTeraForm
        || info->isTotem
        || info->cannotBeTraded;
}

static bool32 IsPlaceholderSpecies(enum Species species)
{
    if (species == SPECIES_NONE || species == SPECIES_EGG)
        return TRUE;
    if (!IsSpeciesEnabled(species))
        return TRUE;
    if (gSpeciesInfo[species].natDexNum == NATIONAL_DEX_NONE)
        return TRUE;
    if (gSpeciesInfo[species].speciesName[0] == CHAR_QUESTION_MARK
     && gSpeciesInfo[species].speciesName[1] == CHAR_QUESTION_MARK)
        return TRUE;

    return FALSE;
}

bool32 IsSpeciesAllowedInRandomPool(enum Species species, bool32 allowLegends)
{
    if (IsPlaceholderSpecies(species))
        return FALSE;
    if (GET_BASE_SPECIES_ID(species) != species)
        return FALSE;
    if (IsBattleOnlyTransformSpecies(species))
        return FALSE;
    if (!allowLegends && IsLegendaryLikeSpecies(species))
        return FALSE;

    return TRUE;
}

static bool32 IsFossilReviveBase(enum Species species)
{
    u32 i;
    enum Species base = GET_BASE_SPECIES_ID(species);

    for (i = 0; i < ARRAY_COUNT(sFossilReviveBases); i++)
    {
        if (base == sFossilReviveBases[i])
            return TRUE;
    }

    return FALSE;
}

static bool32 IsRandomSpeciesFormTableException(enum Species species)
{
    switch (GET_BASE_SPECIES_ID(species))
    {
    case SPECIES_ROTOM:
    case SPECIES_ORICORIO:
    case SPECIES_TORNADUS:
    case SPECIES_THUNDURUS:
    case SPECIES_LANDORUS:
    case SPECIES_ENAMORUS:
    // Item/story forms may appear in battle; NormalizePersistentRandomMon
    // reverts them on catch.
    case SPECIES_ZACIAN:
    case SPECIES_ZAMAZENTA:
    case SPECIES_DIALGA:
    case SPECIES_PALKIA:
    case SPECIES_GIRATINA:
    case SPECIES_CALYREX:
        return TRUE;
    default:
        return FALSE;
    }
}

static bool32 IsSpeciesInFormChangeOrFusionTables(enum Species species, const u16 *formTable)
{
    u32 i, j;

    for (i = 0; formTable[i] != FORM_SPECIES_END; i++)
    {
        const struct FormChange *formChanges = GetSpeciesFormChanges(formTable[i]);
        const struct Fusion *fusionTable = gFusionTablePointers[formTable[i]];

        for (j = 0; formChanges != NULL && formChanges[j].method != FORM_CHANGE_TERMINATOR; j++)
        {
            if (formChanges[j].targetSpecies == species)
                return TRUE;
        }

        for (j = 0; fusionTable != NULL && fusionTable[j].fusionStorageIndex != FUSION_TERMINATOR; j++)
        {
            if (fusionTable[j].targetSpecies1 == species
             || fusionTable[j].targetSpecies2 == species
             || fusionTable[j].fusingIntoMon == species)
                return TRUE;
        }
    }

    return FALSE;
}

static bool32 IsAllowedCatchableForm(enum Species species, const u16 *formTable)
{
    enum Species baseSpecies = GET_BASE_SPECIES_ID(species);

    switch (species)
    {
    case SPECIES_DARMANITAN_ZEN:
    case SPECIES_DARMANITAN_GALAR_ZEN:
        return FALSE;
    case SPECIES_DARMANITAN_GALAR:
        return TRUE;
    default:
        break;
    }

    if (IsBattleOnlyTransformSpecies(species))
        return FALSE;

    if (species != baseSpecies
     && !IsRandomSpeciesFormTableException(baseSpecies)
     && IsSpeciesInFormChangeOrFusionTables(species, formTable))
        return FALSE;

    if (IsPlaceholderSpecies(species))
        return FALSE;

    return TRUE;
}

static enum Species PickPlaythroughForm(enum Species baseSpecies, u32 otId, u32 ngPlusOffset)
{
    const u16 *formTable = GetSpeciesFormTable(baseSpecies);
    u32 allowedCount = 0;
    u32 i;
    u32 pick;
    rng_value_t rngState;

    if (formTable == NULL)
        return baseSpecies;

    for (i = 0; formTable[i] != FORM_SPECIES_END; i++)
    {
        if (IsAllowedCatchableForm(formTable[i], formTable))
            allowedCount++;
    }

    if (allowedCount == 0)
        return baseSpecies;

    rngState = LocalRandomSeed(MixSpeciesRandomSeed(otId, baseSpecies, ngPlusOffset, 0xF0AAu));
    pick = LocalRandom32(&rngState) % allowedCount;

    for (i = 0; formTable[i] != FORM_SPECIES_END; i++)
    {
        if (!IsAllowedCatchableForm(formTable[i], formTable))
            continue;
        if (pick == 0)
            return formTable[i];
        pick--;
    }

    return baseSpecies;
}

static enum Species PickUniformFromNatDexPool(rng_value_t *rngState, bool32 allowLegends, bool32 waterOnly)
{
    u32 attempts;
    u32 poolSize = NATIONAL_DEX_COUNT;

    for (attempts = 0; attempts < poolSize * 2; attempts++)
    {
        enum NationalDexOrder dexNum = (LocalRandom32(rngState) % poolSize) + 1;
        enum Species candidate = NationalPokedexNumToSpecies(dexNum);

        if (!IsSpeciesAllowedInRandomPool(candidate, allowLegends))
            continue;
        if (waterOnly
         && GetSpeciesType(candidate, 0) != TYPE_WATER
         && GetSpeciesType(candidate, 1) != TYPE_WATER)
            continue;

        return candidate;
    }

    return SPECIES_NONE;
}

static enum Species PickUniformFromFixedPool(rng_value_t *rngState, const enum Species *pool, u32 poolCount, bool32 allowLegends)
{
    enum Species eligible[64];
    u32 eligibleCount = 0;
    u32 i;

    for (i = 0; i < poolCount && eligibleCount < ARRAY_COUNT(eligible); i++)
    {
        if (IsSpeciesAllowedInRandomPool(pool[i], allowLegends))
            eligible[eligibleCount++] = pool[i];
    }

    if (eligibleCount == 0)
        return SPECIES_NONE;

    return eligible[LocalRandom32(rngState) % eligibleCount];
}

static bool32 IncludeLegendsInGeneralPool(void)
{
    return FlagGet(FLAG_RANDOMIZE_INCLUDE_LEGENDS);
}

static enum Species PickBaseSpeciesForContext(enum Species sourceSpecies, enum SpeciesRandomContext context, rng_value_t *rngState)
{
    bool32 allowLegends = IncludeLegendsInGeneralPool();

    if (IsFossilReviveBase(sourceSpecies))
        return PickUniformFromFixedPool(rngState, sFossilReviveBases, ARRAY_COUNT(sFossilReviveBases), TRUE);

    if (context == SPECIES_RAND_CTX_FISHING)
        return PickUniformFromNatDexPool(rngState, allowLegends, TRUE);

    return PickUniformFromNatDexPool(rngState, allowLegends, FALSE);
}

static enum Species PickStarterBaseSpecies(rng_value_t *rngState)
{
    u16 mode = VarGet(VAR_STARTER_RANDOM_MODE);

    if (mode >= STARTER_RANDOM_MODE_COUNT)
        mode = STARTER_RANDOM_ALL;

    switch (mode)
    {
    case STARTER_RANDOM_NON_LEGEND:
        return PickUniformFromNatDexPool(rngState, FALSE, FALSE);
    case STARTER_RANDOM_STARTERS_ONLY:
        return PickUniformFromFixedPool(rngState, sStarterBaseSpecies, ARRAY_COUNT(sStarterBaseSpecies), TRUE);
    case STARTER_RANDOM_ALL:
    default:
        return PickUniformFromNatDexPool(rngState, TRUE, FALSE);
    }
}

enum Species GetProceduralRandomizedSpecies(enum Species species)
{
    u32 otId;
    u32 ngPlusOffset;
    enum Species sourceKey;
    enum Species baseSpecies;
    enum SpeciesRandomContext context;
    rng_value_t rngState;
    u32 zoneKey;

    if (species == SPECIES_NONE || !FlagGet(FLAG_RANDOMIZE_MON))
        return species;

    context = sSpeciesRandomContext;
    if (context == SPECIES_RAND_CTX_SKIP)
        return species;

    // Frontier / facilities keep authored species.
    if (gBattleTypeFlags & (BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_EREADER_TRAINER))
        return species;
    if (gMapHeader.regionMapSectionId == MAPSEC_BATTLE_FRONTIER)
        return species;

    otId = GetTrainerId(gSaveBlock2Ptr->playerTrainerId);
    ngPlusOffset = GetNewGamePlusLevelOffset();
    sourceKey = GET_BASE_SPECIES_ID(species);
    zoneKey = gMapHeader.regionMapSectionId;

    if (context == SPECIES_RAND_CTX_STARTER)
    {
        // Starters use a dedicated API; treat as skip if called through CreateMon.
        return species;
    }

    rngState = LocalRandomSeed(MixSpeciesRandomSeed(otId, zoneKey, sourceKey, ngPlusOffset));
    baseSpecies = PickBaseSpeciesForContext(sourceKey, context, &rngState);
    if (baseSpecies == SPECIES_NONE)
        return species;

    return PickPlaythroughForm(baseSpecies, otId, ngPlusOffset);
}

enum Species GetProceduralRandomizedStarterSpecies(u8 setIndex, u8 slotIndex)
{
    u32 otId;
    u32 ngPlusOffset;
    rng_value_t rngState;
    enum Species baseSpecies;

    if (!FlagGet(FLAG_RANDOMIZE_MON))
        return SPECIES_NONE;

    otId = GetTrainerId(gSaveBlock2Ptr->playerTrainerId);
    ngPlusOffset = GetNewGamePlusLevelOffset();
    rngState = LocalRandomSeed(MixSpeciesRandomSeed(otId, setIndex * 100u + slotIndex, ngPlusOffset, 0x57AAu));
    baseSpecies = PickStarterBaseSpecies(&rngState);
    if (baseSpecies == SPECIES_NONE)
        return SPECIES_TREECKO;

    return PickPlaythroughForm(baseSpecies, otId, ngPlusOffset);
}

void NormalizePersistentRandomMon(struct Pokemon *mon)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    enum Species baseSpecies;
    enum Item heldItem;
    enum Item rustedItem = ITEM_NONE;

    if (species == SPECIES_NONE || species == SPECIES_EGG)
        return;

    switch (species)
    {
    case SPECIES_ZACIAN_CROWNED:
        rustedItem = ITEM_RUSTED_SWORD;
        break;
    case SPECIES_ZAMAZENTA_CROWNED:
        rustedItem = ITEM_RUSTED_SHIELD;
        break;
    default:
        break;
    }

    if (rustedItem != ITEM_NONE)
    {
        heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);
        if (heldItem == ITEM_NONE)
            SetMonData(mon, MON_DATA_HELD_ITEM, &rustedItem);
    }

    if (IsBattleOnlyTransformSpecies(species)
     || species == SPECIES_ZACIAN_CROWNED
     || species == SPECIES_ZAMAZENTA_CROWNED
     || species == SPECIES_KYUREM_WHITE
     || species == SPECIES_KYUREM_BLACK
     || species == SPECIES_CALYREX_ICE
     || species == SPECIES_CALYREX_SHADOW
     || species == SPECIES_NECROZMA_DUSK_MANE
     || species == SPECIES_NECROZMA_DAWN_WINGS
     || species == SPECIES_NECROZMA_ULTRA)
    {
        baseSpecies = GET_BASE_SPECIES_ID(species);
        if (baseSpecies != species)
        {
            SetMonData(mon, MON_DATA_SPECIES, &baseSpecies);
            CalculateMonStats(mon);
        }
    }
}
