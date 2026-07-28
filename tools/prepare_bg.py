"""Turn the big painted backgrounds into game-sized, shippable layers.

    python tools/prepare_bg.py

Reads assets/raw_drawings/BG*.png and writes assets/bg<N>_far.png and
assets/bg<N>_near.png.

Why two files per background:
  A belt-scroll background cannot scroll as one piece. The road under your feet
  has to move exactly with you or walking feels like ice, while the skyline
  behind has to move slower or there is no sense of depth. So each painting is
  cut at the horizon: everything above becomes the FAR layer (slow parallax),
  the road strip becomes the NEAR layer (1:1 with the player).

The originals are ~3 MB each; these come out at a few tens of KB, which matters
because the whole assets folder gets preloaded onto the player's phone.
"""
import os, glob, sys
from PIL import Image

GAME_W, GAME_H = 384, 224
BELT_TOP = 150            # must match src/game.h

# Fraction of the painting's height where the pavement meets the road. Tuned
# per image if one of them is framed differently.
SPLIT = {"default": 0.695}

OUT = "assets"                      # shipped: small, game-sized layers
SRC = "art_source/raw_drawings"     # workspace: the big originals, never shipped


def prepare(path):
    name = os.path.splitext(os.path.basename(path))[0].lower()   # bg0, bg1, ...
    im = Image.open(path).convert("RGBA")
    w, h = im.size

    frac = SPLIT.get(name, SPLIT["default"])
    cut = int(h * frac)

    # FAR: everything above the horizon, scaled so it lands on BELT_TOP.
    far = im.crop((0, 0, w, cut))
    far_h = BELT_TOP
    far_w = max(1, round(far.width * far_h / far.height))
    far = far.resize((far_w, far_h), Image.LANCZOS)

    # NEAR: the road, scaled to fill from BELT_TOP down to the bottom.
    near = im.crop((0, cut, w, h))
    near_h = GAME_H - BELT_TOP
    near_w = max(1, round(near.width * near_h / near.height))
    near = near.resize((near_w, near_h), Image.LANCZOS)

    fp = os.path.join(OUT, f"{name}_far.png")
    np_ = os.path.join(OUT, f"{name}_near.png")
    far.save(fp, optimize=True)
    near.save(np_, optimize=True)
    kb = (os.path.getsize(fp) + os.path.getsize(np_)) / 1024
    print(f"{name}: far {far_w}x{far_h}, near {near_w}x{near_h}  -> {kb:.0f} KB "
          f"(from {os.path.getsize(path)/1024/1024:.1f} MB)")


def main():
    files = sorted(glob.glob(os.path.join(SRC, "BG*.png")))
    if not files:
        print(f"no BG*.png found in {SRC}", file=sys.stderr)
        return 1
    os.makedirs(OUT, exist_ok=True)
    for f in files:
        prepare(f)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
