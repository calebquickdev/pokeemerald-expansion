#!/usr/bin/env python3
"""Add specialty mart clerks behind the counter on gym town marts.

Evolution clerks: all gym town marts (Dewford has no mart).
Mega stone clerks: Petalburg onward only (still badge-gated in script).
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# All Hoenn gym towns that have a Poké Mart (no Dewford mart exists).
EVO_MARTS = [
    "RustboroCity_Mart",
    "MauvilleCity_Mart",
    "LavaridgeTown_Mart",
    "PetalburgCity_Mart",
    "FortreeCity_Mart",
    "MossdeepCity_Mart",
    "SootopolisCity_Mart",
]

MEGA_MARTS = {
    "PetalburgCity_Mart",
    "FortreeCity_Mart",
    "MossdeepCity_Mart",
    "SootopolisCity_Mart",
}

EVO_CLERK = {
    "graphics_id": "OBJ_EVENT_GFX_MART_EMPLOYEE",
    "x": 1,
    "y": 2,
    "elevation": 3,
    "movement_type": "MOVEMENT_TYPE_FACE_RIGHT",
    "movement_range_x": 0,
    "movement_range_y": 0,
    "trainer_type": "TRAINER_TYPE_NONE",
    "trainer_sight_or_berry_tree_id": "0",
    "script": "Common_EventScript_EvolutionMartClerk",
    "flag": "0",
}

MEGA_CLERK = {
    "graphics_id": "OBJ_EVENT_GFX_MART_EMPLOYEE",
    "x": 1,
    "y": 4,
    "elevation": 3,
    "movement_type": "MOVEMENT_TYPE_FACE_RIGHT",
    "movement_range_x": 0,
    "movement_range_y": 0,
    "trainer_type": "TRAINER_TYPE_NONE",
    "trainer_sight_or_berry_tree_id": "0",
    "script": "Common_EventScript_MegaStoneMartClerk",
    "flag": "0",
}


def main():
    for name in EVO_MARTS:
        path = ROOT / "data" / "maps" / name / "map.json"
        data = json.loads(path.read_text(encoding="utf-8"))
        data["object_events"] = [
            obj
            for obj in data["object_events"]
            if obj.get("script")
            not in (
                "Common_EventScript_EvolutionMartClerk",
                "Common_EventScript_MegaStoneMartClerk",
            )
        ]
        insert_at = 1
        data["object_events"].insert(insert_at, dict(EVO_CLERK))
        if name in MEGA_MARTS:
            data["object_events"].insert(insert_at + 1, dict(MEGA_CLERK))
        path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8", newline="\n")
        print("updated", path, "objects", len(data["object_events"]))


if __name__ == "__main__":
    main()
