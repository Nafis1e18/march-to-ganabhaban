# Weapon art

Weapons drop on the road when the enemy carrying one goes down. You walk over
it, press pick-up, and your whole moveset changes until you lose it.

Two kinds of art are needed:

1. **The item lying on the road** — one small sheet, all weapons together.
2. **You holding each weapon** — one 20-pose sheet per weapon, same layout as
   every other character sheet.

You do **not** need to draw a pick-up pose. The engine reuses your existing
CROUCH frame (slot 13) for bending down, which is what it already looks like.

---

## Priority

Do them in this order and stop whenever you run out of time — each one works on
its own, and anything undrawn simply never spawns.

| order | weapon | dropped by | why first |
|---|---|---|---|
| 1 | **bamboo lathi** | Chhatra League cadre | the iconic July weapon, and the most common drop |
| 2 | **riot shield** | riot police | blocks bullets, changes how you fight DB Harun |
| 3 | **pistol** | DB Harun | strongest, scarcest, limited ammo |

---

## Prompt 1 — the pickup items

One image, all the weapons lying on the ground, seen from the same side-on
angle as the game.

```
Create a 2D pixel-art item sprite sheet in the exact visual style of the 1993
Capcom arcade game "Cadillacs and Dinosaurs" (CPS-1 hardware): hard black
outlines, flat cel shading, limited palette, chunky readable pixels, no
anti-aliasing, no soft airbrushed gradients.

CONTENT: six weapon pickups lying on the ground, seen from the side, as they
would appear dropped on a street in a side-scrolling beat-em-up.

Arrange them in ONE ROW, evenly spaced, with a caption under each:
BAMBOO LATHI, RIOT SHIELD, PISTOL, BRICK, MOLOTOV, FIRST AID

Descriptions:
- BAMBOO LATHI: a long bamboo stick, natural yellow-tan with darker nodes,
  lying flat at a slight angle.
- RIOT SHIELD: a transparent polycarbonate riot shield, pale blue-grey tint,
  dark frame and handle, lying face up.
- PISTOL: a compact black service pistol lying flat, side view.
- BRICK: a single red clay brick, chipped edges.
- MOLOTOV: a glass bottle of amber liquid with a cloth wick in the neck.
- FIRST AID: a small white-and-red medical pouch with a red cross.

STRICT REQUIREMENTS:
- Plain flat WHITE background. No gradient, no texture, no colour, no scenery.
- No hands, no characters, no ground shadow, no scenery under the items.
- All six drawn at consistent relative scale, as if lying on the same street.
- Each item completely separate from the others, with clear empty space between.
- Captions in plain uppercase text about 10 pixels BELOW each item, never
  touching the artwork.
- Do NOT use pure white or pure neutral grey anywhere ON an item — tint light
  colours warm (cream, tan) or cool (blue-grey, steel).
- No motion lines, no sparkles, no border.

Image size 1536x512.
```

---

## Prompts 2–4 — the hero holding each weapon

Same 20-pose layout as every character sheet, so it drops in with no new code.
Attach the same reference photo you used for the hero.

### Bamboo lathi

```
Create a 2D pixel-art character sprite sheet in the exact visual style of the
1993 Capcom arcade game "Cadillacs and Dinosaurs" (CPS-1 hardware): hard black
outlines, flat cel shading, limited palette, chunky readable pixels, no
anti-aliasing, no soft airbrushed gradients.

CHARACTER: A Bangladeshi student protester, based on the face in the attached
photo. Cream half-sleeve shirt, blue jeans, brown boots, green headband with a
red circle. HE IS HOLDING A LONG BAMBOO LATHI (natural yellow-tan bamboo stick
with darker nodes) IN EVERY SINGLE POSE, including while idle, walking,
running, jumping, hurt and knocked down.

SHEET LAYOUT — 20 poses in 3 rows, with a small caption under each:
Row 1 (9 poses): IDLE, IDLE (BREATHING), WALK 1, WALK 2, WALK 3, WALK 4,
                 RUN 1, RUN 2, RUN 3
Row 2 (7 poses): STICK JAB, STICK OVERHEAD, JUMP, JUMP STRIKE, CROUCH, HURT,
                 KNOCKED DOWN
Row 3 (4 poses): STICK SWING 1, STICK SWING 2, STICK SWING 3, VICTORY

STRICT REQUIREMENTS:
- Plain flat WHITE background. No gradient, no texture, no colour, no scenery.
- Every pose faces RIGHT, side-on three-quarter view.
- The bamboo stick is visible and held in the hand in all 20 poses.
- Identical body size and camera distance in all 20 poses.
- All feet rest on the same horizontal ground line.
- Clear empty space between each pose and between rows.
- Captions in plain uppercase text about 10 pixels BELOW each pose, never
  touching the artwork.
- Do NOT use pure white or pure neutral grey anywhere ON the character — tint
  light colours warm (cream, tan) or cool (blue-grey, steel).
- No speech bubbles, no motion arrows, no lines linking poses, no border.
- Full-body character in every pose, nothing cropped.

Image size 1536x1024.
```

### Riot shield

Same prompt, with these lines swapped:

```
... HE IS CARRYING A TRANSPARENT RIOT SHIELD (pale blue-grey polycarbonate,
dark frame) ON HIS LEFT ARM IN EVERY SINGLE POSE.

Row 2 (7 poses): SHIELD BASH, SHIELD BLOCK, JUMP, JUMP BASH, CROUCH BEHIND
                 SHIELD, HURT, KNOCKED DOWN
Row 3 (4 poses): SHIELD CHARGE 1, SHIELD CHARGE 2, SHIELD CHARGE 3, VICTORY
```

### Pistol

```
... HE IS HOLDING A COMPACT BLACK SERVICE PISTOL IN HIS RIGHT HAND IN EVERY
SINGLE POSE.

Row 2 (7 poses): AIM, FIRING (with muzzle flash), JUMP, JUMP SHOOT, CROUCH
                 SHOOT, HURT, KNOCKED DOWN
Row 3 (4 poses): PISTOL WHIP 1, PISTOL WHIP 2, PISTOL WHIP 3, VICTORY
```

---

## Importing

```sh
python tools/extract_sprites.py "assets/raw_drawings/ITEMS.png"        "assets/items.png"
python tools/extract_sprites.py "assets/raw_drawings/HERO_LATHI.png"   "assets/rebel_lathi.png"
python tools/extract_sprites.py "assets/raw_drawings/HERO_SHIELD.png"  "assets/rebel_shield.png"
python tools/extract_sprites.py "assets/raw_drawings/HERO_PISTOL.png"  "assets/rebel_pistol.png"
```

The items sheet is not character-sized, so it needs the scale override:

```sh
python tools/extract_sprites.py "assets/raw_drawings/ITEMS.png" "assets/items.png" --items
```

---

## How it will work in game

- An enemy carrying a weapon **drops it where he falls**. It stays on the road.
- Walk over it and press the pick-up button — the engine plays your CROUCH
  frame, so no new art is needed for the grab.
- While armed, your character `kind` switches to the weapon's sheet. Every
  animation, hitbox and reach comes from that sheet's own frame data, so a
  lathi genuinely out-ranges fists and a shield genuinely blocks.
- Weapons have limited uses and are dropped when you are knocked down, so they
  are a temporary advantage rather than a permanent upgrade.
