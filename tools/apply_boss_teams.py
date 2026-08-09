#!/usr/bin/env python3
"""Apply docs/gym_e4_champion_teams_showdown.txt to story boss parties in trainers.party."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SHOWDOWN = ROOT / "docs" / "gym_e4_champion_teams_showdown.txt"
PARTY = ROOT / "src" / "data" / "trainers.party"

META = {
    "Roxanne": {
        "id": "TRAINER_ROXANNE_1",
        "header": """=== TRAINER_ROXANNE_1 ===
Name: ROXANNE
Class: Leader
Pic: Leader Roxanne
Gender: Female
Music: Female
Items: Potion / Potion
Battle Type: Singles
AI: Smart Trainer
Starting Status: Stealth Rock Opponent
""",
    },
    "Brawly": {
        "id": "TRAINER_BRAWLY_1",
        "header": """=== TRAINER_BRAWLY_1 ===
Name: BRAWLY
Class: Leader
Pic: Leader Brawly
Gender: Male
Music: Male
Items: Super Potion / Super Potion
Battle Type: Singles
AI: Smart Trainer
""",
    },
    "Wattson": {
        "id": "TRAINER_WATTSON_1",
        "header": """=== TRAINER_WATTSON_1 ===
Name: WATTSON
Class: Leader
Pic: Leader Wattson
Gender: Male
Music: Male
Items: Super Potion / Super Potion
Battle Type: Singles
AI: Smart Trainer / Ace Pokemon
Starting Status: Electric Terrain
""",
    },
    "Flannery": {
        "id": "TRAINER_FLANNERY_1",
        "header": """=== TRAINER_FLANNERY_1 ===
Name: FLANNERY
Class: Leader
Pic: Leader Flannery
Gender: Female
Music: Female
Items: Hyper Potion / Hyper Potion
Battle Type: Singles
AI: Smart Trainer
Starting Status: Sun
""",
    },
    "Norman": {
        "id": "TRAINER_NORMAN_1",
        "header": """=== TRAINER_NORMAN_1 ===
Name: NORMAN
Class: Leader
Pic: Leader Norman
Gender: Male
Music: Male
Items: Hyper Potion / Hyper Potion
Battle Type: Doubles
AI: Smart Trainer
""",
    },
    "Winona": {
        "id": "TRAINER_WINONA_1",
        "header": """=== TRAINER_WINONA_1 ===
Name: WINONA
Class: Leader
Pic: Leader Winona
Gender: Female
Music: Female
Items: Hyper Potion / Hyper Potion
Battle Type: Singles
AI: Smart Trainer / Risky
Starting Status: Tailwind Opponent
""",
    },
    "Tate & Liza": {
        "id": "TRAINER_TATE_AND_LIZA_1",
        "header": """=== TRAINER_TATE_AND_LIZA_1 ===
Name: TATE&LIZA
Class: Leader
Pic: Leader Tate And Liza
Gender: Male
Music: Female
Items: Hyper Potion / Hyper Potion / Hyper Potion / Hyper Potion
Battle Type: Doubles
AI: Smart Trainer
Starting Status: Trick Room
""",
    },
    "Juan": {
        "id": "TRAINER_JUAN_1",
        "header": """=== TRAINER_JUAN_1 ===
Name: JUAN
Class: Leader
Pic: Leader Juan
Gender: Male
Music: Male
Items: Hyper Potion / Hyper Potion
Battle Type: Singles
AI: Smart Trainer / Ace Pokemon
Starting Status: Primordial Sea
""",
    },
    "Sidney": {
        "id": "TRAINER_SIDNEY",
        "header": """=== TRAINER_SIDNEY ===
Name: SIDNEY
Class: Elite Four
Pic: Elite Four Sidney
Gender: Male
Music: Elite Four
Items: Full Restore / Full Restore
Battle Type: Singles
AI: Smart Trainer / Force Setup First Turn
Mugshot: Purple
""",
    },
    "Phoebe": {
        "id": "TRAINER_PHOEBE",
        "header": """=== TRAINER_PHOEBE ===
Name: PHOEBE
Class: Elite Four
Pic: Elite Four Phoebe
Gender: Female
Music: Elite Four
Items: Full Restore / Full Restore
Battle Type: Doubles
AI: Smart Trainer / Double Ace Pokemon
Mugshot: Green
""",
    },
    "Glacia": {
        "id": "TRAINER_GLACIA",
        "header": """=== TRAINER_GLACIA ===
Name: GLACIA
Class: Elite Four
Pic: Elite Four Glacia
Gender: Female
Music: Elite Four
Items: Full Restore / Full Restore
Battle Type: Singles
AI: Smart Trainer
Mugshot: Blue
""",
    },
    "Drake": {
        "id": "TRAINER_DRAKE",
        "header": """=== TRAINER_DRAKE ===
