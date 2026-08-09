from openpyxl import Workbook
from openpyxl.styles import Font, Alignment, PatternFill, Border, Side
from openpyxl.utils import get_column_letter

wb = Workbook()

ws = wb.active
ws.title = "Gimmick Unlocks"
ws.append(["Gimmick", "When Unlocked", "How"])
for row in [
    ["Z-Moves", "After defeating Wattson (Badge 3)", "Mauville Gym gives Z-Power Ring. Type Z-Crystals are overworld item balls."],
    ["Mega Evolution", "After defeating Norman (Badge 5)", "Petalburg Gym gives Mega Ring. Mega Stones are overworld/item finds. NPCs may Mega earlier (Flannery, Winona)."],
    ["Terastallization", "After defeating Tate & Liza (Badge 7)", "Mossdeep Gym gives Tera Orb and sets charged / no-cost flags."],
    ["Dynamax / Gigantamax", "After defeating Juan (Badge 8) — NEW", "Grant Dynamax Band in Sootopolis Gym reward script. Drake room sets B_FLAG_DYNAMAX_BATTLE."],
]:
    ws.append(row)

ws2 = wb.create_sheet("Boss Summary")
ws2.append(["Trainer", "Format", "Fight Gimmick", "Ace Gimmick", "Party Size", "Ace Level"])
for row in [
    ["Roxanne", "Singles", "Stealth Rock up at start", "—", 4, 15],
    ["Brawly", "Singles", "—", "Riolu @ Eviolite", 4, 19],
    ["Wattson", "Singles", "Electric Terrain at start", "Manectric @ Electrium Z", 5, 23],
    ["Flannery", "Singles", "Sun via Torkoal Drought", "Mega Scovillain", 6, 29],
    ["Norman", "Doubles", "Galar Weezing + Slaking (Neutralizing Gas)", "Slaking (intentionally OP)", 6, 31],
    ["Winona", "Singles", "Tailwind (opponent) at start", "Mega Pidgeot", 6, 36],
    ["Tate & Liza", "Doubles", "Psychic Terrain at start + Expanding Force", "Mega Gallade", 6, 48],
    ["Juan", "Singles", "Primordial Sea at start", "Mega Gyarados", 6, 56],
    ["Sidney", "Singles", "Prankster / screens package", "Mega Tyranitar", 6, 61],
    ["Phoebe", "Doubles", "Trick Room pressure + Shedinja @ HDB", "Aegislash / Shedinja", 6, 63],
    ["Glacia", "Singles", "Snow via Alolan Ninetales", "Icium Z Ninetales / Cetitan ace", 6, 64],
    ["Drake", "Singles", "Dynamax battle (flag on)", "G-Max Duraludon", 6, 65],
    ["Wallace", "Singles", "1 legendary + 1 pseudo + 4 fully evolved", "Kyogre (legend ace)", 6, 65],
]:
    ws2.append(row)

ws3 = wb.create_sheet("Full Teams")
ws3.append([
    "Trainer", "Format", "Fight Gimmick", "Slot", "Role", "Pokemon", "Level", "Item",
    "Ability", "Move 1", "Move 2", "Move 3", "Move 4", "Notes",
])

