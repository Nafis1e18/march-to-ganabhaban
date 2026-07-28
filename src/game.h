// ============================================================
//  March to Ganabhaban â€” types and tuning
//  A belt-scroll brawler in the Cadillacs & Dinosaurs mould.
//
//  This header is the DATA layer. Frame timings, reach, damage
//  and AI pacing all live here so you can rebalance the game
//  without touching engine code. That separation is exactly how
//  the arcade originals were built: the art, the animation
//  table and the hitboxes were three different things.
// ============================================================
#pragma once
#include "raylib.h"
#include <vector>
#include <string>

// ---------- screen ----------
// 384x224 is the real CPS-1 arcade resolution, the same board
// Cadillacs & Dinosaurs ran on. We render here and scale up.
constexpr int GAME_W = 384;
constexpr int GAME_H = 224;
constexpr int SCALE  = 3;                 // window is 1152x672

// ---------- the belt ----------
// Every character has THREE coordinates:
//   x = along the street  (world space, the camera scrolls along it)
//   z = depth up/down the road   (BELT_TOP..BELT_BOT, bigger = nearer camera)
//   y = height off the ground    (jumping only)
// Screen position is:  sx = x - camX,   sy = z - y
// You can only hit someone whose z is close to yours. That single
// rule is what makes a brawler a brawler instead of a side-scroller.
constexpr float BELT_TOP = 150.0f;
constexpr float BELT_BOT = 206.0f;

// ---------- movement ----------
constexpr float WALK_X   = 1.45f;
constexpr float WALK_Z   = 0.85f;         // up/down deliberately slower than across

// Running: double-tap left or right. Arcade brawlers use a tap-tap rather than
// a hold button because you spend most of the game standing and hitting, and a
// dedicated run key would sit unused under your thumb.
constexpr float RUN_X       = 2.85f;
constexpr int   RUN_TAP_GAP = 16;         // frames allowed between the two taps
constexpr float GRAVITY  = 0.52f;
constexpr float JUMP_VY  = 7.6f;

// ---------- who is on screen ----------
// Each drawn sheet has its own pose set, so each gets its own animation
// table. Frames 0..15 happen to line up across all of them (idle, walk, run,
// jump, hurt, knocked down), but everything from 16 on is character-specific:
// the rebel has knife attacks, Hasina throws tissues, DB Harun has a gun.
// The last three have no sheet drawn yet, so they render as placeholder
// skeletons. They still need their own kind, otherwise they inherit slot 0
// and every thug on screen turns into a copy of the player.
enum CharKind {
    CK_REBEL, CK_KADER, CK_HASINA, CK_DBPOLICE,
    CK_CHHATRA, CK_POLICE, CK_JALLAD,
    CK_JITU, CK_ANTOR,          // the two allies who fight alongside you
    CK_COUNT
};

// ---------- animation ----------
enum Anim {
    A_IDLE, A_WALK, A_RUN,
    A_PUNCH1, A_PUNCH2, A_PUNCH3, A_KICK,
    A_JUMP, A_JUMPKICK,
    A_HURT, A_DOWN, A_GETUP,
    A_SPECIAL,      // knife combo / tissue attack / grenade
    A_AIM,          // ranged wind-up â€” the telegraph you must react to
    A_SHOOT,        // ranged release
    A_BLOCK,        // shield up
    A_VICTORY,
    A_COUNT
};

// Frame indexes into the sprite sheet, matching the 20 poses extracted from
// the hand-drawn sheet by tools/extract_sprites.py:
//
//   0 IDLE      1 IDLE(BREATH)  2..5 WALK 1-4   6..8 RUN 1-3
//   9 PUNCH    10 KICK         11 JUMP        12 JUMP KICK
//  13 CROUCH   14 HURT         15 KNOCKED DOWN
//  16..18 KNIFE ATTACK 1-3     19 VICTORY
//
// Where the art has only one drawing for a move, the same frame is reused with
// different timing. That is normal â€” the arcade original reused frames heavily
// too, and the feel comes from the frame DATA below, not the drawing count.
struct AnimDef { int first, count; float fps; bool loop; };

