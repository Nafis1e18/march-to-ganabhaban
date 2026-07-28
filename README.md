# March to Ganabhaban

A belt-scroll arcade brawler about the July 2024 uprising, built in C++ with
raylib in the mould of *Cadillacs and Dinosaurs* â€” the game everyone in
Bangladesh remembers as **Mustafa**.

Four stages. Five bosses. Three lives. You fight alongside two allies, Jitu and Antor.

All in-game text is Bangla.

Rendered at **384Ã—224**, the real CPS-1 arcade resolution the original ran on.

---

## Build and play

```sh
mingw32-make run
```

That's it â€” raylib is vendored in `vendor/raylib`, already compiled against
your toolchain. No install step.

| Action | Keyboard | Touch |
|---|---|---|
| Move (8 directions) | Arrows / WASD | left thumb pad |
| Punch â€” three-hit combo | `J` or `Z` | **HIT** |
| Jump (`J` in the air = jump kick) | `K` or `X` | **JUMP** |
| Reload art without rebuilding | `R` | â€” |
| Quit | `Esc` | â€” |

Touch controls appear automatically the first time you touch the screen, so
they never clutter a keyboard game.

---

## How it plays

You walk **up and down the road as well as along it**. You can only hit someone
at a similar depth â€” that single rule is what makes this a brawler rather than
a side-scroller, and it's why bullets can be dodged by stepping into another
lane instead of jumping.

The camera **locks** when a wave arrives and won't scroll on until the street is
clear. Then **GO â†’** appears.

Punch three times in a row for jab â†’ cross â†’ kick; the third hit knocks down.
`J` while airborne is a jump kick.

**DB Harun shoots.** He hangs back at a firing distance rather than closing in,
stops, raises the pistol for 34 frames with a red line down your lane, then
fires. That telegraph is your cue â€” jump it, or change lane.

**Jallad does not flinch.** Super armour: he keeps swinging through your
punches, so you cannot stunlock him the way you can a Chhatra League cadre.

---

## The stages

| # | Stage | Enemies | Boss |
|---|---|---|---|
| 1 | SHAHBAGH | Chhatra League | THE RINGLEADER |
| 2 | RAMPURA | + riot police | **JALLAD** |
| 3 | UTTARA | + DB Harun | **DB HARUN** |
| 4 | JATRABARI | + Jallad as a regular enemy | **KAWWA KADER** |
| 5 | GANABHABAN | everything at once | **SHEIKH HASINA** |

Difficulty climbs through `hpMul`, `dmgMul`, `spdMul` and â€” most importantly â€”
`maxAttackers`, which rises from 2 to 3. Hasina keeps her distance and throws
tissues at you.

Lose all your health and you lose a life. Lose three and the run ends.

---

## Rebalancing

**Everything is data.** Never edit the engine to change how the game feels.

- **[src/stages.h](src/stages.h)** â€” enemy stats, wave composition, stage
  difficulty, boss assignments, scoring.
- **[src/game.h](src/game.h)** â€” frame data for the player's moves, the belt
  geometry, jump height, gravity.

The single most important number in the whole game is **`maxAttackers`**. Real
arcade brawlers hand out a limited number of "attack tokens"; enemies without
one circle and posture instead of swinging. Raise it and the game becomes
brutal, lower it and it becomes a walkover. That one dial matters more than
every health value combined.

Attack timings look like this:

```cpp
constexpr AttackDef ATK_PUNCH1 = { 13,  3,  6, 24.f, 12.f,  4, 1.4f, false };
//                        total â”€â”˜   â”‚   â”‚    â”‚     â”‚     â”‚    â”‚      â”” knockdown?
//                      start-up â”€â”€â”€â”€â”˜   â”‚    â”‚     â”‚     â”‚    â”” knockback
//                    hitbox ends â”€â”€â”€â”€â”€â”€â”€â”˜    â”‚     â”‚     â”” damage
//                            reach in x â”€â”€â”€â”€â”€â”˜     â”” depth tolerance
```

If a punch feels sluggish, lower `activeFrom`. If it whiffs, raise `reach`. The
art, the animation table and the hitboxes are three separate things â€” exactly
how Capcom built the original.

---

## Art pipeline

Characters are hand-drawn 20-pose sheets, cut up automatically:

```sh
python tools/extract_sprites.py "art_source/raw_drawings/JALLAD.png" "assets/jallad.png"
python tools/prepare_bg.py          # backgrounds -> far/near parallax layers
python tools/make_sprite_template.py # regenerate the drawing template
```

See **[ENEMY_ART.md](ENEMY_ART.md)** for the full spec and the AI prompts that
produce importable sheets.

Two things govern whether a sheet imports cleanly: a **plain white background**
(the extractor separates character from background by saturation) and **no pure
white or grey on the character itself** (it would be keyed out â€” tint light
colours warm or cool).

`assets/` is only ~1.5 MB and is the entire phone download. The 34 MB of
originals live in `art_source/` and are never shipped.

---

## Testing

The game can play itself:

```sh
build/game.exe --demo --god --debug --fast
```

| Flag | Effect |
|---|---|
| `--demo` | a bot plays: closes on the nearest enemy, lines up the lane, swings |
| `--god` | keeps health topped up, to reach the late stages |
| `--debug` | overlays wave/flow state and logs every transition to stdout |
| `--fast` | uncaps the framerate â€” a full 5-stage run takes seconds |
| `--stage N` | start on stage N |
| `--shot N FILE` | capture frame N and exit |

Combat is frame-counted rather than delta-timed, so `--fast` changes how long a
test takes but never how the game plays.

A verified full run looks like this â€” five stages, five bosses, victory:

```
f40     PLAY     stage1 wave0/3 boss0 lives3 score0
f4466   PLAY     stage1 wave3/3 boss1 lives3 score3430
f6016   CLEAR    stage1 wave3/3 boss1 lives3 score8820
...
f75226  CLEAR    stage5 wave3/3 boss1 lives3 score197730
f75320  VICTORY  stage5 wave3/3 boss1 lives3 score227730
```

---

## Files

```
src/game.h        types, belt geometry, player frame data, animation tables
src/stages.h      ALL balance: enemy stats, waves, stages, scoring
src/main.cpp      state machine, wave flow, AI, player control
src/render.cpp    parallax backgrounds, placeholder skeleton renderer, particles
src/sprites.cpp   sheet loading, per-animation fallback
src/audio.cpp     synthesised SFX and music â€” no sound files on disk
tools/            sprite extraction, background prep, drawing templates
assets/           what ships: packed sheets + background layers (~1.5 MB)
art_source/       the originals and templates â€” never shipped
```

Audio is generated at start-up: no `.wav` files, nothing to license, and the
music transposes up a semitone each stage so the difficulty is audible.

