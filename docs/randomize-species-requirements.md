# Randomize Species — Requirements

Target design for `feature/nuzlocke-procrng-species-implementation`.

## Core remap

```text
zoneKey   = gMapHeader.regionMapSectionId          // MAPSEC (cave floors share)
sourceKey = GET_BASE_SPECIES_ID(sourceSpecies)
seed      = mix(OT, zoneKey, sourceKey, NG+ offset)
base      = legalBasePool[LocalRandom32(seed) % poolCount]   // uniform index; no next-valid
final     = playthroughForm(OT, base)                        // form seed has no MAPSEC
```

- Species-keyed (not slot-keyed) within a zone.
- Independent per MAPSEC; consistent per OT/playthrough.
- Destination distribution: uniform over the active **legal base** pool. Source early-dex clustering must not bias destinations.

## Legal pool gate

Exclude: `SPECIES_NONE`, `SPECIES_EGG`, `!IsSpeciesEnabled`, `natDexNum == NATIONAL_DEX_NONE`, `??????????` placeholders, mega/gmax/primal/ultra/tera/totem, `cannotBeTraded`.

## Settings

| Setting | Meaning |
|---------|---------|
| Randomize Species | Master on/off (`FLAG_RANDOMIZE_MON`) |
| Include Legends/Mythicals/UBs | General wild/trainer pool (`isRestrictedLegendary` / `isSubLegendary` / `isMythical` / `isUltraBeast` / `isParadox`) |
| Starter mode | **All** / **Non-Legendary** / **Starters Only** |

## Pools

| Path | Pool |
|------|------|
| Wild land/surf, trainers | Legal bases ± legends setting |
| Fishing | Innate Water-type legal bases (± legends) |
| Fossil gift / fossil source | Revive bases only: Omanyte, Kabuto, Aerodactyl, Lileep, Anorith, Cranidos, Shieldon, Tirtouga, Archen, Tyrunt, Amaura, Dracozolt, Arctozolt, Dracovish, Arctovish |
| Starters | Per starter mode; slot seed (not MAPSEC) |

## Forms

- Remap to base, then OT-stable form among allowed catchable forms (regionals, stripes, cosmetics).
- Mega/Gmax/etc. never chosen as random results.
- Item/story forms (Crowned, Origin, Calyrex riders) may appear in battle; **on catch** store base form.
  - Zacian/Zamazenta: keep Rusted Sword/Shield held when caught from crowned context.
  - Kyurem/Calyrex fusions: grant **base only**.

## Moves

- `FLAG_RANDOMIZE_MOVES` off ⇒ learnsets only; species-random must not invent move IDs.
- Trainers with species-random on: `GiveMonInitialMoveset` for remapped species (ignore vanilla `TrainerMon.moves`).
- Starters: learnset unless moves-random is on.

## Exempt

- Daycare eggs / hatches: do not remap.
- Battle Frontier / facilities: ignore species random.

## Other

- Ability lure: type-check **remapped** table species.
- No BST/evo-stage balancing in v1.
- No global uniqueness / bijective tables in v1.
