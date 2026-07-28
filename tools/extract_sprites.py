"""Cut a hand-made / AI-generated character sheet into engine-ready frames.

    python tools/extract_sprites.py assets/Raw_Character/1000035619.png assets/rebel.png

What it does:
  1. Removes the flat background. Background pixels are NEUTRAL (R==G==B) and
     bright; your cream shirt is warm, so saturation separates them. It then
     flood-fills only from the image border, so white highlights *inside* the
     character are never punched out.
  2. Finds the rows of sprites by horizontal projection, and drops the thin
     bands that are text labels rather than artwork.
  3. Splits each row into individual sprites by vertical projection.
  4. Rescales every frame by ONE shared scale factor (from the median standing
     height) so the character never grows or shrinks between frames.
  5. Anchors each frame on the centroid of its LOWER BODY, not its bounding
     box. Bounding-box centring makes the body lurch sideways whenever an arm
     or leg extends; foot centring keeps it planted.
  6. Writes the packed sheet plus a _debug overlay so you can see what it cut.
"""
import sys, os
from collections import deque
import numpy as np
from PIL import Image

# must match src/sprites.h
FW, FH, COLS = 64, 64, 8
BASELINE, MID, CHARH = 60, 26, 52

SAT_MAX    = 10     # max channel spread for a pixel to count as "neutral"
BRIGHT_MIN = 218    # ...and this bright, to count as background
MIN_SPRITE_H = 40   # bands shorter than this are text labels, not artwork
MIN_SPRITE_W = 25
# Blank rows needed to split one band from the next. Measured, not guessed:
# on these sheets the caption sits only 5px under the artwork, so 6 swallows
# the labels into the sprite band and they end up baked into the frames.
GAP          = 4
MERGE_GAP    = 5    # detached bits this close get folded into the body beside them
TRAPPED_MIN_AREA = 45   # neutral blobs this big are trapped background, not highlights
MIN_COMPONENT_AREA = 2500  # below this it is loose debris, not a pose


def load_rgba(path):
    return np.array(Image.open(path).convert("RGBA"))


def background_mask(a):
    """Background = neutral (R==G==B) and bright.

    A border-only flood fill is not enough. The baked drop shadow joins the two
    feet, which seals the gap between the legs off from the image edge, so that
    wedge of checkerboard survives as a white triangle. So we remove every
    neutral+bright region that either touches the border OR is bigger than a
    highlight. Small enclosed neutral specks are kept — those are genuine
    highlights on the shirt.
    """
    from scipy import ndimage
    rgb = a[:, :, :3].astype(np.int16)
    spread = rgb.max(axis=2) - rgb.min(axis=2)
    cand = (spread <= SAT_MAX) & (rgb.min(axis=2) >= BRIGHT_MIN)

    lab, n = ndimage.label(cand, structure=np.ones((3, 3), bool))
    if n == 0:
        return cand

    h, w = cand.shape
    border = np.zeros(n + 1, bool)
    for edge in (lab[0, :], lab[h - 1, :], lab[:, 0], lab[:, w - 1]):
        border[np.unique(edge)] = True
    border[0] = False

    sizes = np.bincount(lab.ravel(), minlength=n + 1)
    kill = border | (sizes >= TRAPPED_MIN_AREA)
    kill[0] = False
    return kill[lab]


def bands(mask_rows, min_run, gap):
    """Turn a boolean per-row/column occupancy array into (start, end) runs."""
    out, start, blank = [], None, 0
    for i, v in enumerate(mask_rows):
        if v:
            if start is None: start = i
            blank = 0
        else:
            if start is not None:
                blank += 1
                if blank >= gap:
                    if i - blank - start + 1 >= min_run: out.append((start, i - blank))
                    start, blank = None, 0
    if start is not None and len(mask_rows) - start >= min_run:
        out.append((start, len(mask_rows) - 1))
    return out


