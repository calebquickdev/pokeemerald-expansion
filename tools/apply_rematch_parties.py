#!/usr/bin/env python3
"""Author gym rematch parties as leveled-up copies of story themes."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PARTY = ROOT / "src" / "data" / "trainers.party"

STAGES = {2: 5, 3: 10, 4: 15, 5: 20}

EVOLUTIONS = [
    ("Glimmet", "Glimmora"),
    ("Nosepass", "Probopass"),
    ("Aron", "Lairon"),
    ("Larvitar", "Pupitar"),
    ("Makuhita", "Hariyama"),
    ("Scraggy", "Scrafty"),
    ("Pawmo", "Pawmot"),
    ("Riolu", "Lucario"),
    ("Charjabug", "Vikavolt"),
]


def extract_block(text: str, trainer_id: str) -> str:
    m = re.search(rf"=== {trainer_id} ===.*?(?=\n=== TRAINER_)", text, flags=re.S)
    if not m:
        raise SystemExit(f"missing {trainer_id}")
    return m.group(0).rstrip() + "\n"


def bump_party(block: str, base_id: str, stage: int, bump: int) -> str:
    out_id = base_id[:-1] + str(stage)
    lines = block.splitlines()
    lines[0] = f"=== {out_id} ==="
    result = []
    for line in lines:
        if line.startswith("Level:"):
            lvl = int(line.split(":", 1)[1].strip())
            result.append(f"Level: {lvl + bump}")
            continue
        if (
            line
            and not line.startswith("-")
            and not line.startswith("===")
            and ":" not in line.split(" @ ")[0]
            and not any(
                line.startswith(p)
                for p in (
                    "Name:",
                    "Class:",
                    "Pic:",
                    "Gender:",
                    "Music:",
                    "Items:",
                    "Battle Type:",
                    "AI:",
                    "Starting Status:",
                    "Mugshot:",
                )
            )
            and stage >= 3
        ):
            species_line = line
            for pre, post in EVOLUTIONS:
                if (
                    species_line == pre
                    or species_line.startswith(pre + " @")
                    or species_line.startswith(pre + " ")
                ):
                    species_line = species_line.replace(pre, post, 1)
                    if "Eviolite" in species_line:
                        species_line = species_line.split(" @ ")[0]
                    break
            result.append(species_line)
            continue
        result.append(line)
    return "\n".join(result) + "\n"


def main():
    text = PARTY.read_text(encoding="utf-8")
    bases = [
        "TRAINER_ROXANNE_1",
        "TRAINER_BRAWLY_1",
        "TRAINER_WATTSON_1",
        "TRAINER_FLANNERY_1",
        "TRAINER_NORMAN_1",
        "TRAINER_WINONA_1",
        "TRAINER_TATE_AND_LIZA_1",
        "TRAINER_JUAN_1",
    ]

    for base in bases:
        story = extract_block(text, base)
        for stage, bump in STAGES.items():
            rematch_id = base[:-1] + str(stage)
            new_block = bump_party(story, base, stage, bump)
            if "NORMAN" in rematch_id or "TATE" in rematch_id:
                new_block = new_block.replace(
                    "Battle Type: Singles", "Battle Type: Doubles"
                )
            pattern = rf"=== {rematch_id} ===.*?(?=\n=== TRAINER_)"
            m = re.search(pattern, text, flags=re.S)
            if m:
                text = text[: m.start()] + new_block + "\n" + text[m.end() :]
                print("updated", rematch_id)
            else:
                insert_at = text.find("=== TRAINER_WATTSON_3 ===")
                if insert_at < 0:
                    insert_at = text.find("=== TRAINER_FLANNERY_2 ===")
                if insert_at < 0:
                    insert_at = len(text)
                text = text[:insert_at] + new_block + "\n" + text[insert_at:]
                print("inserted", rematch_id)

    PARTY.write_text(text, encoding="utf-8", newline="\n")
    print("done")


if __name__ == "__main__":
    main()
