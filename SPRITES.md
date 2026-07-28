# Drawing the characters

Everything about how your art gets into the game. Read this before you draw
anything — the geometry has to match or the engine can't use it.

Regenerate the templates any time with:

```sh
python tools/make_sprite_template.py
```

---

## The sheet

| | |
|---|---|
| Frame size | **64 × 64 px** |
| Layout | **8 columns × 4 rows**, read left-to-right, top-to-bottom |
| Full sheet | **512 × 256 px** |
| Feet baseline | **y = 60** inside each cell (the red line) |
| Body centre | **x = 26** inside each cell (the blue line) |
| Character height | **~52 px** — head top to feet |
| Format | PNG with **transparency** |

Two files are generated into `assets/`:

- **`_guide_character.png`** — the numbered grid with proportion lines. Keep this
  as a **background layer** in your paint program. Do not draw on it.
- **`_blank_character.png`** — empty, correct size. **Draw here.**

Save your finished work as `assets/rebel.png`, `assets/punk.png`,
`assets/police.png`, `assets/heavy.png`, `assets/boss.png`.

---

## Three rules that matter more than the art

**1. Always draw facing RIGHT.** The engine mirrors the sprite for left-facing.
If you draw a left-facing frame it will appear backwards in game.

**2. Feet always on the red baseline.** This is the single biggest cause of
janky hand-drawn animation. If the feet drift up and down between frames, the
character appears to bounce and slide. Head height *should* change — that's the
bob. Feet should not, except when jumping or knocked down.

**3. Keep the body on the blue centre line.** Punches and kicks extend to the
**right** of it, into the empty space. That space is deliberate — that's why the
centre line is at x=26 and not x=32.

The faint horizontal lines are head / chin / shoulder / hip / knee. Keeping
those consistent across 29 frames is what makes it read as one character
instead of 29 different people.

---

## Frame map

The green stripe is **tier 1**. Draw those nine frames first — the game is fully
playable with only those, and everything else falls back to the placeholder
skeleton until you paint it. You can ship at any stage of finishing.

| Cells | Animation | Tier | What it is |
|---|---|---|---|
| 0–1 | `idle` | **1** | Standing, breathing. Loops. |
| 2–5 | `walk` | **1** | One full stride. Loops. Frame 2 and 4 are the contact poses. |
| 6–7 | `punch1` | **1** | Jab. Frame 6 winds up, frame 7 is the connect. |
| 8 | `hurt` | **1** | Recoiling backwards. |
| 9–10 | `down` | **1** | Falling, then flat on the road. |
| 11–12 | `punch2` | 2 | Cross, with the other arm. |
| 13–15 | `punch3` | 2 | Hook. Bigger, knocks them down. |
| 16–18 | `kick` | 2 | Chamber, extend, recover. |
| 19–20 | `jump` | 2 | Rising, falling. |
| 21–22 | `jumpkick` | 2 | Leg locked out in the air. |
| 23–24 | `getup` | 3 | Pushing up off the floor. |
| 25 | `grab` | 3 | Holding someone by the collar. |
| 26–28 | `throw` | 3 | Swing them over and down. |

**The connect frame is everything.** In `punch1`, frame 7 is live for 4 frames
(`ATK_PUNCH1` in `src/game.h` says `activeFrom 3, activeTo 6`). Draw it with the
arm fully extended and the body committed. That one drawing carries the punch.

---

## Timing lives in code, not in the drawing

You draw *poses*. How long each pose is held is data in `src/game.h`:

```cpp
constexpr AttackDef ATK_PUNCH1 = { 13,  3,  6, 24.f, 12.f,  4, 1.4f, false };
//                        total ─┘   │   │    │     │     │    │      └ knockdown?
//                      start-up ────┘   │    │     │     │    └ knockback
//                    hitbox ends ───────┘    │     │     └ damage
//                            reach in x ─────┘     └ depth tolerance
```

So if a punch feels sluggish, don't redraw it — lower `activeFrom`. If it feels
like it whiffs, raise `reach`. This is exactly how Capcom worked: the art, the
animation table, and the hitboxes were three separate things, edited by three
different people.

---

## Wiring your sheet in

Not built yet — this is the next piece of code. When it's in, it will be one
call per character, and **per-animation**: if you've painted `walk` but not
`kick`, walk uses your art and kick keeps using the placeholder skeleton.

```cpp
LoadCharacterSheet(PAL_REBEL, "assets/rebel.png");
```

---

## Practical advice for 29 frames on a deadline

- **Draw `idle` first, then `walk` frame 2.** Every other frame is a
  modification of those two. Don't start with the hard poses.
- **Copy-paste-modify.** `punch2` is `punch1` with the arms swapped. `hurt` is
  `idle` leaning back. Real animators do exactly this.
- **Silhouette test.** Fill a frame solid black. If you can't tell what the
  character is doing, neither can a player at 60fps. Fix the pose, not the shading.
- **Flat colours, hard edges.** The arcade original had 16 colours per sprite.
  Soft airbrushed shading will look wrong next to chunky pixel backgrounds, and
  it takes four times as long.
- **The enemies can be recolours.** `punk`, `police` and `heavy` can start as
  the same body with different clothes and a different palette. That's what the
  original did too.
