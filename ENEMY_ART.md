# Making the enemy sheets

Everything you need to generate the remaining characters so they drop straight
into the game. The rules below are not style preferences — each one comes from
something that actually broke or worked while importing your first four sheets.

---

## The 20 poses, in this exact order

Lay them out in **3 rows: 9 / 7 / 4**, with a caption under each pose.

| # | Row | Pose | What the engine uses it for |
|---|---|---|---|
| 0 | 1 | IDLE | standing, loops with 1 |
| 1 | 1 | IDLE (BREATHING) | second idle frame |
| 2 | 1 | WALK 1 | walk loop |
| 3 | 1 | WALK 2 | walk loop |
| 4 | 1 | WALK 3 | walk loop |
| 5 | 1 | WALK 4 | walk loop |
| 6 | 1 | RUN 1 | charging at you |
| 7 | 1 | RUN 2 | ” |
| 8 | 1 | RUN 3 | ” |
| 9 | 2 | PUNCH | his main attack, the connect frame |
| 10 | 2 | KICK | his heavy attack / knockdown |
| 11 | 2 | JUMP | airborne |
| 12 | 2 | JUMP KICK | air attack |
| 13 | 2 | CROUCH | doubles as getting back up |
| 14 | 2 | HURT | recoiling from your punch |
| 15 | 2 | KNOCKED DOWN | flat on the road |
| 16 | 3 | WEAPON ATTACK 1 | his signature move, wind-up |
| 17 | 3 | WEAPON ATTACK 2 | ” mid |
| 18 | 3 | WEAPON ATTACK 3 | ” connect |
| 19 | 3 | VICTORY | plays when he knocks you down |

Slots 16–18 are whatever that enemy's signature is — a stick swing, a shield
bash, a baton combo. Keep them in slots 16–18 and the engine picks them up with
no code change.

---

## Six rules that decide whether the import works

**1. Flat, neutral, light background.** White or light grey. The importer
separates your character from the background by *saturation* — background
pixels are neutral (red = green = blue) and bright. A red or blue or gradient
backdrop breaks this completely. Your first batch had red backgrounds and I
could not use any of them; the one on a plain light background gave all 20
poses first try.

**2. Never colour the character pure white or pure grey.** This is the same
rule from the other side. Your rebel's shirt survived because it is *cream* —
warm, slightly yellow. A pure-white shirt would be read as background and
punched full of holes. Tint every light colour warm (cream, sand, tan) or cool
(blue-grey, steel). No `#FFFFFF`, no `#CCCCCC`.

**3. Every pose faces RIGHT.** The engine mirrors sprites for left-facing. A
left-facing drawing appears backwards in game.

**4. Identical body scale in every pose.** Same camera distance throughout. The
importer measures the median height across all poses and applies **one** scale
factor to all of them — so if one pose is drawn bigger, that character grows
mid-animation.

**5. Feet on a consistent ground line.** Each pose sits on the ground at the
same height. Drifting feet make the character bounce while walking.

**6. Leave a clear gap between the artwork and its caption.** About 8–10px.
Your sheets had 5px, which works, but tighter than that and the caption gets
absorbed into the sprite and ends up baked into the frame in game.

Also: no speech bubbles, no motion arrows, no lines connecting poses, no
decorative frame around the sheet. Loose bits like a thrown weapon in mid-air
are fine — the importer discards them, because projectiles are drawn by the
engine.

---

## The prompt

Copy this, fill in the two bracketed parts, and keep everything else identical
to what produced your working sheets.

```
Create a 2D pixel-art character sprite sheet in the exact visual style of the
1993 Capcom arcade game "Cadillacs and Dinosaurs" (CPS-1 hardware): hard black
outlines, flat cel shading, limited palette, chunky readable pixels, no
anti-aliasing, no soft airbrushed gradients.

CHARACTER: [DESCRIBE YOUR ENEMY HERE — build, clothing, colours, weapon]

SHEET LAYOUT — 20 poses arranged in 3 rows, with a small caption under each:
Row 1 (9 poses): IDLE, IDLE (BREATHING), WALK 1, WALK 2, WALK 3, WALK 4,
                 RUN 1, RUN 2, RUN 3
Row 2 (7 poses): PUNCH, KICK, JUMP, JUMP KICK, CROUCH, HURT, KNOCKED DOWN
Row 3 (4 poses): [WEAPON] ATTACK 1, [WEAPON] ATTACK 2, [WEAPON] ATTACK 3,
                 VICTORY

STRICT REQUIREMENTS:
- Plain flat WHITE background. No gradient, no texture, no colour, no scenery.
- Every pose faces RIGHT, side-on three-quarter view.
- Identical body size and camera distance in all 20 poses.
- All feet rest on the same horizontal ground line.
- Clear empty space between each pose and between rows.
- Captions in plain uppercase text, placed about 10 pixels BELOW each pose,
  never touching the artwork.
- Do NOT use pure white or pure neutral grey anywhere ON the character —
  tint light colours warm (cream, tan) or cool (blue-grey, steel).
- No speech bubbles, no motion arrows, no lines linking poses, no border.
- Full-body character in every pose, nothing cropped.

Image size 1536x1024.
```

---

## The three enemies the game needs now

These are the placeholder skeletons in game right now, labelled `chhatra`,
`police` and `heavy` in the corner readout. Do them in that order — the
Chhatra League cadre appears far more often than the other two.

| slot | who | health | signature (slots 16–18) |
|---|---|---|---|
| `chhatra` | Chhatra League cadre | low, dies fast, comes in numbers | stick swing |
| `police` | riot police with shield | medium, sometimes blocks | shield bash |
| `jallad` | the executioner — heavy enforcer | high, does not flinch | overhead cleaver swing |

---

## Importing a finished sheet

```sh
python tools/extract_sprites.py "assets/raw_drawings/CHHATRA.png" "assets/chhatra.png"
python tools/extract_sprites.py "assets/raw_drawings/POLICE.png"  "assets/police.png"
python tools/extract_sprites.py "assets/raw_drawings/JALLAD.png"  "assets/jallad.png"
```

Then check two things it writes into `art_source/`:

- **`_debug_cuts.png`** — red boxes over the original. There must be exactly 20,
  one per pose, none merged and none around a caption.
- **`_verify_align.png`** — the packed frames over the alignment guide. Every
  character's feet should sit on the red line.

If both look right, run the game. The sheet loads automatically and the corner
readout will change that enemy from `-` to `20`.

**Press `R` in game to reload art without rebuilding.**

---

## Later, if you have time

`boss` slots are free for more ministers. Any new character following this same
20-pose layout needs only one line in `src/main.cpp` to appear — the animation
table, the extraction and the fallback all already handle it.
