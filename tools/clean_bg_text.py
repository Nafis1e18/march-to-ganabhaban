"""Paint out the garbled text the image generator stamped onto backgrounds.

    python tools/clean_bg_text.py

AI-generated scenery routinely puts nonsense lettering on buildings and
banners. At 384x224 it reads as noise, so each bad patch is replaced by a
clean strip lifted from the same rows elsewhere in the image — same lighting,
same texture, no visible seam.

Regions are (x0, y0, x1, y1, source_x) in the SMALL bg<N>_far.png coordinates.
`source_x` is where to copy the replacement strip from, at the same rows.
"""
import os
from PIL import Image, ImageFilter

PATCHES = {
    # Shahbag: gibberish stamped across the ministry facade
    "bg0_far.png": [(120, 57, 156, 70, 168)],
    # the rest are checked by eye; add entries here as you spot them
    "bg1_far.png": [],
    "bg2_far.png": [],
    "bg3_far.png": [],
}


def clean(name, regions):
    path = os.path.join("assets", name)
    if not os.path.exists(path) or not regions:
        return
    im = Image.open(path).convert("RGBA")
    for (x0, y0, x1, y1, sx) in regions:
        w, h = x1 - x0, y1 - y0
        strip = im.crop((sx, y0, sx + w, y1))
        # a touch of blur hides the join where the copied texture meets
        strip = strip.filter(ImageFilter.GaussianBlur(0.4))
        im.paste(strip, (x0, y0))
    im.save(path)
    print(f"{name}: patched {len(regions)} region(s)")


for n, r in PATCHES.items():
    clean(n, r)