// The rebel and Kawwa Kader share the 20-pose layout:
//   0 IDLE  1 IDLE(BREATH)  2..5 WALK  6..8 RUN  9 PUNCH  10 KICK
//  11 JUMP  12 JUMP KICK   13 CROUCH  14 HURT   15 KNOCKED DOWN
//  16..18 KNIFE ATTACK     19 VICTORY
constexpr AnimDef ANIMS_STD[A_COUNT] = {
    /* IDLE     */ {  0, 2,  3.0f, true  },
    /* WALK     */ {  2, 4, 10.0f, true  },
    /* RUN      */ {  6, 3, 12.0f, true  },
    /* PUNCH1   */ {  9, 1, 12.0f, false },
    /* PUNCH2   */ {  9, 1, 12.0f, false },   // same drawing, snappier timing
    /* PUNCH3   */ { 10, 1, 10.0f, false },   // the kick, as knockdown finisher
    /* KICK     */ { 10, 1, 10.0f, false },
    /* JUMP     */ { 11, 1,  8.0f, false },
    /* JUMPKICK */ { 12, 1, 10.0f, false },
    /* HURT     */ { 14, 1, 10.0f, false },
    /* DOWN     */ { 15, 1,  8.0f, false },
    /* GETUP    */ { 13, 1,  8.0f, false },   // the crouch reads as pushing up
    /* SPECIAL  */ { 16, 3, 12.0f, false },   // knife combo
    /* AIM      */ {  9, 1, 12.0f, false },   // no gun â€” falls back to a punch
    /* SHOOT    */ {  9, 1, 12.0f, false },
    /* BLOCK    */ { 13, 1,  8.0f, false },
    /* VICTORY  */ { 19, 1,  4.0f, false },
};

// Hasina, 21 poses. Same 0..15, then:
//  16 TISSUE BLOW  17 TISSUE THROW  18 TISSUE ATTACK  19 SPEECH  20 VICTORY
// The thrown tissue is her ranged attack â€” it is genuinely how she is
// remembered, and it reads instantly.
constexpr AnimDef ANIMS_HASINA[A_COUNT] = {
    /* IDLE     */ {  0, 2,  3.0f, true  },
    /* WALK     */ {  2, 4, 10.0f, true  },
    /* RUN      */ {  6, 3, 12.0f, true  },
    /* PUNCH1   */ {  9, 1, 12.0f, false },
    /* PUNCH2   */ {  9, 1, 12.0f, false },
    /* PUNCH3   */ { 10, 1, 10.0f, false },
    /* KICK     */ { 10, 1, 10.0f, false },
    /* JUMP     */ { 11, 1,  8.0f, false },
    /* JUMPKICK */ { 12, 1, 10.0f, false },
    /* HURT     */ { 14, 1, 10.0f, false },
    /* DOWN     */ { 15, 1,  8.0f, false },
    /* GETUP    */ { 13, 1,  8.0f, false },
    /* SPECIAL  */ { 18, 1, 10.0f, false },   // tissue attack, point-blank
    /* AIM      */ { 16, 1,  8.0f, false },   // tissue blow â€” the wind-up
    /* SHOOT    */ { 17, 1, 10.0f, false },   // tissue throw â€” the release
    /* BLOCK    */ { 19, 1,  6.0f, false },   // speech podium, used as a taunt
    /* VICTORY  */ { 20, 1,  4.0f, false },
};

// DB Harun, 22 poses, built around the gun:
//   9 AIM & SHOOT  10 SHOOTING  12 JUMP SHOOT  13 CROUCH SHOOT
//  16 SHIELD BLOCK 17 SHIELD CHARGE 18 GUN UPPERCUT 19 GRENADE THROW
//  20 ROLL         21 VICTORY
constexpr AnimDef ANIMS_DBPOLICE[A_COUNT] = {
    /* IDLE     */ {  0, 2,  3.0f, true  },
    /* WALK     */ {  2, 4, 10.0f, true  },
    /* RUN      */ {  6, 3, 12.0f, true  },
    /* PUNCH1   */ { 18, 1, 12.0f, false },   // gun uppercut is his melee
    /* PUNCH2   */ { 18, 1, 12.0f, false },
    /* PUNCH3   */ { 18, 1, 10.0f, false },
    /* KICK     */ { 18, 1, 10.0f, false },
    /* JUMP     */ { 11, 1,  8.0f, false },
    /* JUMPKICK */ { 12, 1, 10.0f, false },   // jump shoot
    /* HURT     */ { 14, 1, 10.0f, false },
    /* DOWN     */ { 15, 1,  8.0f, false },
    /* GETUP    */ { 20, 1,  8.0f, false },   // rolls back to his feet
    /* SPECIAL  */ { 19, 1, 10.0f, false },   // grenade throw
    /* AIM      */ {  9, 1,  8.0f, false },
    /* SHOOT    */ { 10, 1, 10.0f, false },
    /* BLOCK    */ { 16, 1,  8.0f, false },   // shield block
    /* VICTORY  */ { 21, 1,  4.0f, false },
};