Name: DRAKE
Class: Elite Four
Pic: Elite Four Drake
Gender: Male
Music: Elite Four
Items: Full Restore / Full Restore
Battle Type: Singles
AI: Smart Trainer / Ace Pokemon
Mugshot: Yellow
""",
    },
    "Wallace": {
        "id": "TRAINER_WALLACE",
        "header": """=== TRAINER_WALLACE ===
Name: WALLACE
Class: Champion
Pic: Champion Wallace
Gender: Male
Music: Male
Items: Full Restore / Full Restore
Battle Type: Singles
AI: Smart Trainer / Prediction / Double Ace Pokemon
Mugshot: Yellow
""",
    },
}


def parse_mon(block: str):
    lines = [ln.rstrip() for ln in block.strip().splitlines() if ln.strip()]
    if not lines:
        return None
    first = lines[0]
    if " @ " in first:
        species, item = first.split(" @ ", 1)
    else:
        species, item = first, None
    species = species.strip()
    item = item.strip() if item else None
    ability = None
    level = None
    tera = None
    gigantamax = False
    ivs = {}
    moves = []
    for ln in lines[1:]:
        if ln.startswith("Ability:"):
            ability = ln.split(":", 1)[1].strip()
        elif ln.startswith("Level:"):
            level = int(ln.split(":", 1)[1].strip())
        elif ln.startswith("Tera Type:"):
            tera = ln.split(":", 1)[1].strip()
        elif ln.startswith("Gigantamax:"):
            gigantamax = ln.split(":", 1)[1].strip().lower() in ("yes", "y", "true")
        elif ln.startswith("IVs:"):
            for part in ln.split(":", 1)[1].split("/"):
                part = part.strip()
                m = re.match(r"(\d+)\s+(\w+)", part)
                if m:
                    ivs[m.group(2)] = int(m.group(1))
        elif ln.startswith("- "):
            moves.append(ln[2:].strip())
    return {
        "species": species,
        "item": item,
        "ability": ability,
        "level": level,
        "tera": tera,
        "gigantamax": gigantamax,
        "ivs": ivs,
        "moves": moves,
    }


def format_mon(mon, is_ace=False) -> str:
    out = []
    if mon["item"]:
        out.append(f"{mon['species']} @ {mon['item']}")
    else:
        out.append(mon["species"])
    out.append(f"Level: {mon['level']}")
    if mon["ability"]:
        out.append(f"Ability: {mon['ability']}")
    if mon["ivs"].get("Atk") == 0:
        nature = "Modest"
    elif mon["ivs"].get("Spe") == 0:
        nature = "Brave"
    else:
        nature = "Hardy"
    out.append(f"Nature: {nature}")
    if mon["tera"]:
        out.append(f"Tera Type: {mon['tera']}")
    iv_order = ["HP", "Atk", "Def", "SpA", "SpD", "Spe"]
    iv_vals = []
    for s in iv_order:
        if s in mon["ivs"]:
            v = mon["ivs"][s]
        elif is_ace:
            v = 31
        else:
            v = 25
        iv_vals.append(f"{v} {s}")
    out.append("IVs: " + " / ".join(iv_vals))
    if mon["gigantamax"]:
        out.append("Gigantamax: Yes")
        out.append("Dynamax Level: 10")
    for mv in mon["moves"]:
        out.append(f"- {mv}")
    return "\n".join(out)


def section_name(title_line: str) -> str:
    if title_line.startswith("Tate"):
        return "Tate & Liza"
    return title_line.split("(")[0].strip()


def main():
    showdown = SHOWDOWN.read_text(encoding="utf-8")
    party = PARTY.read_text(encoding="utf-8")
    sections = re.split(r"^=== ", showdown, flags=re.M)[1:]
    parsed = {}
    for sec in sections:
        title_line, _, body = sec.partition("\n")
        name = section_name(title_line)
        if name not in META:
            raise SystemExit(f"Unknown section: {title_line!r}")
        mons = []
        for ch in re.split(r"\n\s*\n", body.strip()):
            if not ch.strip():
                continue
            mon = parse_mon(ch)
            if mon:
                mons.append(mon)
        parsed[name] = mons
        print(name, len(mons), [m["species"] for m in mons])

    for name, meta in META.items():
        tid = meta["id"]
        mons = parsed[name]
        mon_text = "\n\n".join(
            format_mon(m, is_ace=(i == len(mons) - 1)) for i, m in enumerate(mons)
        )
        new_block = meta["header"].rstrip() + "\n\n" + mon_text + "\n"
        pattern = rf"=== {tid} ===.*?(?=\n=== TRAINER_)"
        m = re.search(pattern, party, flags=re.S)
        if not m:
            raise SystemExit(f"missing {tid}")
        party = party[: m.start()] + new_block + "\n" + party[m.end() :]
        print("replaced", tid)

    PARTY.write_text(party, encoding="utf-8", newline="\n")
    print("wrote", PARTY)


if __name__ == "__main__":
    main()