teams = [
    ("Roxanne", "Singles", "Stealth Rock at start", [
        (1, "Lead", "Glimmet", 13, "", "Toxic Debris", "Rock Throw", "Smack Down", "Toxic Spikes", "Harden", ""),
        (2, "", "Rolycoly", 13, "", "Steam Engine", "Smack Down", "Rapid Spin", "Smokescreen", "Tackle", ""),
        (3, "", "Aron", 14, "", "Sturdy", "Rock Tomb", "Metal Claw", "Protect", "Harden", ""),
        (4, "Ace", "Nosepass", 15, "Oran Berry", "Sturdy", "Rock Tomb", "Thunder Wave", "Block", "Tackle", ""),
    ]),
    ("Brawly", "Singles", "—", [
        (1, "", "Makuhita", 16, "", "Guts", "Arm Thrust", "Bulk Up", "Knock Off", "Rock Tomb", ""),
        (2, "", "Mienfoo", 17, "", "Regenerator", "Drain Punch", "U-turn", "Fake Out", "Swift", ""),
        (3, "", "Stufful", 17, "", "Fluffy", "Brick Break", "Brutal Swing", "Baby-Doll Eyes", "Protect", ""),
        (4, "Ace", "Riolu", 19, "Eviolite", "Inner Focus", "Force Palm", "Bite", "Bulk Up", "Quick Attack", ""),
    ]),
    ("Wattson", "Singles", "Electric Terrain at start", [
        (1, "", "Stunfisk", 21, "", "Static", "Earth Power", "Thunder Wave", "Yawn", "Mud Shot", ""),
        (2, "", "Morpeko", 21, "", "Hunger Switch", "Spark", "Bite", "Fake Out", "Quick Attack", ""),
        (3, "", "Kilowattrel", 22, "", "Wind Power", "Volt Switch", "Air Cutter", "Thunder Wave", "Growl", ""),
        (4, "Paradox", "Iron Thorns", 22, "Sitrus Berry", "Quark Drive", "Rock Slide", "Thunder Fang", "Iron Defense", "Screech", "Nerfed: no EQ/Wild Charge"),
        (5, "Ace", "Manectric", 23, "Electrium Z", "Lightning Rod", "Thunderbolt", "Snarl", "Flame Burst", "Charge Beam", "Z-Move showcase"),
    ]),
    ("Flannery", "Singles", "Sun via Torkoal Drought", [
        (1, "Lead", "Torkoal", 26, "Heat Rock", "Drought", "Lava Plume", "Stealth Rock", "Clear Smog", "Body Press", ""),
        (2, "", "Arcanine", 27, "", "Intimidate", "Fire Fang", "Extreme Speed", "Will-O-Wisp", "Morning Sun", ""),
        (3, "", "Centiskorch", 27, "", "Flash Fire", "Fire Lash", "Bug Bite", "Coil", "Knock Off", ""),
        (4, "", "Turtonator", 28, "", "Shell Armor", "Incinerate", "Dragon Tail", "Iron Defense", "Protect", ""),
        (5, "", "Houndoom", 28, "", "Flash Fire", "Dark Pulse", "Fire Fang", "Will-O-Wisp", "Snarl", ""),
        (6, "Ace", "Scovillain", 29, "Scovillainite", "Chlorophyll → Mega Spicy Spray", "Overheat", "Energy Ball", "Growth", "Stomping Tantrum", "NPC Mega before player Mega Ring"),
    ]),
    ("Norman", "Doubles", "Galar Weezing + Slaking (intentionally hard)", [
        (1, "Lead", "Weezing-Galar", 29, "Black Sludge", "Neutralizing Gas", "Strange Steam", "Will-O-Wisp", "Protect", "Destiny Bond", "Suppresses Truant"),
        (2, "Lead partner", "Cinccino", 29, "", "Skill Link", "Tail Slap", "Rock Blast", "Bullet Seed", "Encore", ""),
        (3, "", "Tauros", 30, "", "Intimidate", "Rock Climb", "Zen Headbutt", "Close Combat", "Protect", ""),
        (4, "", "Exploud", 30, "", "Soundproof", "Hyper Voice", "Fire Blast", "Ice Beam", "Protect", ""),
        (5, "Mega", "Kangaskhan", 31, "Kangaskhanite", "Scrappy → Mega Parental Bond", "Fake Out", "Body Slam", "Crunch", "Power-Up Punch", ""),
        (6, "Ace", "Slaking", 31, "Life Orb", "Truant (suppressed by Weezing)", "Giga Impact", "Earthquake", "Sucker Punch", "Slack Off", "Intentionally OP"),
    ]),
    ("Winona", "Singles", "Tailwind (opponent) at start", [
        (1, "", "Staraptor", 34, "", "Intimidate", "Brave Bird", "Close Combat", "U-turn", "Double-Edge", ""),
        (2, "", "Gligar", 34, "Eviolite", "Immunity", "Earthquake", "Aerial Ace", "Roost", "Knock Off", ""),
        (3, "", "Talonflame", 35, "", "Gale Wings", "Acrobatics", "Flare Blitz", "Roost", "Will-O-Wisp", ""),
        (4, "", "Noivern", 35, "", "Infiltrator", "Air Slash", "Dragon Pulse", "Super Fang", "Roost", ""),
        (5, "", "Altaria", 36, "", "Natural Cure", "Dragon Pulse", "Roost", "Cotton Guard", "Earthquake", ""),
        (6, "Ace", "Pidgeot", 36, "Pidgeotite", "Keen Eye → Mega No Guard", "Hurricane", "Heat Wave", "U-turn", "Protect", ""),
    ]),
    ("Tate & Liza", "Doubles", "Psychic Terrain at start + Expanding Force", [
        (1, "Lead", "Indeedee-F", 46, "Focus Sash", "Psychic Surge", "Expanding Force", "Follow Me", "Heal Pulse", "Protect", "Backup terrain setter"),
        (2, "Lead", "Armarouge", 46, "Life Orb", "Flash Fire", "Expanding Force", "Armor Cannon", "Aura Sphere", "Protect", ""),
        (3, "", "Bronzong", 47, "", "Levitate", "Expanding Force", "Gyro Ball", "Hypnosis", "Protect", ""),
        (4, "", "Gothitelle", 47, "", "Shadow Tag", "Psychic", "Thunderbolt", "Protect", "Helping Hand", ""),
        (5, "Z-Move", "Malamar", 48, "Darkinium Z", "Contrary", "Superpower", "Psycho Cut", "Knock Off", "Topsy-Turvy", ""),
        (6, "Ace", "Gallade", 48, "Galladite", "Justified → Mega Inner Focus", "Psycho Cut", "Close Combat", "Shadow Sneak", "Wide Guard", ""),
    ]),
    ("Juan", "Singles", "Primordial Sea at start", [
        (1, "", "Slowking", 55, "", "Regenerator", "Scald", "Slack Off", "Future Sight", "Yawn", ""),
        (2, "", "Walrein", 55, "Leftovers", "Thick Fat", "Surf", "Toxic", "Protect", "Rest", ""),
        (3, "", "Kingdra", 56, "Life Orb", "Swift Swim", "Hydro Pump", "Dragon Pulse", "Ice Beam", "Flip Turn", ""),
        (4, "", "Basculegion", 57, "", "Swift Swim", "Wave Crash", "Aqua Jet", "Crunch", "Flip Turn", ""),
        (5, "", "Milotic", 58, "Leftovers", "Marvel Scale", "Scald", "Recover", "Haze", "Ice Beam", ""),
        (6, "Ace", "Gyarados", 56, "Gyaradosite", "Intimidate → Mega Mold Breaker", "Waterfall", "Crunch", "Earthquake", "Dragon Dance", ""),
    ]),
    ("Sidney", "Singles", "Prankster / screens package", [
        (1, "Lead", "Grimmsnarl", 59, "Light Clay", "Prankster", "Reflect", "Light Screen", "Thunder Wave", "Spirit Break", ""),
        (2, "", "Weavile", 59, "Focus Sash", "Pressure", "Knock Off", "Ice Shard", "Triple Axel", "Low Kick", ""),
        (3, "", "Bisharp", 60, "", "Defiant", "Sucker Punch", "Iron Head", "Swords Dance", "Knock Off", ""),
        (4, "", "Hydreigon", 60, "", "Levitate", "Dark Pulse", "Draco Meteor", "Fire Blast", "U-turn", ""),
        (5, "", "Mandibuzz", 61, "Rocky Helmet", "Overcoat", "Foul Play", "Roost", "Toxic", "U-turn", ""),
        (6, "Ace", "Tyranitar", 61, "Tyranitarite", "Sand Stream → Mega Sand Stream", "Crunch", "Stone Edge", "Fire Punch", "Stealth Rock", ""),
    ]),
    ("Phoebe", "Doubles", "Trick Room + Shedinja @ Heavy-Duty Boots", [
        (1, "Lead", "Dusclops", 61, "Eviolite", "Frisk", "Trick Room", "Will-O-Wisp", "Night Shade", "Protect", ""),
        (2, "Lead", "Shedinja", 61, "Heavy-Duty Boots", "Wonder Guard", "Shadow Sneak", "X-Scissor", "Will-O-Wisp", "Protect", ""),
        (3, "", "Mimikyu", 62, "", "Disguise", "Play Rough", "Shadow Claw", "Shadow Sneak", "Protect", ""),
        (4, "", "Dragapult", 62, "", "Clear Body", "Shadow Ball", "Draco Meteor", "U-turn", "Protect", ""),
        (5, "", "Chandelure", 62, "", "Flash Fire", "Shadow Ball", "Heat Wave", "Energy Ball", "Protect", ""),
        (6, "Ace", "Aegislash", 63, "Leftovers", "Stance Change", "Shadow Ball", "Flash Cannon", "King's Shield", "Sacred Sword", ""),
    ]),
    ("Glacia", "Singles", "Snow via Alolan Ninetales", [
        (1, "Lead", "Ninetales-Alola", 62, "Icium Z", "Snow Warning", "Aurora Veil", "Moonblast", "Freeze-Dry", "Encore", ""),
        (2, "", "Baxcalibur", 63, "Loaded Dice", "Thermal Exchange", "Glaive Rush", "Icicle Spear", "Earthquake", "Dragon Dance", ""),
        (3, "", "Avalugg", 63, "", "Sturdy", "Body Press", "Recover", "Rock Slide", "Rapid Spin", ""),
        (4, "", "Frosmoth", 62, "", "Ice Scales", "Quiver Dance", "Bug Buzz", "Ice Beam", "Giga Drain", ""),
        (5, "", "Mamoswine", 63, "Focus Sash", "Thick Fat", "Icicle Crash", "Earthquake", "Ice Shard", "Stone Edge", ""),
        (6, "Ace", "Cetitan", 64, "Leftovers", "Slush Rush", "Ice Spinner", "Earthquake", "Belly Drum", "Ice Shard", ""),
    ]),
    ("Drake", "Singles", "Dynamax battle (flag on)", [
        (1, "", "Cyclizar", 63, "", "Regenerator", "Shed Tail", "Knock Off", "U-turn", "Dragon Claw", ""),
        (2, "", "Noivern", 63, "", "Infiltrator", "Draco Meteor", "Hurricane", "Flamethrower", "U-turn", ""),
        (3, "", "Haxorus", 63, "Lum Berry", "Mold Breaker", "Outrage", "Earthquake", "Poison Jab", "Dragon Dance", ""),
        (4, "", "Dragalge", 64, "", "Adaptability", "Sludge Wave", "Draco Meteor", "Toxic Spikes", "Flip Turn", ""),
        (5, "", "Salamence", 64, "", "Intimidate", "Dual Wingbeat", "Outrage", "Earthquake", "Dragon Dance", ""),
        (6, "Ace", "Duraludon", 65, "", "Light Metal", "Draco Meteor", "Flash Cannon", "Thunderbolt", "Stealth Rock", "Gigantamax: Yes; Dynamax Level: 10"),
    ]),
    ("Wallace", "Singles", "1 legendary + 1 pseudo + 4 fully evolved", [
        (1, "", "Milotic", 65, "Leftovers", "Marvel Scale", "Scald", "Recover", "Ice Beam", "Haze", ""),
        (2, "", "Gyarados", 65, "", "Intimidate", "Waterfall", "Earthquake", "Ice Fang", "Dragon Dance", ""),
        (3, "", "Swampert", 65, "", "Torrent", "Liquidation", "Earthquake", "Ice Punch", "Flip Turn", ""),
        (4, "", "Kingdra", 65, "Life Orb", "Swift Swim", "Hydro Pump", "Draco Meteor", "Ice Beam", "Hurricane", ""),
        (5, "Pseudo", "Dragonite", 65, "Lum Berry", "Multiscale", "Dual Wingbeat", "Earthquake", "Extreme Speed", "Dragon Dance", ""),
        (6, "Legend Ace", "Kyogre", 65, "Blue Orb", "Drizzle / Primordial Sea if Primal", "Origin Pulse", "Ice Beam", "Thunder", "Calm Mind", "Alt: Suicune if Kyogre too stacked after Juan"),
    ]),
]

