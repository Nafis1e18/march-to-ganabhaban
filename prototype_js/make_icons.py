"""Generate the PWA icons with the standard library only (no Pillow needed).

    python tools/make_icons.py

Writes icons/icon-192.png and icons/icon-512.png.
"""
import math, os, struct, zlib

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "icons")


class Img:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.px = bytearray(w * h * 4)

    def set(self, x, y, c):
        x, y = int(x), int(y)
        if not (0 <= x < self.w and 0 <= y < self.h):
            return
        r, g, b, a = c
        i = (y * self.w + x) * 4
        if a >= 255:
            self.px[i:i + 4] = bytes((r, g, b, 255))
        else:                                   # simple source-over blend
            t = a / 255.0
            for k, v in enumerate((r, g, b)):
                self.px[i + k] = int(self.px[i + k] * (1 - t) + v * t)
            self.px[i + 3] = max(self.px[i + 3], a)

    def rect(self, x, y, w, h, c):
        for yy in range(int(y), int(y + h)):
            for xx in range(int(x), int(x + w)):
                self.set(xx, yy, c)

    def circle(self, cx, cy, r, c):
        for yy in range(int(cy - r), int(cy + r) + 1):
            dy = yy - cy
            span = math.sqrt(max(0.0, r * r - dy * dy))
            for xx in range(int(cx - span), int(cx + span) + 1):
                self.set(xx, yy, c)

    def line(self, x1, y1, x2, y2, w, c):
        n = max(1, int(math.hypot(x2 - x1, y2 - y1)))
        for i in range(n + 1):
            t = i / n
            self.circle(x1 + (x2 - x1) * t, y1 + (y2 - y1) * t, w / 2, c)

    def save(self, path):
        raw = b"".join(
            b"\x00" + bytes(self.px[y * self.w * 4:(y + 1) * self.w * 4])
            for y in range(self.h)
        )

        def chunk(tag, data):
            return (struct.pack(">I", len(data)) + tag + data
                    + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

        png = (b"\x89PNG\r\n\x1a\n"
               + chunk(b"IHDR", struct.pack(">IIBBBBB", self.w, self.h, 8, 6, 0, 0, 0))
               + chunk(b"IDAT", zlib.compress(raw, 9))
               + chunk(b"IEND", b""))
        with open(path, "wb") as f:
            f.write(png)


def joint(x, y, ang, ln):
    return x + math.sin(ang) * ln, y + math.cos(ang) * ln


def build(S):
    """Flat flag-field icon: green ground, red disc, one bold black silhouette.

    Everything sits inside the middle 80% so Android's maskable crop cannot
    slice off the flag, and it is a two-tone silhouette so it still reads at
    the 48px the launcher actually draws it at.
    """
    im = Img(S, S)

    GREEN_D = (14, 78, 46, 255)
    GREEN_L = (32, 148, 82, 255)
    RED = (222, 48, 48, 255)
    SILH = (7, 26, 17, 255)

    # green field, subtly lit from the top
    for y in range(S):
        t = y / S
        im.rect(0, y, S, 1, tuple(
            int(GREEN_L[k] + (GREEN_D[k] - GREEN_L[k]) * t) for k in range(3)
        ) + (255,))

    # the red disc, set slightly left of centre as on the flag
    im.circle(S * 0.46, S * 0.47, S * 0.235, RED)

    # darker ground so the marcher is standing on something
    im.rect(0, int(S * 0.76), S, S, (9, 44, 27, 255))

    # ---- silhouette marcher, mid-stride, flag held high ----
    GROUND = S * 0.76
    h = S * 0.44
    lw = max(3, int(h * 0.095))
    thigh = shin = h * 0.235
    upper = fore = h * 0.195

    hip = [S * 0.44, -h * 0.44]                 # y is relative for now
    shd = [S * 0.475, -h * 0.74]

    # planted front leg and trailing back leg
    fk = joint(hip[0], hip[1], 0.28, thigh)
    ff = joint(fk[0], fk[1], 0.06, shin)
    bk = joint(hip[0], hip[1], -0.78, thigh)
    bf = joint(bk[0], bk[1], -0.98, shin)

    # drop the whole figure so the lowest foot rests exactly on the ground
    dy = GROUND - max(ff[1], bf[1])
    sh = lambda p: (p[0], p[1] + dy)
    hip, shd, fk, ff, bk, bf = map(sh, (hip, shd, fk, ff, bk, bf))

    def taper(p1, p2, w1, w2, n=260):
        """thick line whose width eases from w1 to w2 — gives real shoulders"""
        for i in range(n + 1):
            t = i / n
            im.circle(p1[0] + (p2[0] - p1[0]) * t,
                      p1[1] + (p2[1] - p1[1]) * t,
                      (w1 + (w2 - w1) * t) / 2, SILH)

    # back leg first (behind the body)
    im.line(hip[0], hip[1], bk[0], bk[1], lw, SILH)
    im.line(bk[0], bk[1], bf[0], bf[1], lw * 0.85, SILH)
    im.line(bf[0], bf[1], bf[0] + h * 0.06, bf[1] + h * 0.02, lw * 0.8, SILH)   # toe

    # trailing arm, swung clearly back so it reads as a limb
    e = joint(shd[0], shd[1], -1.75, upper)
    im.line(shd[0], shd[1], e[0], e[1], lw * 0.85, SILH)
    im.line(e[0], e[1], *joint(e[0], e[1], -1.15, fore), lw * 0.75, SILH)

    # torso: narrow at the hip, broad at the shoulders
    taper(hip, shd, h * 0.17, h * 0.27)

    # neck + head, sitting clear of the shoulder line
    hx, hy = shd[0] + h * 0.055, shd[1] - h * 0.215
    im.line(shd[0], shd[1], hx, hy, h * 0.085, SILH)
    im.circle(hx, hy, h * 0.125, SILH)

    # front leg (in front of the body)
    im.line(hip[0], hip[1], fk[0], fk[1], lw, SILH)
    im.line(fk[0], fk[1], ff[0], ff[1], lw, SILH)
    im.line(ff[0], ff[1], ff[0] + h * 0.07, ff[1], lw * 0.8, SILH)              # foot

    # raised arm holding the flag aloft
    e = joint(shd[0], shd[1], 2.30, upper)
    hand = joint(e[0], e[1], 2.55, fore)
    im.line(shd[0], shd[1], e[0], e[1], lw * 0.9, SILH)
    im.line(e[0], e[1], hand[0], hand[1], lw * 0.8, SILH)

    # pole, gripped at the hand and running well past it both ways
    top = (hand[0] - S * 0.015, hand[1] - S * 0.150)
    im.line(hand[0] + S * 0.008, hand[1] + S * 0.030, top[0], top[1],
            max(3, int(S * 0.013)), SILH)

    # banner hanging off the pole, with a waved trailing edge
    bw, bh = S * 0.185, S * 0.100
    for row in range(int(bh)):
        t = row / bh
        cut = math.sin(t * 3.14159) * S * 0.022        # scalloped free edge
        im.rect(top[0] - bw + cut, top[1] + row, bw - cut, 1, SILH)

    return im


os.makedirs(OUT, exist_ok=True)
for size in (192, 512):
    p = os.path.join(OUT, f"icon-{size}.png")
    build(size).save(p)
    print("wrote", p)
