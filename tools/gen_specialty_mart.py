#!/usr/bin/env python3
"""Generate specialty mart item lists from repo data."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
items_h = (ROOT / "include" / "constants" / "items.h").read_text(encoding="utf-8")

# Mega stones: ITEM_*ITE except non-megas
exclude_mega = {
    "ITEM_EVIOLITE",
    "ITEM_METEORITE",
    "ITEM_BLACK_AUGURITE",
}
megas = []
seen = set()
for m in re.findall(r"\b(ITEM_\w+ITE)\b", items_h):
    if m in seen or m in exclude_mega:
        continue
    seen.add(m)
    megas.append(m)

# Evolution items: scan evolution tables for ITEM_*
evo_items = []
seen_e = set()
for path in (ROOT / "src" / "data").rglob("*.h"):
    text = path.read_text(encoding="utf-8", errors="ignore")
    if "EVO_" not in text and "ITEM_" not in text:
        continue
    for m in re.findall(
        r"EVO_(?:ITEM|ITEM_HOLD|ITEM_HOLD_DAY|ITEM_HOLD_NIGHT|ITEM_HOLD_MALE|ITEM_HOLD_FEMALE|"
        r"ITEM_DAY|ITEM_NIGHT|ITEM_MALE|ITEM_FEMALE|ITEM_RAIN|ITEM_CRITICAL|TRADE_ITEM|"
        r"ITEM_COUNT_17|ITEM_COUNT_20|ITEM_COUNT_30|ITEM_COUNT_40|ITEM_COUNT|"
        r"LEVEL_ITEM|MAPSEC_ITEM)[^\n]*?(ITEM_\w+)",
        text,
    ):
        if m not in seen_e and m in items_h:
            seen_e.add(m)
            evo_items.append(m)

# Also pull common stones block from items.h range comments if sparse
fallback = [
    "ITEM_FIRE_STONE",
    "ITEM_WATER_STONE",
    "ITEM_THUNDER_STONE",
    "ITEM_LEAF_STONE",
    "ITEM_MOON_STONE",
    "ITEM_SUN_STONE",
    "ITEM_SHINY_STONE",
    "ITEM_DUSK_STONE",
    "ITEM_DAWN_STONE",
    "ITEM_ICE_STONE",
    "ITEM_LINKING_CORD",
    "ITEM_KINGS_ROCK",
    "ITEM_METAL_COAT",
    "ITEM_DRAGON_SCALE",
    "ITEM_UPGRADE",
    "ITEM_PROTECTOR",
    "ITEM_ELECTIRIZER",
    "ITEM_MAGMARIZER",
    "ITEM_DUBIOUS_DISC",
    "ITEM_REAPER_CLOTH",
    "ITEM_PRISM_SCALE",
    "ITEM_WHIPPED_DREAM",
    "ITEM_SACHET",
    "ITEM_OVAL_STONE",
    "ITEM_RAZOR_CLAW",
    "ITEM_RAZOR_FANG",
    "ITEM_SWEET_APPLE",
    "ITEM_TART_APPLE",
    "ITEM_CRACKED_POT",
    "ITEM_CHIPPED_POT",
    "ITEM_GALARICA_CUFF",
    "ITEM_GALARICA_WREATH",
    "ITEM_BLACK_AUGURITE",
    "ITEM_PEAT_BLOCK",
    "ITEM_AUSPICIOUS_ARMOR",
    "ITEM_MALICIOUS_ARMOR",
    "ITEM_SYRUPY_APPLE",
    "ITEM_UNREMARKABLE_TEACUP",
    "ITEM_MASTERPIECE_TEACUP",
    "ITEM_METAL_ALLOY",
    "ITEM_SCROLL_OF_DARKNESS",
    "ITEM_SCROLL_OF_WATERS",
    "ITEM_DEEP_SEA_TOOTH",
    "ITEM_DEEP_SEA_SCALE",
]
for m in fallback:
    if m not in seen_e and m in items_h:
        seen_e.add(m)
        evo_items.append(m)

evo_items = sorted(set(evo_items))
print(f"evo {len(evo_items)} mega {len(megas)}")

out = ROOT / "data" / "scripts" / "specialty_mart.inc"
lines = [
    "@ Specialty clerks: evolution items (all gym town marts) + mega stones (Petalburg onward, post-Norman)",
    "",
    "Common_EventScript_EvolutionMartClerk::",
    "\tlock",
    "\tfaceplayer",
    "\tmessage gText_HowMayIServeYou",
    "\twaitmessage",
    "\tpokemart Common_Pokemart_EvolutionItems",
    "\tmsgbox gText_PleaseComeAgain, MSGBOX_DEFAULT",
    "\trelease",
    "\tend",
    "",
    "Common_EventScript_MegaStoneMartClerk::",
    "\tlock",
    "\tfaceplayer",
    "\tgoto_if_unset FLAG_BADGE05_GET, Common_EventScript_SpecialtyMartNotReady",
    "\tmessage gText_HowMayIServeYou",
    "\twaitmessage",
    "\tpokemart Common_Pokemart_MegaStones",
    "\tmsgbox gText_PleaseComeAgain, MSGBOX_DEFAULT",
    "\trelease",
    "\tend",
    "",
    "Common_EventScript_SpecialtyMartNotReady::",
    "\tmsgbox Common_Text_SpecialtyMartNotReady, MSGBOX_DEFAULT",
    "\trelease",
    "\tend",
    "",
    "Common_Text_SpecialtyMartNotReady:",
    '\t.string "Sorry, my stock isn\'t ready yet.\\n"',
    '\t.string "Please come back after you\'ve\\l"',
    '\t.string "proven yourself at PETALBURG GYM.$"',
    "",
    "\t.align 2",
    "Common_Pokemart_EvolutionItems:",
]
for item in evo_items:
    lines.append(f"\t.2byte {item}")
lines.append("\tpokemartlistend")
lines.append("")
lines.append("\t.align 2")
lines.append("Common_Pokemart_MegaStones:")
for item in megas:
    lines.append(f"\t.2byte {item}")
lines.append("\tpokemartlistend")
lines.append("")

out.write_text("\n".join(lines), encoding="utf-8", newline="\n")
print("wrote", out)