def main():
    # Source art lives in art_source/ and is NEVER shipped. Only the packed
    # sheet goes into assets/, because the web build preloads that whole folder
    # onto the player's phone.
    src = sys.argv[1] if len(sys.argv) > 1 else "art_source/1000035619.png"
    dst = sys.argv[2] if len(sys.argv) > 2 else "assets/rebel.png"

    a = load_rgba(src)
    h, w = a.shape[:2]
    print(f"source {src}  {w}x{h}")

    bg = background_mask(a)
    a[:, :, 3] = np.where(bg, 0, 255)
    solid = ~bg
    print(f"background removed: {bg.mean()*100:.1f}% of pixels")

    # ---- rows ----
    row_occ = solid.sum(axis=1) > (w * 0.002)
    all_bands = bands(row_occ, 1, GAP)
    sprite_rows = [b for b in all_bands if (b[1] - b[0]) >= MIN_SPRITE_H]
    label_rows = [b for b in all_bands if (b[1] - b[0]) < MIN_SPRITE_H]
    print(f"bands: {len(sprite_rows)} sprite rows, {len(label_rows)} label/thin bands")
    for b in sprite_rows: print(f"   sprite row y={b[0]}..{b[1]}  h={b[1]-b[0]+1}")

    # ---- sprites within each row ----
    # Projection alone is not enough: JUMP KICK's extended leg overlaps the
    # x-range of the sprite beside it, so no column gap exists to split them.
    # Connected components separate them correctly because the two drawings
    # never actually touch. Detached bits (a thrown knife, the HURT sparks)
    # are then merged back into whichever body they sit closest to.
    from scipy import ndimage

    boxes = []
    for (y0, y1) in sprite_rows:
        strip = solid[y0:y1 + 1]
        lab, n = ndimage.label(strip, structure=np.ones((3, 3), bool))
        comps = []
        for (sy, sx) in ndimage.find_objects(lab):
            area = int((lab[sy, sx] > 0).sum())
            # Drop loose debris: a thrown tissue, a grenade in flight, a muzzle
            # flash floating clear of the barrel. Those belong to the engine as
            # real projectiles, not baked into a frame. A knocked-down body is
            # ~8000px and a curled-up roll ~7000px at source scale, so 2500 is
            # comfortably below any genuine pose.
            if area < MIN_COMPONENT_AREA:
                continue
            comps.append([sx.start, sy.start, sx.stop - 1, sy.stop - 1, area])

        comps.sort(key=lambda b: b[0])
        merged = []
        for c in comps:
            if merged:
                p = merged[-1]
                overlap = min(p[2], c[2]) - max(p[0], c[0])
                # merge when the pieces overlap horizontally, or nearly touch,
                # AND the newcomer is small enough to be an attachment
                small = c[4] < p[4] * 0.55
                if overlap > -MERGE_GAP and (small or overlap > (c[2] - c[0]) * 0.55):
                    p[0] = min(p[0], c[0]); p[1] = min(p[1], c[1])
                    p[2] = max(p[2], c[2]); p[3] = max(p[3], c[3]); p[4] += c[4]
                    continue
            merged.append(c)

        for c in merged:
            if (c[2] - c[0]) < MIN_SPRITE_W:
                continue
            boxes.append((c[0], y0 + c[1], c[2], y0 + c[3]))

    print(f"found {len(boxes)} sprites")

    # ---- one shared scale, from the median height ----
    heights = np.array([b[3] - b[1] + 1 for b in boxes])
    med = float(np.median(heights))
    scale = CHARH / med
    print(f"median sprite height {med:.0f}px -> scale {scale:.4f} (target {CHARH}px)")

    # ---- pack ----
    rows_needed = (len(boxes) + COLS - 1) // COLS
    sheet = Image.new("RGBA", (COLS * FW, rows_needed * FH), (0, 0, 0, 0))
    rgba = Image.fromarray(a)

    for i, (x0, y0, x1, y1) in enumerate(boxes):
        crop = rgba.crop((x0, y0, x1 + 1, y1 + 1))
        cw, ch = crop.size
        nw, nh = max(1, round(cw * scale)), max(1, round(ch * scale))
        crop = crop.resize((nw, nh), Image.LANCZOS)

        # Horizontal anchor: centroid of the bottom 25% (the feet), so an
        # extended punch does not drag the whole body sideways.
        arr = np.array(crop)[:, :, 3] > 16
        foot = arr[int(nh * 0.75):] if nh > 4 else arr
        xs = np.where(foot.any(axis=0))[0]
        cx = (xs.mean() if len(xs) else nw / 2.0)

        dst_x = (i % COLS) * FW + MID - int(round(cx))
        dst_y = (i // COLS) * FH + BASELINE - nh
        sheet.alpha_composite(crop, (dst_x, dst_y))

    sheet.save(dst)
    print(f"\nwrote {dst}  ({sheet.width}x{sheet.height}, {len(boxes)} frames, {COLS} cols)")

    # ---- debug overlay so you can check the cuts ----
    dbg = Image.fromarray(a).convert("RGB")
    from PIL import ImageDraw
    d = ImageDraw.Draw(dbg)
    for i, (x0, y0, x1, y1) in enumerate(boxes):
        d.rectangle([x0, y0, x1, y1], outline=(255, 0, 0), width=3)
        d.text((x0 + 5, y0 + 5), str(i), fill=(255, 255, 0))
    os.makedirs("art_source", exist_ok=True)
    dbgp = os.path.join("art_source", "_debug_cuts.png")
    dbg.save(dbgp)
    print(f"wrote {dbgp}  <- open this to check the boxes are right")

    # alignment proof: the packed sheet over the guide, so you can see at a
    # glance whether feet sit on the baseline in every frame
    gp = os.path.join("art_source", "templates", "_guide_character.png")
    if os.path.exists(gp):
        guide = Image.open(gp).convert("RGBA")
        chk = Image.new("RGBA", sheet.size, (20, 22, 30, 255))
        chk.alpha_composite(guide.crop((0, 0, sheet.width, sheet.height)))
        chk.alpha_composite(sheet)
        vp = os.path.join("art_source", "_verify_align.png")
        chk.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).convert("RGB").save(vp)
        print(f"wrote {vp}  <- feet should sit on every red line")


if __name__ == "__main__":
    main()
