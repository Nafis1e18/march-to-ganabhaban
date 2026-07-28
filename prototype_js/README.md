# March to Ganabhaban

A pixel-art side-scrolling beat-'em-up about the July uprising, in the spirit of
*Mustafa*. You march, you jump the barricades, you break the line, and at the end
of every stage a minister blocks the road. Five stages, five bosses, one gate.

Runs in any browser and installs to an Android home screen as a real fullscreen
app. No engine, no build step, no dependencies — open the folder and play.

---

## Run it right now

Any static server works. From this folder:

```sh
python -m http.server 8000
```

then open <http://localhost:8000>. (`npx serve` works too if you prefer Node.)

> Opening `index.html` by double-clicking mostly works, but `file://` blocks the
> service worker, so you lose offline mode and installability. Use the server.

**Controls**

| Action | Keyboard | Touch |
|---|---|---|
| Jump (hold = higher) | `Space` / `↑` / `W` | **JUMP** |
| Attack (hold to keep swinging) | `X` / `J` / `→` | **HIT** |
| Hold back / brake | `←` / `A` / `Shift` | **◀** |
| Mute music / all sound | `M` / `N` | — |

You march forward automatically. Jumping is variable-height with coyote time and
input buffering, so a slightly-late tap still works — it should feel forgiving on
a touch screen.

---

## Get it on your Android phone

The service worker needs HTTPS (or localhost), so a plain LAN IP will play but
won't install. Easiest permanent route, and it gives you a QR code for the judges:

```sh
git init && git add -A && git commit -m "March to Ganabhaban"
gh repo create march-to-ganabhaban --public --source=. --push
```

Then **Settings → Pages → Deploy from branch → main / root**. You get
`https://<you>.github.io/march-to-ganabhaban/`.

On the phone, open that URL → Chrome menu → **Add to Home screen**. It installs
with the icon, launches fullscreen with no browser chrome, and works offline
afterwards.

### If you need an actual `.apk` file

Some hackathons want a file, not a link. Go to <https://www.pwabuilder.com>,
paste your Pages URL, choose **Android → Generate**. It returns a signed APK.
No Android Studio, no SDK, no Gradle.

---

## Changing the game

Almost everything you'd want to change lives in **`js/data.js`**. You should not
need to touch the engine.

**Rename a boss** — one line each. The five bosses are ministers by portfolio
plus a final boss; `name` is the big text, `title` the small line under it:

```js
boss: { name:'THE HOME MINISTER', title:'keeper of the batons',
        hp:14, pal:'police', prop:'baton', scale:1.35, speed:0.8 },
```

**Make it easier or harder** — in `CFG`: `jumpPower`, `gravity`, `attackReach`,
`hurtInvuln`, `maxHearts`. Per stage: `len`, `density`, `boss.hp`.

**Add a stage** — copy a `STAGES` entry and append it. The stage count, progress
bar, and score scaling all read from the array length, so nothing else changes.

**Recolour a character** — `PAL` maps a name to `[skin, hair, shirt, trousers, accent]`.

---

## Using your own painted art

The game ships with no image files: characters are a small skeleton (hip,
shoulder, knee, elbow) posed per frame, which is why every character animates
from one system. When your paintings are ready you can swap them in without
touching game logic.

1. Save a PNG strip to `assets/rebel.png` — frames in one row, all the same size.
   Suggested 24×32 per frame, in this order:
   `run ×4 | jump | fall | attack ×3 | hurt | down`
2. At the bottom of `js/art.js`:
   ```js
   SPRITES.enabled = true;
   SPRITES.load('rebel',  'assets/rebel.png');
   SPRITES.load('police', 'assets/police.png');
   ```
3. Add the new files to `FILES` in `sw.js` and bump `CACHE` to `mtg-v2`.

Anything you haven't painted yet keeps using the drawn skeleton, so the game
never breaks half-finished. **Do the art last** — it's the one part that can be
cut if you run out of time.

---

## Tests

```sh
node tools/headless_test.js
```

Stubs the DOM and canvas, then plays the whole game with a bot: all five stages,
the win screen, the death path, high-score persistence, and the final boss's
pattern rotation and phase change. Catches runtime errors without a browser.
16 checks, ~40s.

```sh
python tools/make_icons.py     # regenerate the PWA icons (stdlib only, no Pillow)
```

---

## Files

```
index.html              shell, touch buttons, rotate-to-landscape nag
manifest.webmanifest    PWA metadata (landscape, fullscreen)
sw.js                   offline cache — bump CACHE when you change files
js/data.js              ALL tuning: stages, bosses, physics, scoring
js/art.js               skeleton renderer, parallax backgrounds, sprite override
js/audio.js             synthesised SFX + a marching music loop
js/game.js              state machine, physics, combat, boss AI
js/main.js              canvas scaling, input, game loop, HUD
tools/                  icon generator + headless test
```

The world renders into a 320×180 buffer that gets scaled up with smoothing off,
so it stays chunky pixel art. The HUD is then drawn at the screen's real
resolution on top, so text stays sharp instead of turning into blurry 4× mush.

---

## A note on the subject matter

The bosses are satirical caricatures of political offices, and they are defeated
by being knocked down — no gore, nothing graphic. That's a deliberate choice: it
reads as political cartooning rather than shock, and it keeps you clear of the
content rules most hackathons have. All the names are one-line edits in
`js/data.js` if your organisers want something different.