// Jitu and Antor, 19 poses. Their sheets are missing RUN 2, so every frame
// from PUNCH onward sits one index lower than the standard layout. Fixing it
// in data costs nothing; redrawing the sheets would cost an afternoon.
//   0 IDLE  1 IDLE(BREATH)  2..5 WALK  6 RUN1  7 RUN3  8 PUNCH  9 KICK
//  10 JUMP 11 JUMP KICK    12 CROUCH  13 HURT 14 KNOCKED DOWN
//  15..17 KNIFE ATTACK     18 VICTORY
constexpr AnimDef ANIMS_ALLY[A_COUNT] = {
    /* IDLE     */ {  0, 2,  3.0f, true  },
    /* WALK     */ {  2, 4, 10.0f, true  },
    /* RUN      */ {  6, 2, 11.0f, true  },
    /* PUNCH1   */ {  8, 1, 12.0f, false },
    /* PUNCH2   */ {  8, 1, 12.0f, false },
    /* PUNCH3   */ {  9, 1, 10.0f, false },
    /* KICK     */ {  9, 1, 10.0f, false },
    /* JUMP     */ { 10, 1,  8.0f, false },
    /* JUMPKICK */ { 11, 1, 10.0f, false },
    /* HURT     */ { 13, 1, 10.0f, false },
    /* DOWN     */ { 14, 1,  8.0f, false },
    /* GETUP    */ { 12, 1,  8.0f, false },
    /* SPECIAL  */ { 15, 3, 12.0f, false },
    /* AIM      */ {  8, 1, 12.0f, false },
    /* SHOOT    */ {  8, 1, 12.0f, false },
    /* BLOCK    */ { 12, 1,  8.0f, false },
    /* VICTORY  */ { 18, 1,  4.0f, false },
};

constexpr const AnimDef* CHAR_ANIMS[CK_COUNT] = {
    ANIMS_STD,       // CK_REBEL
    ANIMS_STD,       // CK_KADER
    ANIMS_HASINA,    // CK_HASINA
    ANIMS_DBPOLICE,  // CK_DBPOLICE
    ANIMS_STD,       // CK_CHHATRA
    ANIMS_STD,       // CK_POLICE
    ANIMS_STD,       // CK_JALLAD
    ANIMS_ALLY,      // CK_JITU
    ANIMS_ALLY,      // CK_ANTOR
};

// Names for the on-screen art-progress readout.
constexpr const char* CHAR_NAMES[CK_COUNT] = {
    "rebel", "kader", "hasina", "dbharun", "chhatra", "police", "jallad",
    "jitu", "antor"
};

inline const AnimDef& GetAnim(int kind, int anim) {
    return CHAR_ANIMS[(kind >= 0 && kind < CK_COUNT) ? kind : 0][anim];
}

// ---------- combat frame data ----------
// This is the heart of the feel. `active` is the window, in frames,
// where the hitbox exists. Everything before it is start-up (you are
// committed and can be punished) and everything after is recovery.
// Shrinking start-up makes a move feel snappy; growing recovery makes
// it risky. Tune these numbers, not the code.
struct AttackDef {
    int   total;          // how long the whole move lasts, in frames
    int   activeFrom;     // hitbox turns on
    int   activeTo;       // hitbox turns off
    float reach;          // how far in x
    float zTol;           // how forgiving in depth
    int   damage;
    float push;           // knockback
    bool  knockdown;      // does it put them on the floor
};

constexpr AttackDef ATK_PUNCH1   = { 13,  3,  6, 24.f, 12.f,  4, 1.4f, false };
constexpr AttackDef ATK_PUNCH2   = { 13,  3,  6, 25.f, 12.f,  4, 1.6f, false };
constexpr AttackDef ATK_PUNCH3   = { 20,  4,  8, 28.f, 13.f,  8, 4.5f, true  };
constexpr AttackDef ATK_KICK     = { 18,  5,  9, 30.f, 13.f,  6, 3.0f, false };
constexpr AttackDef ATK_JUMPKICK = { 26,  4, 18, 26.f, 13.f,  7, 3.4f, true  };

constexpr int COMBO_WINDOW = 26;   // frames to chain punch1 -> punch2 -> punch3
constexpr int HURT_STUN    = 22;
constexpr int DOWN_TIME    = 70;
constexpr int GETUP_INVULN = 40;   // mercy invulnerability while standing up

// ---------- arcade AI pacing ----------
// THE most important number in the file. Real arcade brawlers hand out
// a limited number of "attack tokens"; enemies without one circle and
// posture instead of swinging. Without this they all pile in at once
// and the game is unplayable. Raise it to make the game brutal.
constexpr int MAX_ATTACKERS = 2;

// ---------- character state ----------
enum State {
    S_IDLE, S_WALK, S_JUMP, S_ATTACK, S_HURT, S_DOWN, S_GETUP, S_DEAD
};

