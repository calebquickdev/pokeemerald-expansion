from PIL import Image
import collections

SRC = r"C:\Users\caleb\.cursor\projects\c-Users-caleb-Github-pokemon-perfect-emerald\assets\candy_jar_ai_draft.png"

PAL = [
    (180, 180, 180),  # 0 transparent
    (49, 49, 49),     # 1 outline
    (255, 255, 255),  # 2 white
    (213, 230, 238),  # 3 pale glass
    (164, 197, 213),  # 4 mid glass
    (98, 139, 156),   # 5 deep glass
    (222, 57, 57),    # 6 red
    (255, 164, 164),  # 7 red hi
    (49, 131, 222),   # 8 blue
    (139, 197, 255),  # 9 blue hi
    (57, 180, 74),    # 10 green
    (139, 230, 156),  # 11 green hi
    (180, 131, 82),   # 12 wood
    (222, 180, 123),  # 13 wood hi
    (123, 82, 49),    # 14 wood dark
    (197, 197, 205),  # 15 metal
]


def classify(r, g, b, a=255):
    if a < 90:
        return 0
    mx, mn = max(r, g, b), min(r, g, b)
    sat = mx - mn
    lum = (r + g + b) / 3.0

    # background gray
    if sat < 35 and 150 < lum < 210 and abs(r - g) < 20 and abs(g - b) < 20:
        return 0

    if lum < 60:
        return 1
    if lum > 240 and sat < 45:
        return 2

    # wood
    if r > 95 and (r - b) > 35 and r >= g - 5 and b < 160 and g < 200:
        if lum > 175:
            return 13
        if lum < 115:
            return 14
        return 12

    # metal
    if sat < 28 and 120 <= lum <= 205:
        return 15

    # red candy (incl pink swirls)
    if r > 145 and r + 15 >= g and r >= b and (r - b) > 20:
        return 7 if lum > 185 or (g > 140 and b > 140) else 6

    # green candy
    if g > 135 and g >= r - 5 and g >= b - 5 and (g - min(r, b)) > 20:
        return 11 if lum > 185 else 10

    # blue candy vs glass
    if b > 140:
        # glass: cyan-ish, less saturated, higher green+red
        glasslike = g > 145 and r > 130 and sat < 80
        if glasslike:
            if lum > 205:
                return 2 if sat < 40 else 3
            if lum > 165:
                return 4
            return 5
        if b > r + 15:
            return 9 if lum > 180 else 8

    if b >= g - 5 and lum > 130 and sat < 90:
        if lum > 200:
            return 3
        if lum > 160:
            return 4
        return 5

    best, bestd = 1, 1e18
    for i, c in enumerate(PAL):
        if i == 0:
            continue
        d = (r - c[0]) ** 2 + (g - c[1]) ** 2 + (b - c[2]) ** 2
        if d < bestd:
            bestd, best = d, i
    return best


img = Image.open(SRC).convert("RGBA")
print("src", img.size)
bbox = img.getbbox()
cropped = img.crop(bbox)

# Fit into 22x22 then center -> keeps more detail than 20
tw, th = 22, 22
fitted = cropped.resize((tw, th), Image.Resampling.LANCZOS)
canvas_rgba = Image.new("RGBA", (24, 24), (180, 180, 180, 0))
ox, oy = (24 - tw) // 2, (24 - th) // 2
canvas_rgba.paste(fitted, (ox, oy), fitted)

out = Image.new("P", (24, 24))
out.putpalette([v for c in PAL for v in c])
src = canvas_rgba.load()
dst = out.load()

for y in range(24):
    for x in range(24):
        dst[x, y] = classify(*src[x, y])

# Dilate silhouette outline onto OUTSIDE transparent pixels only (don't eat interior)
opaque = [[dst[x, y] != 0 for x in range(24)] for y in range(24)]
to_outline = []
for y in range(24):
    for x in range(24):
        if opaque[y][x]:
            continue
        for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < 24 and 0 <= ny < 24 and opaque[ny][nx]:
                to_outline.append((x, y))
                break
for x, y in to_outline:
    dst[x, y] = 1

# Also darken existing near-black interior edges that are already dark glass
for y in range(24):
    for x in range(24):
        if dst[x, y] == 5:
            # if on extreme edge of opaque region keep as deep glass (rim)
            pass

out.save("graphics/items/icons/candy_jar.png")
with open("graphics/items/icon_palettes/candy_jar.pal", "w", newline="\n") as f:
    f.write("JASC-PAL\n0100\n16\n")
    for r, g, b in PAL:
        f.write(f"{r} {g} {b}\n")

preview = out.convert("RGB").resize((288, 288), Image.NEAREST)
preview.save("graphics/items/icons/_candy_jar_preview.png")

a = out.convert("RGB").resize((192, 192), Image.NEAREST)
b = Image.open("graphics/items/icons/powder_jar.png").convert("RGB").resize((192, 192), Image.NEAREST)
cmp = Image.new("RGB", (400, 192), (40, 40, 40))
cmp.paste(a, (0, 0))
cmp.paste(b, (208, 0))
cmp.save("graphics/items/icons/_compare.png")

print("grid:")
for y in range(24):
    print("".join(f"{dst[x,y]:X}" for x in range(24)))
print("counts", collections.Counter(dst[x, y] for y in range(24) for x in range(24)))
