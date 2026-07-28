"""Generate the sprite-sheet drawing template.

    python tools/make_sprite_template.py

Writes into assets/:
    _guide_<char>.png   grid + proportion guides + frame numbers  (reference layer)
    _blank_<char>.png   fully transparent, exact size             (draw on this)

Draw ONE frame per cell, character facing RIGHT, feet on the red baseline.
The engine mirrors sprites for left-facing, so never draw a left-facing frame.

Standard library only — no Pillow required.
"""
import os, struct, zlib

# ---- sheet geometry. Change here and the engine constants must match. ----
FW, FH = 64, 64          # one frame
COLS = 8                 # frames per row
BASELINE = 60            # feet sit on this y inside a cell
CENTER = 26              # body centre x inside a cell (attacks extend right)

# ---- proportion guides, measured up from the baseline ----
GUIDES = [("head", 52), ("chin", 43), ("shldr", 39), ("hip", 23), ("knee", 13)]

# ---- the pose list. MUST match the frame map in src/game.h ----
# This is the 20-pose layout the extracted rebel sheet already uses, so every
# other character lines up with it frame for frame.
# (name, frame count, tier)   tier 1 = draw these first, game is playable
ANIMS = [
    ("idle",     2, 1),   #  0..1   standing, breathing
    ("walk",     4, 1),   #  2..5   one full stride
    ("run",      3, 2),   #  6..8
    ("punch",    1, 1),   #  9
    ("kick",     1, 2),   # 10
    ("jump",     1, 2),   # 11
    ("jumpkick", 1, 2),   # 12
    ("crouch",   1, 3),   # 13     doubles as get-up
    ("hurt",     1, 1),   # 14
    ("down",     1, 1),   # 15     knocked flat
    ("knife",    3, 3),   # 16..18 weapon attack
    ("victory",  1, 3),   # 19
]

CHARS = ["rebel", "punk", "police", "heavy", "boss"]

# ---- 3x5 digits, enough to number the cells ----
DIGITS = {
    '0': ["111", "101", "101", "101", "111"], '1': ["010", "110", "010", "010", "111"],
    '2': ["111", "001", "111", "100", "111"], '3': ["111", "001", "111", "001", "111"],
    '4': ["101", "101", "111", "001", "001"], '5': ["111", "100", "111", "001", "111"],
    '6': ["111", "100", "111", "101", "111"], '7': ["111", "001", "010", "010", "010"],
    '8': ["111", "101", "111", "101", "111"], '9': ["111", "101", "111", "001", "111"],
}