struct Fighter {
    float x = 0, z = 180, y = 0;     // belt coordinates
    float vy = 0;                    // vertical velocity while airborne
    int   hp = 100, maxHp = 100;
    int   facing = 1;                // +1 right, -1 left
    State state = S_IDLE;
    int   stateT = 0;                // frames spent in the current state
    Anim  anim = A_IDLE;
    float animT = 0;                 // seconds into the current animation

    // attack bookkeeping
    AttackDef atk{};
    bool  hitLanded = false;         // one hitbox connects once per swing
    int   comboStep = 0;             // 0,1,2 through the punch chain
    int   comboTimer = 0;

    int   invuln = 0;
    int   height = 30;               // drawn height in px
    int   palette = 0;               // skeleton colours, when no sheet is drawn
    int   kind = CK_REBEL;           // which sheet + animation table to use
    bool  isPlayer = false;
    bool  isAlly = false;            // Jitu / Antor: fight for you, cannot be killed
    bool  isBoss = false;            // gets a named bar, never despawns, no flinch-shove
    bool  inArena = false;           // has walked on-screen; from then on he is fenced in
    int   age = 0;                   // frames since spawn, regardless of state changes
    bool  hasToken = false;          // holds an attack token this frame
    bool  alive = true;
    int   aiTimer = 0;

    // ---- ranged enemies (DB Harun) ----
    bool  ranged = false;
    int   shootCD = 0;               // frames until he may line up another shot
    int   standOff = 0;              // preferred distance; he backs off to it

    // Feet position on screen. y is subtracted because jumping moves
    // you UP the screen without changing your depth on the belt.
    float screenY() const { return z - y; }
};

// ---------- ranged combat ----------
// A gun that just hits you would be unfair in a game with no blocking, so the
// shot is telegraphed: he stops, raises the pistol for AIM_FRAMES, and only
// then fires. That window is your cue to close the distance or change lane.
// Bullets travel along one depth line, so stepping up or down the belt dodges
// them â€” which is the whole point of having a belt.
constexpr int   AIM_FRAMES   = 34;
constexpr int   SHOOT_RECOVER= 22;
constexpr int   SHOOT_CD     = 110;   // frames between shots
constexpr float BULLET_SPEED = 3.6f;
constexpr int   BULLET_DMG   = 9;
constexpr float BULLET_ZTOL  = 9.0f;  // how close in depth a bullet must be to hit
constexpr float BULLET_YTOL  = 20.0f; // jump higher than this and it passes under

// A gun shot has no melee hitbox at all â€” the damage comes from the bullet it
// spawns. activeFrom is pushed past `total` so the melee check never fires.
constexpr AttackDef ATK_RANGED = { AIM_FRAMES + SHOOT_RECOVER, 9999, 9999,
                                   0.f, 0.f, 0, 0.f, false };

struct Projectile {
    float x = 0, z = 0, y = 0;
    float vx = 0, vy = 0;
    int   dmg = BULLET_DMG;
    int   life = 0;
    bool  fromPlayer = false;
    bool  alive = false;
    int   kind = 0;                   // 0 = bullet, 1 = tissue, 2 = grenade
};

// ---------- particles ----------
// Impact feedback. A punch that lands with no spark, no shake and no pause
// feels like the game ignored you, no matter how correct the hitbox was.
struct Particle {
    float x = 0, z = 0, y = 0;
    float vx = 0, vy = 0;
    int   life = 0, maxLife = 1;
    int   kind = 0;              // 0 spark, 1 dust, 2 rising text
    char  text[10] = {0};
    Color col{255,255,255,255};
};

void DrawParticle(const Particle& p, int camX);

// ---------- palettes: {skin, hair, shirt, trousers, accent} ----------
struct Palette { Color skin, hair, shirt, pant, accent; };
constexpr int PAL_REBEL = 0, PAL_CHHATRA = 1, PAL_POLICE = 2, PAL_JALLAD = 3;
extern const Palette PALETTES[];
extern const int PALETTE_COUNT;

// ---------- rendering (src/render.cpp) ----------
void DrawBelt(int camX, int frame);
void DrawFighter(const Fighter& f, int camX);
void DrawShadowAt(float sx, float beltY, int h, float alpha = 0.34f);
void DrawProjectile(const Projectile& p, int camX, int frame);
void DrawAimLine(const Fighter& f, int camX, int frame);

// Painted stage backgrounds (assets/bg<N>_far.png + bg<N>_near.png).
// Missing files are fine â€” DrawBelt falls back to the procedural skyline.
void LoadStageBackground(int idx);
void UnloadStageBackground();
bool StageBackgroundLoaded();