for trainer, fmt, gimmick, mons in teams:
    for slot, role, mon, lv, item, ability, m1, m2, m3, m4, notes in mons:
        ws3.append([trainer, fmt, gimmick, slot, role, mon, lv, item, ability, m1, m2, m3, m4, notes])

header_font = Font(bold=True, color="FFFFFF")
header_fill = PatternFill("solid", fgColor="2F5496")
thin = Border(
    left=Side(style="thin", color="B0B0B0"),
    right=Side(style="thin", color="B0B0B0"),
    top=Side(style="thin", color="B0B0B0"),
    bottom=Side(style="thin", color="B0B0B0"),
)
alt_fill = PatternFill("solid", fgColor="D6E3F0")
ace_fill = PatternFill("solid", fgColor="FFF2CC")

for sheet in wb.worksheets:
    for cell in sheet[1]:
        cell.font = header_font
        cell.fill = header_fill
        cell.alignment = Alignment(vertical="center", wrap_text=True)
    sheet.freeze_panes = "A2"
    sheet.auto_filter.ref = sheet.dimensions
    for col in range(1, sheet.max_column + 1):
        max_len = 0
        letter = get_column_letter(col)
        for cell in sheet[letter]:
            if cell.value is not None:
                max_len = max(max_len, min(len(str(cell.value)), 45))
            cell.border = thin
            cell.alignment = Alignment(vertical="center", wrap_text=True)
        sheet.column_dimensions[letter].width = max(12, max_len + 2)

for row in range(2, ws3.max_row + 1):
    role = ws3.cell(row=row, column=5).value or ""
    fill = ace_fill if ("Ace" in str(role) or role == "Legend Ace") else (alt_fill if row % 2 == 0 else None)
    if fill:
        for col in range(1, ws3.max_column + 1):
            ws3.cell(row=row, column=col).fill = fill

out = r"c:\Users\caleb\Github\pokemon-perfect-emerald\docs\gym_e4_champion_teams.xlsx"
wb.save(out)
print(out)
