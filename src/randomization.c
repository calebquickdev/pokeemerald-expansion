#include "global.h"
#include "randomization.h"
#include "battle.h"
#include "battle_pyramid.h"
#include "caps.h"
#include "data.h"
#include "event_data.h"
#include "item.h"
#include "move.h"
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
#include "data/procedural_items.h"

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

void ApplyResolvedTypesAndMovesToBattleMon(struct BattlePokemon *mon)
{
    u8 type1, type2;
    u16 originalMoves[MAX_MON_MOVES];
    u16 resolvedMoves[MAX_MON_MOVES];
    u32 moveIdx;

    GetResolvedTypePair(mon->species, &type1, &type2);
    mon->types[0] = type1;
    mon->types[1] = type2;
    mon->types[2] = TYPE_MYSTERY;

    for (moveIdx = 0; moveIdx < MAX_MON_MOVES; moveIdx++)
        originalMoves[moveIdx] = mon->moves[moveIdx];

    ResolveMonMoves(mon->species, originalMoves, resolvedMoves);
    for (moveIdx = 0; moveIdx < MAX_MON_MOVES; moveIdx++)
    {
        if (resolvedMoves[moveIdx] != originalMoves[moveIdx])
        {
            mon->moves[moveIdx] = resolvedMoves[moveIdx];
            mon->pp[moveIdx] = GetMovePP(resolvedMoves[moveIdx]);
        }
    }
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
    case SPECIES_ETERNATUS_ETERNAMAX:
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

static enum Species PickUniformFromLegendaryLikePool(rng_value_t *rngState)
{
    u32 attempts;
    u32 poolSize = NATIONAL_DEX_COUNT;

    for (attempts = 0; attempts < poolSize * 2; attempts++)
    {
        enum NationalDexOrder dexNum = (LocalRandom32(rngState) % poolSize) + 1;
        enum Species candidate = NationalPokedexNumToSpecies(dexNum);

        if (!IsSpeciesAllowedInRandomPool(candidate, TRUE))
            continue;
        if (!IsLegendaryLikeSpecies(candidate))
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

static bool32 IncludeLegendsInGeneralPool(void);

static const enum Species sPseudoLegendarySpecies[] =
{
    SPECIES_DRAGONITE,
    SPECIES_TYRANITAR,
    SPECIES_SALAMENCE,
    SPECIES_METAGROSS,
    SPECIES_GARCHOMP,
    SPECIES_HYDREIGON,
    SPECIES_GOODRA,
    SPECIES_GOODRA_HISUI,
    SPECIES_KOMMO_O,
    SPECIES_DRAGAPULT,
    SPECIES_BAXCALIBUR,
};

static u8 GetMonPrimaryTypeForTheme(enum Species species)
{
    u8 primaryType = gSpeciesInfo[species].types[0];
    u8 secondaryType = gSpeciesInfo[species].types[1];

    if (secondaryType != TYPE_NONE && primaryType == TYPE_NORMAL)
        return secondaryType;

    return primaryType;
}

static bool32 IsSpeciesAlreadyUsed(enum Species species, const enum Species *usedSpecies, u8 usedCount)
{
    u8 i;
    enum Species finalSpecies = GetFinalEvolution(species);

    for (i = 0; i < usedCount; i++)
    {
        if (GetFinalEvolution(usedSpecies[i]) == finalSpecies)
            return TRUE;
    }

    return FALSE;
}

static enum Species PickGuaranteedAllowedFinalSpecies(rng_value_t *rngState, bool32 allowLegends, const enum Species *usedSpecies, u8 usedCount, bool32 requireUnique)
{
    u32 poolSize = NATIONAL_DEX_COUNT;
    u32 start = LocalRandom32(rngState) % poolSize;
    u32 i;

    for (i = 0; i < poolSize; i++)
    {
        enum NationalDexOrder dexNum = ((start + i) % poolSize) + 1;
        enum Species candidate = NationalPokedexNumToSpecies(dexNum);
        enum Species finalSpecies;

        if (!IsSpeciesAllowedInRandomPool(candidate, allowLegends))
            continue;

        finalSpecies = GetFinalEvolution(candidate);

        if (requireUnique && IsSpeciesAlreadyUsed(finalSpecies, usedSpecies, usedCount))
            continue;

        return finalSpecies;
    }

    for (i = 0; i < poolSize; i++)
    {
        enum Species candidate = NationalPokedexNumToSpecies(i + 1);

        if (!IsSpeciesAllowedInRandomPool(candidate, TRUE))
            continue;

        return GetFinalEvolution(candidate);
    }

    return SPECIES_TREECKO;
}

static enum Species ScanChampionLegendarySpecies(rng_value_t *rngState, const enum Species *usedSpecies, u8 usedCount, bool32 requireUnique)
{
    u32 poolSize = NATIONAL_DEX_COUNT;
    u32 start = LocalRandom32(rngState) % poolSize;
    u32 i;

    for (i = 0; i < poolSize; i++)
    {
        enum Species candidate = NationalPokedexNumToSpecies(((start + i) % poolSize) + 1);
        enum Species finalSpecies;

        if (!IsSpeciesAllowedInRandomPool(candidate, TRUE))
            continue;
        if (!IsChampionLegendarySpecies(candidate))
            continue;

        finalSpecies = GetFinalEvolution(candidate);

        if (requireUnique && IsSpeciesAlreadyUsed(finalSpecies, usedSpecies, usedCount))
            continue;

        return finalSpecies;
    }

    return SPECIES_NONE;
}

static enum Species ScanPseudoLegendarySpecies(rng_value_t *rngState, const enum Species *usedSpecies, u8 usedCount, bool32 requireUnique)
{
    u8 i;
    u8 start = LocalRandom32(rngState) % ARRAY_COUNT(sPseudoLegendarySpecies);

    for (i = 0; i < ARRAY_COUNT(sPseudoLegendarySpecies); i++)
    {
        enum Species candidate = sPseudoLegendarySpecies[(start + i) % ARRAY_COUNT(sPseudoLegendarySpecies)];
        enum Species finalSpecies;

        if (!IsSpeciesAllowedInRandomPool(candidate, TRUE))
            continue;

        finalSpecies = GetFinalEvolution(candidate);

        if (requireUnique && IsSpeciesAlreadyUsed(finalSpecies, usedSpecies, usedCount))
            continue;

        return finalSpecies;
    }

    return SPECIES_NONE;
}

static enum Species ScanFullyEvolvedRandomSpecies(rng_value_t *rngState, bool32 allowLegends, const enum Species *usedSpecies, u8 usedCount, bool32 requireUnique, bool32 excludeLegendsAndPseudos)
{
    u32 poolSize = NATIONAL_DEX_COUNT;
    u32 start = LocalRandom32(rngState) % poolSize;
    u32 i;

    for (i = 0; i < poolSize; i++)
    {
        enum Species candidate = NationalPokedexNumToSpecies(((start + i) % poolSize) + 1);
        enum Species finalSpecies;

        if (!IsSpeciesAllowedInRandomPool(candidate, allowLegends))
            continue;

        finalSpecies = GetFinalEvolution(candidate);

        if (excludeLegendsAndPseudos)
        {
            if (IsChampionLegendarySpecies(finalSpecies))
                continue;
            if (IsPseudoLegendarySpecies(finalSpecies))
                continue;
        }

        if (requireUnique && IsSpeciesAlreadyUsed(finalSpecies, usedSpecies, usedCount))
            continue;

        return finalSpecies;
    }

    return SPECIES_NONE;
}

static enum Species ScanRandomSpeciesMatchingType(rng_value_t *rngState, u8 themeType, bool32 allowLegends, const enum Species *usedSpecies, u8 usedCount, bool32 requireUnique)
{
    u32 poolSize = NATIONAL_DEX_COUNT;
    u32 start = LocalRandom32(rngState) % poolSize;
    u32 i;

    for (i = 0; i < poolSize; i++)
    {
        enum Species candidate = NationalPokedexNumToSpecies(((start + i) % poolSize) + 1);
        enum Species finalSpecies;

        if (!IsSpeciesAllowedInRandomPool(candidate, allowLegends))
            continue;

        finalSpecies = GetFinalEvolution(candidate);

        if (!SpeciesMatchesThemeType(finalSpecies, themeType))
            continue;

        if (requireUnique && IsSpeciesAlreadyUsed(finalSpecies, usedSpecies, usedCount))
            continue;

        return finalSpecies;
    }

    return SPECIES_NONE;
}

bool32 IsChampionLegendarySpecies(enum Species species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];

    return info->isRestrictedLegendary
        || info->isSubLegendary
        || info->isMythical;
}

bool32 IsPseudoLegendarySpecies(enum Species species)
{
    u32 i;
    enum Species baseSpecies = GET_BASE_SPECIES_ID(species);

    for (i = 0; i < ARRAY_COUNT(sPseudoLegendarySpecies); i++)
    {
        if (baseSpecies == sPseudoLegendarySpecies[i])
            return TRUE;
    }

    return FALSE;
}

bool32 SpeciesMatchesThemeType(enum Species species, u8 themeType)
{
    if (themeType == TYPE_MYSTERY)
        return TRUE;

    return GetSpeciesType(species, 0) == themeType
        || GetSpeciesType(species, 1) == themeType;
}

u8 GetTrainerPartyThemeType(const struct Trainer *trainer, const u32 *monIndices, u8 monCount)
{
    u8 typeCounts[NUMBER_OF_MON_TYPES] = {0};
    u8 maxCount = 0;
    u8 themeType = TYPE_MYSTERY;
    u8 i;

    for (i = 0; i < monCount; i++)
    {
        enum Species species = GetFinalEvolution(trainer->party[monIndices[i]].species);
        u8 type = GetMonPrimaryTypeForTheme(species);

        typeCounts[type]++;
        if (typeCounts[type] > maxCount)
        {
            maxCount = typeCounts[type];
            themeType = type;
        }
    }

    return themeType;
}

enum Species PickChampionLegendarySpecies(rng_value_t *rngState, const enum Species *usedSpecies, u8 usedCount)
{
    u32 attempts;
    u32 poolSize = NATIONAL_DEX_COUNT;
    enum Species picked;

    for (attempts = 0; attempts < poolSize * 4; attempts++)
    {
        enum NationalDexOrder dexNum = (LocalRandom32(rngState) % poolSize) + 1;
        enum Species candidate = NationalPokedexNumToSpecies(dexNum);

        if (!IsSpeciesAllowedInRandomPool(candidate, TRUE))
            continue;
        if (!IsChampionLegendarySpecies(candidate))
            continue;
        if (IsSpeciesAlreadyUsed(candidate, usedSpecies, usedCount))
            continue;

        return GetFinalEvolution(candidate);
    }

    picked = ScanChampionLegendarySpecies(rngState, usedSpecies, usedCount, TRUE);
    if (picked != SPECIES_NONE)
        return picked;

    picked = ScanChampionLegendarySpecies(rngState, usedSpecies, usedCount, FALSE);
    if (picked != SPECIES_NONE)
        return picked;

    return PickGuaranteedAllowedFinalSpecies(rngState, TRUE, usedSpecies, usedCount, FALSE);
}

enum Species PickPseudoLegendarySpecies(rng_value_t *rngState, const enum Species *usedSpecies, u8 usedCount)
{
    enum Species eligible[ARRAY_COUNT(sPseudoLegendarySpecies)];
    u8 eligibleCount = 0;
    u8 i;
    enum Species picked;

    for (i = 0; i < ARRAY_COUNT(sPseudoLegendarySpecies); i++)
    {
        enum Species candidate = sPseudoLegendarySpecies[i];

        if (!IsSpeciesAllowedInRandomPool(candidate, TRUE))
            continue;
        if (IsSpeciesAlreadyUsed(candidate, usedSpecies, usedCount))
            continue;

        eligible[eligibleCount++] = candidate;
    }

    if (eligibleCount != 0)
        return GetFinalEvolution(eligible[LocalRandom32(rngState) % eligibleCount]);

    picked = ScanPseudoLegendarySpecies(rngState, usedSpecies, usedCount, TRUE);
    if (picked != SPECIES_NONE)
        return picked;

    picked = ScanPseudoLegendarySpecies(rngState, usedSpecies, usedCount, FALSE);
    if (picked != SPECIES_NONE)
        return picked;

    return PickGuaranteedAllowedFinalSpecies(rngState, TRUE, usedSpecies, usedCount, FALSE);
}

enum Species PickFullyEvolvedRandomSpecies(rng_value_t *rngState, const enum Species *usedSpecies, u8 usedCount)
{
    u32 attempts;
    u32 poolSize = NATIONAL_DEX_COUNT;
    bool32 allowLegends = IncludeLegendsInGeneralPool();
    enum Species picked;

    for (attempts = 0; attempts < poolSize * 4; attempts++)
    {
        enum NationalDexOrder dexNum = (LocalRandom32(rngState) % poolSize) + 1;
        enum Species candidate = NationalPokedexNumToSpecies(dexNum);
        enum Species finalSpecies;

        if (!IsSpeciesAllowedInRandomPool(candidate, allowLegends))
            continue;

        finalSpecies = GetFinalEvolution(candidate);

        if (IsChampionLegendarySpecies(finalSpecies))
            continue;
        if (IsPseudoLegendarySpecies(finalSpecies))
            continue;
        if (IsSpeciesAlreadyUsed(finalSpecies, usedSpecies, usedCount))
            continue;

        return finalSpecies;
    }

    picked = ScanFullyEvolvedRandomSpecies(rngState, allowLegends, usedSpecies, usedCount, TRUE, TRUE);
    if (picked != SPECIES_NONE)
        return picked;

    picked = ScanFullyEvolvedRandomSpecies(rngState, allowLegends, usedSpecies, usedCount, TRUE, FALSE);
    if (picked != SPECIES_NONE)
        return picked;

    picked = ScanFullyEvolvedRandomSpecies(rngState, allowLegends, usedSpecies, usedCount, FALSE, FALSE);
    if (picked != SPECIES_NONE)
        return picked;

    return PickGuaranteedAllowedFinalSpecies(rngState, allowLegends, usedSpecies, usedCount, FALSE);
}

enum Species PickRandomSpeciesMatchingType(rng_value_t *rngState, u8 themeType, bool32 allowLegends, const enum Species *usedSpecies, u8 usedCount)
{
    u32 attempts;
    u32 poolSize = NATIONAL_DEX_COUNT;
    enum Species picked;

    for (attempts = 0; attempts < poolSize * 8; attempts++)
    {
        enum NationalDexOrder dexNum = (LocalRandom32(rngState) % poolSize) + 1;
        enum Species candidate = NationalPokedexNumToSpecies(dexNum);
        enum Species finalSpecies;

        if (!IsSpeciesAllowedInRandomPool(candidate, allowLegends))
            continue;

        finalSpecies = GetFinalEvolution(candidate);

        if (!SpeciesMatchesThemeType(finalSpecies, themeType))
            continue;
        if (IsSpeciesAlreadyUsed(finalSpecies, usedSpecies, usedCount))
            continue;

        return finalSpecies;
    }

    picked = ScanRandomSpeciesMatchingType(rngState, themeType, allowLegends, usedSpecies, usedCount, TRUE);
    if (picked != SPECIES_NONE)
        return picked;

    picked = ScanRandomSpeciesMatchingType(rngState, themeType, allowLegends, usedSpecies, usedCount, FALSE);
    if (picked != SPECIES_NONE)
        return picked;

    picked = PickFullyEvolvedRandomSpecies(rngState, usedSpecies, usedCount);
    if (picked != SPECIES_NONE)
        return picked;

    return PickGuaranteedAllowedFinalSpecies(rngState, allowLegends, usedSpecies, usedCount, FALSE);
}

void GenerateWallaceChampionParty(enum Species *outSpecies, u8 count, u32 trainerKey)
{
    u32 otId = GetTrainerId(gSaveBlock2Ptr->playerTrainerId);
    rng_value_t rngState = LocalRandomSeed(MixSpeciesRandomSeed(otId, trainerKey, GetNewGamePlusLevelOffset(), 0xC4A1u));
    u8 i;

    for (i = 0; i < count; i++)
    {
        if (i == 0)
            outSpecies[i] = PickChampionLegendarySpecies(&rngState, outSpecies, i);
        else if (i == 1)
            outSpecies[i] = PickPseudoLegendarySpecies(&rngState, outSpecies, i);
        else
            outSpecies[i] = PickFullyEvolvedRandomSpecies(&rngState, outSpecies, i);
    }
}

void GenerateTypedBossParty(enum Species *outSpecies, u8 count, u32 trainerKey, const struct Trainer *trainer, const u32 *monIndices)
{
    u32 otId = GetTrainerId(gSaveBlock2Ptr->playerTrainerId);
    rng_value_t rngState = LocalRandomSeed(MixSpeciesRandomSeed(otId, trainerKey, GetNewGamePlusLevelOffset(), 0xB055u));
    u8 themeType = GetTrainerPartyThemeType(trainer, monIndices, count);
    bool32 allowLegends = IncludeLegendsInGeneralPool();
    u8 i;

    for (i = 0; i < count; i++)
        outSpecies[i] = PickRandomSpeciesMatchingType(&rngState, themeType, allowLegends, outSpecies, i);
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

    if (context == SPECIES_RAND_CTX_SCRIPTED_WILD && IsLegendaryLikeSpecies(sourceSpecies))
        return PickUniformFromLegendaryLikePool(rngState);

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
     || species == SPECIES_NECROZMA_ULTRA
     || species == SPECIES_ETERNATUS_ETERNAMAX)
    {
        baseSpecies = GET_BASE_SPECIES_ID(species);
        if (baseSpecies != species)
        {
            SetMonData(mon, MON_DATA_SPECIES, &baseSpecies);
            CalculateMonStats(mon);
        }
    }
}

// --- Overworld item randomization (FLAG_RANDOMIZE_ITEMS) ---

enum ProceduralItemTier
{
    PROCEDURAL_ITEM_TIER_COMMON,
    PROCEDURAL_ITEM_TIER_UNCOMMON,
    PROCEDURAL_ITEM_TIER_RARE,
    PROCEDURAL_ITEM_TIER_JACKPOT,
};

#define PROCEDURAL_ITEM_WEIGHT_COMMON    55
#define PROCEDURAL_ITEM_WEIGHT_UNCOMMON  25
#define PROCEDURAL_ITEM_WEIGHT_RARE      15
#define PROCEDURAL_ITEM_WEIGHT_JACKPOT    5

static bool32 IsProtectedOverworldItem(u16 itemId)
{
    enum Pocket pocket;

    if (itemId == ITEM_NONE || itemId >= ITEMS_COUNT)
        return TRUE;

    pocket = GetItemPocket(itemId);
    if (pocket == POCKET_KEY_ITEMS)
        return TRUE;

    // HMs (and any free TM/HM) must never be remapped or used as pool fodder.
    if (pocket == POCKET_TM_HM && GetItemPrice(itemId) == 0)
        return TRUE;

    return FALSE;
}

static enum ProceduralItemTier PickProceduralItemTier(rng_value_t *rngState)
{
    u32 roll = LocalRandom32(rngState) % 100;

    if (roll < PROCEDURAL_ITEM_WEIGHT_COMMON)
        return PROCEDURAL_ITEM_TIER_COMMON;
    roll -= PROCEDURAL_ITEM_WEIGHT_COMMON;
    if (roll < PROCEDURAL_ITEM_WEIGHT_UNCOMMON)
        return PROCEDURAL_ITEM_TIER_UNCOMMON;
    roll -= PROCEDURAL_ITEM_WEIGHT_UNCOMMON;
    if (roll < PROCEDURAL_ITEM_WEIGHT_RARE)
        return PROCEDURAL_ITEM_TIER_RARE;
    return PROCEDURAL_ITEM_TIER_JACKPOT;
}

static u16 PickFromProceduralItemPool(const u16 *pool, u32 count, rng_value_t *rngState)
{
    if (count == 0)
        return ITEM_NONE;
    return pool[LocalRandom32(rngState) % count];
}

u16 GetProceduralRandomizedOverworldItem(u16 itemId)
{
    u32 otId;
    u32 ngPlusOffset;
    u32 zoneKey;
    rng_value_t rngState;
    enum ProceduralItemTier tier;
    u16 remapped;

    if (itemId == ITEM_NONE || !FlagGet(FLAG_RANDOMIZE_ITEMS))
        return itemId;

    if (IsProtectedOverworldItem(itemId))
        return itemId;

    // Battle Pyramid already rolls its own item tables.
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
        return itemId;

    otId = GetTrainerId(gSaveBlock2Ptr->playerTrainerId);
    ngPlusOffset = GetNewGamePlusLevelOffset();
    zoneKey = gMapHeader.regionMapSectionId;
    rngState = LocalRandomSeed(MixSpeciesRandomSeed(otId, zoneKey, itemId, ngPlusOffset ^ 0x17EDu));

    tier = PickProceduralItemTier(&rngState);
    switch (tier)
    {
    case PROCEDURAL_ITEM_TIER_COMMON:
        remapped = PickFromProceduralItemPool(sProceduralItemsCommon, ARRAY_COUNT(sProceduralItemsCommon), &rngState);
        break;
    case PROCEDURAL_ITEM_TIER_UNCOMMON:
        remapped = PickFromProceduralItemPool(sProceduralItemsUncommon, ARRAY_COUNT(sProceduralItemsUncommon), &rngState);
        break;
    case PROCEDURAL_ITEM_TIER_RARE:
        remapped = PickFromProceduralItemPool(sProceduralItemsRare, ARRAY_COUNT(sProceduralItemsRare), &rngState);
        break;
    case PROCEDURAL_ITEM_TIER_JACKPOT:
    default:
        remapped = PickFromProceduralItemPool(sProceduralItemsJackpot, ARRAY_COUNT(sProceduralItemsJackpot), &rngState);
        break;
    }

    if (remapped == ITEM_NONE || IsProtectedOverworldItem(remapped))
        return itemId;

    return remapped;
}

void Script_RemapFindItem(void)
{
    gSpecialVar_0x8000 = GetProceduralRandomizedOverworldItem(gSpecialVar_0x8000);
}