class Img:
    def __init__(self, w, h):
        self.w, self.h, self.px = w, h, bytearray(w * h * 4)

    def set(self, x, y, c):
        x, y = int(x), int(y)
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[(y * self.w + x) * 4:(y * self.w + x) * 4 + 4] = bytes(c)

    def rect(self, x, y, w, h, c):
        for yy in range(int(y), int(y + h)):
            for xx in range(int(x), int(x + w)):
                self.set(xx, yy, c)

    def digit(self, ch, x, y, c, scale=2):
        rows = DIGITS.get(ch)
        if not rows:
            return
        for ry, row in enumerate(rows):
            for rx, bit in enumerate(row):
                if bit == '1':
                    self.rect(x + rx * scale, y + ry * scale, scale, scale, c)

    def number(self, n, x, y, c, scale=2):
        for i, ch in enumerate(str(n)):
            self.digit(ch, x + i * (3 * scale + scale), y, c, scale)

    def save(self, path):
        raw = b"".join(b"\x00" + bytes(self.px[y * self.w * 4:(y + 1) * self.w * 4])
                       for y in range(self.h))

        def chunk(tag, data):
            return (struct.pack(">I", len(data)) + tag + data +
                    struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

        with open(path, "wb") as f:
            f.write(b"\x89PNG\r\n\x1a\n" +
                    chunk(b"IHDR", struct.pack(">IIBBBBB", self.w, self.h, 8, 6, 0, 0, 0)) +
                    chunk(b"IDAT", zlib.compress(raw, 9)) +
                    chunk(b"IEND", b""))


# ---- expand the animation list into a flat frame map ----
frames, order = [], []
for name, count, tier in ANIMS:
    idxs = []
    for k in range(count):
        idxs.append(len(frames))
        frames.append((name, k, tier))
    order.append((name, idxs, tier))

TOTAL = len(frames)
ROWS = (TOTAL + COLS - 1) // COLS
W, H = COLS * FW, ROWS * FH

# The guide is an OPAQUE reference layer you keep underneath your drawing.
# It has to be dark and high-contrast or it vanishes against a white canvas.
BG     = ( 24,  26,  34, 255)
CELL   = ( 32,  35,  46, 255)     # alternating cell fill, so cells are countable
GRID   = ( 90, 100, 125, 255)
BASE   = (255,  80,  80, 255)     # feet line — the important one
MID    = ( 80, 170, 255, 200)     # body centre
GUIDE  = (255, 255, 255,  46)     # head / chin / shoulder / hip / knee
NUM    = (235, 240, 255, 255)
TINT   = {1: (110, 235, 160, 255), 2: (255, 205, 100, 255), 3: (240, 130, 190, 255)}

# Templates are a drawing aid, not a game asset — they live in the art
# workspace so they never get bundled into the phone build.
OUT_DIR = os.path.join("art_source", "templates")
os.makedirs(OUT_DIR, exist_ok=True)

guide = Img(W, H)
guide.rect(0, 0, W, H, BG)
for i, (name, k, tier) in enumerate(frames):
    cx, cy = (i % COLS) * FW, (i // COLS) * FH

    if (i + (i // COLS)) % 2 == 0:
        guide.rect(cx, cy, FW, FH, CELL)

    for _, up in GUIDES:                              # proportion lines
        guide.rect(cx + 2, cy + BASELINE - up, FW - 4, 1, GUIDE)

    guide.rect(cx + CENTER, cy + 5, 1, FH - 10, MID)  # body centre
    guide.rect(cx + 1, cy + BASELINE, FW - 2, 1, BASE)

    for gy in (0, FH - 1):                            # cell border
        guide.rect(cx, cy + gy, FW, 1, GRID)
    for gx in (0, FW - 1):
        guide.rect(cx + gx, cy, 1, FH, GRID)

    guide.rect(cx + 1, cy + 1, FW - 2, 2, TINT[tier]) # tier stripe along the top
    guide.number(i, cx + 3, cy + 6, NUM)

# unused cells at the end of the last row get struck out
for i in range(TOTAL, ROWS * COLS):
    cx, cy = (i % COLS) * FW, (i // COLS) * FH
    guide.rect(cx, cy, FW, FH, BG)
    for d in range(min(FW, FH)):
        guide.set(cx + d, cy + d, GRID)
        guide.set(cx + FW - 1 - d, cy + d, GRID)

guide.save(os.path.join("assets", "_guide_character.png"))

Img(W, H).save(os.path.join("assets", "_blank_character.png"))
for c in CHARS:
    Img(W, H).save(os.path.join("assets", f"_blank_{c}.png"))

print(f"sheet: {W}x{H}  ({COLS} cols x {ROWS} rows, {TOTAL} frames of {FW}x{FH})")
print(f"baseline y={BASELINE}  centre x={CENTER}  character height ~{GUIDES[0][1]}px\n")
print("wrote assets/_guide_character.png   <- keep as a background layer")
print("wrote assets/_blank_character.png   <- draw here, save as assets/<name>.png\n")
print("frame map (paste into src/game.h if you change the list):")
for name, idxs, tier in order:
    star = "*" if tier == 1 else " "
    print(f"  {star} {name:<9} {idxs}")
print("\n  * = tier 1. Draw these first; the game is playable with only these.")
