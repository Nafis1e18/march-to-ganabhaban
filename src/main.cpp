// ============================================================
//  March to Ganabhaban â€” belt-scroll brawler
//
//  Five stages, each a run of camera-locked waves ending in a
//  boss. Three lives. Difficulty climbs stage by stage.
//
//  Everything is FRAME based at a locked 60fps, exactly like the
//  arcade original. A move that is "13 frames" is 13 frames on
//  every machine. Never make combat depend on delta time â€” that
//  is how you get a game that feels different on every laptop.
// ============================================================
#include "game.h"
#include "stages.h"
#include "sprites.h"
#include "audio.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#if defined(PLATFORM_WEB)
  #include <emscripten/emscripten.h>
#endif

// ---------- small helpers ----------
static void DrawTextSh(const char* t, int x, int y, int size, Color c) {
    DrawText(t, x + 1, y + 1, size, { 0, 0, 0, 200 });
    DrawText(t, x, y, size, c);
}
static void DrawTextC(const char* t, int cx, int y, int size, Color c) {
    DrawTextSh(t, cx - MeasureText(t, size) / 2, y, size, c);
}
static float Clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

static bool g_debug = false;   // --debug: overlay the wave/flow state

// ============================================================
//  Touch controls
//  Positions are in the 384x224 game space; raylib reports touches
//  in window pixels, so everything is divided by SCALE. The UI only
//  appears once a real touch has happened, so it never clutters the
//  screen for someone playing on a keyboard.
// ============================================================
static bool g_touchUI = false;
static bool g_tPunchPrev = false, g_tJumpPrev = false;

struct Pad { float x, y, r; };
static constexpr Pad PAD_DIR   = { 46.0f, 172.0f, 30.0f };
static constexpr Pad PAD_PUNCH = { 322.0f, 190.0f, 24.0f };
static constexpr Pad PAD_JUMP  = { 352.0f, 146.0f, 22.0f };

static bool TouchIn(const Pad& p, float tx, float ty) {
    float dx = tx - p.x, dy = ty - p.y;
    return dx * dx + dy * dy <= p.r * p.r * 1.6f;   // generous, fingers are fat
}

static void ReadTouch(bool& L, bool& R, bool& U, bool& D, bool& punch, bool& jump,
                      bool& any) {
    int n = GetTouchPointCount();
    bool hitPunch = false, hitJump = false;
    for (int i = 0; i < n; i++) {
        Vector2 t = GetTouchPosition(i);
        float tx = t.x / SCALE, ty = t.y / SCALE;
        g_touchUI = true;
        any = true;
        if (TouchIn(PAD_DIR, tx, ty)) {
            float dx = tx - PAD_DIR.x, dy = ty - PAD_DIR.y;
            if (fabsf(dx) > 7.0f) { L |= dx < 0; R |= dx > 0; }
            if (fabsf(dy) > 7.0f) { U |= dy < 0; D |= dy > 0; }
        }
        else if (TouchIn(PAD_PUNCH, tx, ty)) hitPunch = true;
        else if (TouchIn(PAD_JUMP,  tx, ty)) hitJump = true;
    }
    // press-edge, so holding the button does not machine-gun attacks
    punch |= (hitPunch && !g_tPunchPrev);
    jump  |= (hitJump  && !g_tJumpPrev);
    g_tPunchPrev = hitPunch;
    g_tJumpPrev = hitJump;
}

static void DrawPad(const Pad& p, const char* label, Color c, bool lit) {
    DrawCircleLines((int)p.x, (int)p.y, p.r, { c.r, c.g, c.b, 110 });
    DrawCircle((int)p.x, (int)p.y, p.r - 2, { c.r, c.g, c.b, (unsigned char)(lit ? 90 : 38) });
    if (label) DrawTextC(label, (int)p.x, (int)p.y - 4, 10, { 255, 255, 255, 190 });
}

static void DrawTouchUI() {
    if (!g_touchUI) return;
    DrawPad(PAD_DIR, nullptr, { 230, 230, 240, 255 }, false);
    // little arrows so the d-pad reads as a d-pad
    for (int i = 0; i < 4; i++) {
        float a = i * PI / 2.0f;
        DrawCircle((int)(PAD_DIR.x + cosf(a) * 18), (int)(PAD_DIR.y + sinf(a) * 18), 3,
                   { 255, 255, 255, 150 });
    }
    DrawPad(PAD_PUNCH, "HIT",  { 235, 80, 80, 255 },  g_tPunchPrev);
    DrawPad(PAD_JUMP,  "JUMP", { 90, 200, 130, 255 }, g_tJumpPrev);
}

// ============================================================
//  World
// ============================================================
enum GameState { GS_TITLE, GS_INTRO, GS_PLAY, GS_CLEAR, GS_GAMEOVER, GS_VICTORY };

struct World {
    GameState st = GS_TITLE;
    int  stateT = 0;

    Fighter player;
    std::vector<Fighter>    enemies;
    std::vector<Projectile> shots;
    std::vector<Particle>   parts;

    int   stage = 0;
    int   camX = 0;
    bool  camLock = false;
    int   camLockX = 0;

    int   waveIdx = 0;
    int   waveSpawned = 0;      // how many of this wave exist / existed
    bool  waveActive = false;
    bool  bossOut = false;
    bool  bossDead = false;
    int   goTimer = 0;          // "GO ->" flash after clearing a wave

    int   lives = START_LIVES;
    int   score = 0;
    int   hiScore = 0;

    // feel
    float shake = 0;
    int   hitStop = 0;
    float flash = 0;
    int   respawnT = 0;

    // banner
    const char* banner = nullptr;
    const char* bannerSub = nullptr;
    int  bannerT = 0;
};

static World W;

// ============================================================
//  Feel
// ============================================================
static void Spark(float x, float z, float y) {
    Particle p; p.x = x; p.z = z; p.y = y;
    p.life = p.maxLife = 14; p.kind = 0;
    W.parts.push_back(p);
}
static void Dust(float x, float z, int n) {
    for (int i = 0; i < n; i++) {
        Particle p; p.x = x + GetRandomValue(-5, 5); p.z = z; p.y = 0;
        p.vx = GetRandomValue(-6, 6) * 0.1f; p.vy = GetRandomValue(2, 8) * 0.1f;
        p.life = p.maxLife = GetRandomValue(14, 26); p.kind = 1;
        W.parts.push_back(p);
    }
}
static void PopText(float x, float z, float y, const char* s, Color c) {
    Particle p; p.x = x; p.z = z; p.y = y;
    p.vy = 0.55f; p.life = p.maxLife = 46; p.kind = 2; p.col = c;
    snprintf(p.text, sizeof(p.text), "%s", s);
    W.parts.push_back(p);
}
static void Banner(const char* t, const char* sub, int frames) {
    W.banner = t; W.bannerSub = sub; W.bannerT = frames;
}

// ============================================================
//  Fighters
// ============================================================
static const Stage& Cur() { return STAGES[W.stage]; }

static Fighter MakeFighter(int kind, float x, float z, bool elite = false) {
    const EnemyStats& s = ENEMY_STATS[kind];
    const Stage& st = Cur();
    Fighter e;
    e.kind = kind;
    e.palette = (kind == CK_JALLAD)   ? PAL_JALLAD
              : (kind == CK_POLICE)   ? PAL_POLICE
              : (kind == CK_DBPOLICE) ? PAL_POLICE
              : (kind == CK_HASINA)   ? PAL_POLICE
              : (kind == CK_KADER)    ? PAL_JALLAD
                                      : PAL_CHHATRA;
    e.x = x; e.z = z;
    e.hp = e.maxHp = (int)(s.hp * st.hpMul * (elite ? st.bossHpMul : 1.0f));
    e.height = elite ? (int)(s.height * 1.18f) : s.height;
    e.facing = -1;
    e.ranged = s.ranged;
    e.standOff = s.ranged ? GetRandomValue(95, 150) : 0;
    e.shootCD = GetRandomValue(40, 120);
    e.aiTimer = GetRandomValue(20, 80);
    return e;
}

static AttackDef AttackFor(const Fighter& f) {
    const EnemyStats& s = ENEMY_STATS[f.kind];
    const Stage& st = Cur();
    AttackDef d{};
    d.total = s.atkTotal; d.activeFrom = s.atkFrom; d.activeTo = s.atkTo;
    d.reach = s.reach; d.zTol = s.zTol;
    d.damage = (int)(s.damage * st.dmgMul);
    d.push = 2.6f; d.knockdown = false;
    return d;
}

static bool InRange(const Fighter& a, const Fighter& b, const AttackDef& d) {
    float dx = b.x - a.x;
    if (a.facing > 0 ? (dx < -6 || dx > d.reach) : (dx > 6 || dx < -d.reach)) return false;
    if (fabsf(b.z - a.z) > d.zTol) return false;      // the belt rule
    if (fabsf(b.y - a.y) > 24.0f) return false;
    return true;
}

static void StartAttack(Fighter& f, Anim a, const AttackDef& d) {
    f.state = S_ATTACK; f.anim = a; f.atk = d;
    f.stateT = 0; f.animT = 0; f.hitLanded = false;
}

static void LoseLife();

static void TakeHit(Fighter& v, const AttackDef& d, int fromFacing, bool byPlayer) {
    if (v.invuln > 0 || !v.alive) return;

    const EnemyStats& s = ENEMY_STATS[v.kind];
    v.hp -= d.damage;
    v.hitLanded = false;
    v.comboStep = 0;

    Spark(v.x, v.z, v.height * 0.55f);
    W.shake = fmaxf(W.shake, d.knockdown ? 7.0f : 4.0f);
    W.hitStop = d.knockdown ? 7 : 4;
    PlaySfx(SFX_HIT);

    if (byPlayer) {
        W.score += SCORE_HIT;
        // Bosses and armoured enemies do not get shoved around, or you could
        // push a boss into a corner and hold him there for the whole fight.
        if (!s.superArmor) v.x += d.push * fromFacing;
    } else {
        v.x += d.push * fromFacing;
    }
    v.facing = -fromFacing;

    if (v.hp <= 0) {
        v.hp = 0; v.alive = false;
        // Release the attack token, or the corpse holds a slot for the 150
        // frames it lies there and the survivors just stand around watching.
        v.hasToken = false;
        v.state = S_DOWN; v.anim = A_DOWN; v.stateT = 0; v.animT = 0;
        if (byPlayer) {
            int gain = (int)(s.score * (1.0f + W.stage * 0.35f));
            W.score += gain;
            char buf[10]; snprintf(buf, sizeof(buf), "%d", gain);
            PopText(v.x, v.z, v.height + 6.0f, buf, { 255, 214, 100, 255 });
            PlaySfx(SFX_KILL);
        }
        return;
    }

    // Super-armour enemies keep swinging through your punches. That is what
    // makes Jallad frightening rather than just slow.
    if (s.superArmor && !d.knockdown) return;

    if (d.knockdown) { v.state = S_DOWN; v.anim = A_DOWN; v.vy = 3.2f; }
    else             { v.state = S_HURT; v.anim = A_HURT; }
    v.stateT = 0; v.animT = 0;
}

static void HurtPlayer(const AttackDef& d, int fromFacing) {
    Fighter& p = W.player;
    if (p.invuln > 0 || W.respawnT > 0) return;
    p.hp -= d.damage;
    p.x += d.push * fromFacing;
    p.facing = -fromFacing;
    p.comboStep = 0;
    W.shake = 9; W.hitStop = 6; W.flash = 0.35f;
    Spark(p.x, p.z, p.height * 0.55f);
    PlaySfx(SFX_HURT);

    if (p.hp <= 0) { p.hp = 0; LoseLife(); return; }
    p.state = S_HURT; p.anim = A_HURT; p.stateT = 0; p.animT = 0;
    p.invuln = 40;
}

// ============================================================
//  Stage / wave flow
// ============================================================
static void SpawnPlayer(float x) {
    Fighter& p = W.player;
    p = Fighter{};
    p.kind = CK_REBEL; p.palette = PAL_REBEL;
    p.x = x; p.z = (BELT_TOP + BELT_BOT) * 0.5f;
    p.height = 48; p.hp = p.maxHp = 100;
    p.isPlayer = true; p.facing = 1;
    p.invuln = 70;
}

static void StartStage(int idx) {
    W.stage = idx;
    W.enemies.clear(); W.shots.clear(); W.parts.clear();
    W.waveIdx = 0; W.waveSpawned = 0; W.waveActive = false;
    W.bossOut = false; W.bossDead = false;
    W.camLock = false; W.goTimer = 0;
    SpawnPlayer(60);
    W.camX = 0;
    LoadStageBackground(Cur().bg);
    StartMusic(idx);            // a semitone up and a few bpm faster each stage
    W.st = GS_INTRO; W.stateT = 0;
}

static void StartRun() {
    W.lives = START_LIVES;
    W.score = 0;
    StartStage(0);
}

static void LoseLife() {
    W.lives--;
    W.shake = 14; W.flash = 0.8f;
    Fighter& p = W.player;
    p.state = S_DOWN; p.anim = A_DOWN; p.stateT = 0; p.animT = 0; p.vy = 3.4f;
    if (W.lives <= 0) {
        W.st = GS_GAMEOVER; W.stateT = 0;
        StopMusic(); PlaySfx(SFX_GAMEOVER);
        if (W.score > W.hiScore) W.hiScore = W.score;
    } else {
        W.respawnT = 90;   // lie there, then get back up where you fell
    }
}

// How many of the current wave are still standing.
static int AliveCount() {
    int n = 0;
    for (auto& e : W.enemies) if (e.alive) n++;
    return n;
}

static void SpawnFromWave(const WaveDef& w) {
    int alive = AliveCount();
    while (W.waveSpawned < w.total && alive < w.onceMax) {
        int kind = w.kinds[W.waveSpawned % w.kindCount];
        // Alternate the side they come from so you get flanked rather than
        // always fighting a queue arriving from the right.
        bool fromRight = (W.waveSpawned % 3 != 2);
        float x = fromRight ? W.camX + GAME_W + GetRandomValue(10, 50)
                            : W.camX - GetRandomValue(20, 50);
        float z = BELT_TOP + 6 + GetRandomValue(0, (int)(BELT_BOT - BELT_TOP - 12));
        W.enemies.push_back(MakeFighter(kind, x, z));
        W.waveSpawned++;
        alive++;
    }
}

static void SpawnBoss() {
    const Stage& s = Cur();
    Fighter b = MakeFighter(s.bossKind, W.camX + GAME_W + 40.0f,
                            (BELT_TOP + BELT_BOT) * 0.5f, s.bossElite);
    if (!s.bossElite) b.hp = b.maxHp = (int)(ENEMY_STATS[s.bossKind].hp * s.hpMul * s.bossHpMul);
    b.isBoss = true;
    W.enemies.push_back(b);
    W.bossOut = true;
    W.shake = 12;
    PlaySfx(SFX_BOSS);
    Banner(s.bossName, s.bossTitle, 140);
}

// ============================================================
//  Enemy AI
// ============================================================
// Keep a fighter inside the locked arena — but only once he has actually
// walked into it, so reinforcements can still arrive from off-screen.
// Without this a ranged enemy backing away to keep his distance strolls out
// past the edge the player is fenced behind, and the wave can never be cleared.
static void FenceIn(Fighter& e) {
    if (!W.camLock) return;
    float lo = (float)W.camX + 8.0f, hi = (float)W.camX + GAME_W - 8.0f;
    if (!e.inArena) {
        // A ranged enemy stops the instant he reaches his firing distance, so
        // if he spawned outside he can sit there forever, out of reach of a
        // player who IS fenced in — and the wave can never be cleared. The age
        // cut-off guarantees everyone is inside the fight within three seconds.
        if ((e.x > lo && e.x < hi) || e.age > 180) e.inArena = true;
        else return;
    }
    e.x = Clampf(e.x, lo, hi);
}

static void UpdateEnemy(Fighter& e, int& attackers) {
    Fighter& p = W.player;
    const EnemyStats& s = ENEMY_STATS[e.kind];
    const Stage& st = Cur();

    e.stateT++; e.age++; e.animT += 1.0f / 60.0f;
    if (e.invuln > 0) e.invuln--;
    FenceIn(e);

    if (!e.alive) {                                   // dying / dead on the road
        if (e.y > 0 || e.vy > 0) { e.y += e.vy; e.vy -= GRAVITY; if (e.y < 0) { e.y = 0; e.vy = 0; } }
        return;
    }

    float dx = p.x - e.x, dz = p.z - e.z;
    e.facing = (dx > 0) ? 1 : -1;

    if (e.state == S_HURT) {
        if (e.stateT >= HURT_STUN) { e.state = S_IDLE; e.anim = A_IDLE; e.animT = 0; }
        return;
    }
    if (e.state == S_DOWN) {
        if (e.y > 0 || e.vy > 0) { e.y += e.vy; e.vy -= GRAVITY; if (e.y < 0) { e.y = 0; e.vy = 0; } }
        if (e.stateT >= DOWN_TIME) {
            e.state = S_GETUP; e.anim = A_GETUP; e.stateT = 0; e.animT = 0;
            e.invuln = GETUP_INVULN; e.hasToken = false;
        }
        return;
    }
    if (e.state == S_GETUP) {
        if (e.stateT >= 22) { e.state = S_IDLE; e.anim = A_IDLE; e.animT = 0; }
        return;
    }
    if (e.state == S_ATTACK) {
        if (e.ranged) {
            if (e.stateT == AIM_FRAMES) {
                e.anim = A_SHOOT; e.animT = 0;
                Projectile b;
                b.x = e.x + e.facing * 14.0f;
                b.z = e.z; b.y = e.height * 0.55f;
                b.vx = BULLET_SPEED * e.facing;
                b.dmg = (int)(s.damage * st.dmgMul);
                b.life = 240; b.alive = true;
                if (e.kind == CK_HASINA) { b.kind = 1; b.vx *= 0.66f; b.vy = 1.8f; }
                else                     { b.kind = 0; }
                W.shots.push_back(b);
            }
        } else if (!e.hitLanded &&
                   e.stateT >= e.atk.activeFrom && e.stateT <= e.atk.activeTo) {
            if (InRange(e, p, e.atk)) { HurtPlayer(e.atk, e.facing); e.hitLanded = true; }
        }
        if (e.stateT >= e.atk.total) {
            e.state = S_IDLE; e.anim = A_IDLE; e.animT = 0;
            e.hasToken = false;
            e.shootCD = SHOOT_CD + GetRandomValue(-30, 50);
            e.aiTimer = GetRandomValue(24, 70);
        }
        return;
    }

    // ---------------- ranged ----------------
    if (e.ranged) {
        if (e.shootCD > 0) e.shootCD--;
        float adx = fabsf(dx), mx = 0, mz = 0;
        if (adx < e.standOff - 18)      mx = (dx > 0 ? -1.0f : 1.0f);
        else if (adx > e.standOff + 24) mx = (dx > 0 ?  1.0f : -1.0f);
        if (fabsf(dz) > 5.0f) mz = (dz > 0 ? 1.0f : -1.0f);

        e.x += mx * WALK_X * 0.55f * st.spdMul;
        e.z = Clampf(e.z + mz * WALK_Z * 0.6f, BELT_TOP, BELT_BOT);

        bool moving = (mx != 0 || mz != 0);
        e.state = moving ? S_WALK : S_IDLE;
        e.anim  = moving ? A_WALK : A_IDLE;

        bool laned = fabsf(dz) < BULLET_ZTOL;
        if (!e.hasToken && attackers < st.maxAttackers && e.shootCD == 0 && laned) {
            e.hasToken = true; attackers++;
        }
        if (e.hasToken && e.shootCD == 0 && laned && adx > 40 && adx < 265) {
            AttackDef d = AttackFor(e);
            d.total = AIM_FRAMES + SHOOT_RECOVER; d.activeFrom = 9999; d.activeTo = 9999;
            StartAttack(e, A_AIM, d);
        }
        return;
    }

    // ---------------- melee ----------------
    float want = s.reach - 4.0f;
    bool aligned = fabsf(dz) < 8.0f;
    float mx = 0, mz = 0;

    if (fabsf(dz) > 4.0f) mz = (dz > 0 ? 1.0f : -1.0f);
    if (fabsf(dx) > want) mx = (dx > 0 ? 1.0f : -1.0f);
    else if (!e.hasToken && fabsf(dx) < want - 8) mx = (dx > 0 ? -0.6f : 0.6f);

    e.x += mx * s.speed * WALK_X * 0.62f * st.spdMul;
    e.z = Clampf(e.z + mz * s.speed * WALK_Z * 0.75f * st.spdMul, BELT_TOP, BELT_BOT);

    bool moving = (mx != 0 || mz != 0);
    // Shield police put the shield up while waiting rather than shuffling in
    // the open, which is what makes them read as a wall you have to break.
    if (!moving && s.guardChance > 0 && (e.aiTimer % 90) < 40) {
        e.state = S_IDLE; e.anim = A_BLOCK;
    } else {
        e.state = moving ? S_WALK : S_IDLE;
        e.anim  = moving ? A_WALK : A_IDLE;
    }

    if (e.aiTimer > 0) e.aiTimer--;
    if (!e.hasToken && attackers < st.maxAttackers && aligned &&
        fabsf(dx) < want + 10 && e.aiTimer == 0) {
        e.hasToken = true; attackers++;
    }
    if (e.hasToken && aligned && fabsf(dx) < want + 6) {
        AttackDef d = AttackFor(e);
        Anim a = A_PUNCH1;
        // Bosses mix in their signature move; it hits harder and knocks down,
        // so there is a reason to respect them beyond a bigger health bar.
        if (e.isBoss && GetRandomValue(0, 100) < 40) {
            a = A_SPECIAL; d.damage = (int)(d.damage * 1.5f);
            d.knockdown = true; d.push = 5.0f; d.total += 8;
        } else if (GetRandomValue(0, 100) < 25) {
            a = A_KICK; d.knockdown = true; d.push = 4.2f;
        }
        StartAttack(e, a, d);
    }
}

// ============================================================
//  Player
// ============================================================
static void UpdatePlayer(bool bL, bool bR, bool bU, bool bD, bool bPunch, bool bJump) {
    Fighter& p = W.player;
    p.stateT++; p.animT += 1.0f / 60.0f;
    if (p.invuln > 0) p.invuln--;
    if (p.comboTimer > 0) p.comboTimer--; else p.comboStep = 0;

    if (W.respawnT > 0) {                     // knocked out, waiting to get up
        if (p.y > 0 || p.vy > 0) { p.y += p.vy; p.vy -= GRAVITY; if (p.y < 0) { p.y = 0; p.vy = 0; } }
        if (--W.respawnT == 0) {
            p.hp = p.maxHp; p.invuln = 90;
            p.state = S_GETUP; p.anim = A_GETUP; p.stateT = 0; p.animT = 0;
        }
        return;
    }

    bool grounded = (p.y <= 0.01f);

    if (p.state == S_IDLE || p.state == S_WALK) {
        float mx = (bR ? 1.0f : 0) - (bL ? 1.0f : 0);
        float mz = (bD ? 1.0f : 0) - (bU ? 1.0f : 0);
        if (mx != 0) p.facing = (mx > 0) ? 1 : -1;

        p.x += mx * WALK_X;
        p.z = Clampf(p.z + mz * WALK_Z, BELT_TOP, BELT_BOT);

        bool moving = (mx != 0 || mz != 0);
        if (moving && p.state != S_WALK) { p.state = S_WALK; p.anim = A_WALK; }
        if (!moving && p.state != S_IDLE) { p.state = S_IDLE; p.anim = A_IDLE; p.animT = 0; }

        if (bJump) {
            p.vy = JUMP_VY; p.state = S_JUMP; p.anim = A_JUMP;
            p.stateT = 0; p.animT = 0; Dust(p.x, p.z, 3); PlaySfx(SFX_JUMP);
        } else if (bPunch) {
            if (p.comboStep == 0)      StartAttack(p, A_PUNCH1, ATK_PUNCH1);
            else if (p.comboStep == 1) StartAttack(p, A_PUNCH2, ATK_PUNCH2);
            else                       StartAttack(p, A_PUNCH3, ATK_PUNCH3);
        }
    }
    else if (p.state == S_JUMP) {
        float mx = (bR ? 1.0f : 0) - (bL ? 1.0f : 0);
        p.x += mx * WALK_X * 0.8f;
        if (bPunch && p.anim != A_JUMPKICK) {
            StartAttack(p, A_JUMPKICK, ATK_JUMPKICK);
            p.state = S_JUMP;
        }
    }
    else if (p.state == S_ATTACK) {
        if (p.stateT >= p.atk.total) {
            p.state = S_IDLE; p.anim = A_IDLE; p.animT = 0;
            p.comboStep = std::min(p.comboStep + 1, 2);
            p.comboTimer = COMBO_WINDOW;
        }
    }
    else if (p.state == S_HURT) {
        if (p.stateT >= HURT_STUN) { p.state = S_IDLE; p.anim = A_IDLE; p.animT = 0; }
    }
    else if (p.state == S_GETUP) {
        if (p.stateT >= 22) { p.state = S_IDLE; p.anim = A_IDLE; p.animT = 0; }
    }

    if (!grounded || p.vy > 0) {
        p.y += p.vy; p.vy -= GRAVITY;
        if (p.y <= 0) {
            p.y = 0; p.vy = 0;
            Dust(p.x, p.z, 2);
            if (p.state == S_JUMP) { p.state = S_IDLE; p.anim = A_IDLE; p.animT = 0; }
        }
    }

    // one swing connects with one enemy, so a punch cannot mow down a crowd
    if (p.state == S_ATTACK && !p.hitLanded &&
        p.stateT >= p.atk.activeFrom && p.stateT <= p.atk.activeTo) {
        for (auto& e : W.enemies) {
            if (!e.alive || e.state == S_DOWN) continue;
            if (InRange(p, e, p.atk)) { TakeHit(e, p.atk, p.facing, true); p.hitLanded = true; break; }
        }
    }

    // stay inside the locked arena / never walk back off the left edge
    float leftWall = (float)W.camX + 10.0f;
    float rightWall = W.camLock ? (float)W.camX + GAME_W - 12.0f : 1e9f;
    p.x = Clampf(p.x, leftWall, rightWall);
}

// ============================================================
//  Play step
// ============================================================
static void UpdatePlay(bool bL, bool bR, bool bU, bool bD, bool bPunch, bool bJump) {
    const Stage& s = Cur();
    Fighter& p = W.player;

    UpdatePlayer(bL, bR, bU, bD, bPunch, bJump);

    int attackers = 0;
    for (auto& e : W.enemies) if (e.hasToken) attackers++;
    for (auto& e : W.enemies) UpdateEnemy(e, attackers);

    // ---- projectiles ----
    for (auto& b : W.shots) {
        if (!b.alive) continue;
        b.x += b.vx; b.y += b.vy;
        if (b.vy != 0) b.vy -= GRAVITY * 0.35f;
        if (b.y < 0) { Dust(b.x, b.z, 3); b.alive = false; continue; }
        if (--b.life <= 0) { b.alive = false; continue; }
        if (b.x < W.camX - 40 || b.x > W.camX + GAME_W + 40) { b.alive = false; continue; }
        // Lane AND height must both match, so stepping up the belt or jumping
        // are each a complete answer to being shot at.
        if (W.respawnT == 0 && p.invuln == 0 && p.state != S_DOWN &&
            fabsf(b.x - p.x) < 9.0f &&
            fabsf(b.z - p.z) < BULLET_ZTOL &&
            fabsf(b.y - (p.y + p.height * 0.5f)) < BULLET_YTOL) {
            AttackDef hit{}; hit.damage = b.dmg; hit.push = 2.2f;
            HurtPlayer(hit, (b.vx > 0) ? 1 : -1);
            b.alive = false;
        }
    }
    W.shots.erase(std::remove_if(W.shots.begin(), W.shots.end(),
        [](const Projectile& q) { return !q.alive; }), W.shots.end());

    // ---- particles ----
    for (auto& q : W.parts) { q.x += q.vx; q.y += q.vy; q.life--; }
    W.parts.erase(std::remove_if(W.parts.begin(), W.parts.end(),
        [](const Particle& q) { return q.life <= 0; }), W.parts.end());

    // ---- bodies clear off once they have lain there a while ----
    W.enemies.erase(std::remove_if(W.enemies.begin(), W.enemies.end(),
        [](const Fighter& e) { return !e.alive && e.stateT > 150; }), W.enemies.end());

    if (W.goTimer > 0) W.goTimer--;
    if (W.bannerT > 0) W.bannerT--;

    // ============ wave / boss flow ============
    if (!W.bossOut) {
        if (!W.waveActive && W.waveIdx < s.waveCount && p.x >= s.waves[W.waveIdx].atX) {
            W.waveActive = true;
            W.waveSpawned = 0;
            W.camLock = true;
            W.camLockX = W.camX;
        }
        if (W.waveActive) {
            const WaveDef& w = s.waves[W.waveIdx];
            SpawnFromWave(w);
            if (W.waveSpawned >= w.total && AliveCount() == 0) {
                W.waveActive = false;
                W.camLock = false;
                W.waveIdx++;
                W.goTimer = 150;
            }
        }
        else if (W.waveIdx >= s.waveCount && p.x >= s.endX) {
            W.camLock = true; W.camLockX = W.camX;
            SpawnBoss();
        }
    } else if (!W.bossDead) {
        bool anyBoss = false;
        for (auto& e : W.enemies) if (e.isBoss && e.alive) anyBoss = true;
        if (!anyBoss) {
            W.bossDead = true;
            W.flash = 0.9f; W.shake = 16;
            W.score += SCORE_STAGE * (W.stage + 1);
            StopMusic(); PlaySfx(SFX_CLEAR);
            W.st = GS_CLEAR; W.stateT = 0;
        }
    }

    // ============ camera ============
    if (!W.camLock) {
        int want = (int)(p.x - GAME_W * 0.38f);
        if (want > W.camX) W.camX += std::min(4, want - W.camX);
        if (W.camX < 0) W.camX = 0;
    }
}

// ============================================================
//  Rendering
// ============================================================
static void DrawHealthBarOver(const Fighter& e, int camX) {
    if (!e.alive || e.isBoss) return;
    if (e.hp >= e.maxHp) return;                    // only once he has been hit
    float sx = e.x - camX, sy = e.z - e.y - e.height - 6;
    int w = 22;
    DrawRectangle((int)sx - w / 2 - 1, (int)sy - 1, w + 2, 4, { 0, 0, 0, 170 });
    DrawRectangle((int)sx - w / 2, (int)sy, (int)(w * (e.hp / (float)e.maxHp)), 2,
                  { 230, 90, 70, 255 });
}

static void DrawWorld() {
    const Stage& s = Cur();
    DrawBelt(W.camX, W.stateT);

    std::vector<const Fighter*> order;
    order.reserve(W.enemies.size() + 1);
    order.push_back(&W.player);
    for (auto& e : W.enemies) order.push_back(&e);
    std::sort(order.begin(), order.end(),
              [](const Fighter* a, const Fighter* b) { return a->z < b->z; });
    for (auto* f : order) DrawFighter(*f, W.camX);
    for (auto& e : W.enemies) DrawHealthBarOver(e, W.camX);

    for (const auto& e : W.enemies)
        if (e.alive && e.state == S_ATTACK && e.anim == A_AIM) DrawAimLine(e, W.camX, W.stateT);
    for (const auto& b : W.shots)   DrawProjectile(b, W.camX, W.stateT);
    for (const auto& q : W.parts)   DrawParticle(q, W.camX);

    // ---- HUD ----
    DrawRectangle(6, 6, 104, 8, { 0, 0, 0, 170 });
    DrawRectangle(7, 7, (int)(102 * (W.player.hp / (float)W.player.maxHp)), 6,
                  { 214, 60, 60, 255 });
    DrawTextSh("REBEL", 7, 16, 10, RAYWHITE);
    for (int i = 0; i < W.lives - 1; i++)
        DrawRectangle(48 + i * 7, 18, 5, 5, { 255, 200, 90, 255 });

    DrawTextSh(TextFormat("%07d", W.score), GAME_W - 62, 6, 10, RAYWHITE);
    DrawTextSh(TextFormat("STAGE %d-%d", W.stage + 1, std::min(W.waveIdx + 1, s.waveCount)),
               GAME_W - 62, 17, 10, { 255, 255, 255, 160 });

    // boss bar â€” nearest boss only
    const Fighter* boss = nullptr; float best = 1e9f;
    for (const auto& e : W.enemies) {
        if (!e.alive || !e.isBoss) continue;
        float d = fabsf(e.x - W.player.x);
        if (d < best) { best = d; boss = &e; }
    }
    if (boss) {
        int bw = 210, bx = (GAME_W - bw) / 2;
        DrawRectangle(bx - 1, 30, bw + 2, 8, { 0, 0, 0, 190 });
        DrawRectangle(bx, 31, (int)(bw * (boss->hp / (float)boss->maxHp)), 6,
                      { 205, 40, 40, 255 });
        DrawTextC(s.bossName, GAME_W / 2, 20, 10, { 255, 175, 175, 235 });
    }

    // "GO ->" once a wave is cleared
    if (W.goTimer > 0 && (W.goTimer / 8) % 2) {
        DrawTextC("GO", GAME_W - 46, GAME_H / 2 - 16, 20, { 255, 225, 90, 255 });
        for (int i = 0; i < 3; i++)
            DrawTriangle({ (float)GAME_W - 34 + i * 9, (float)GAME_H / 2 + 4 },
                         { (float)GAME_W - 34 + i * 9, (float)GAME_H / 2 + 14 },
                         { (float)GAME_W - 27 + i * 9, (float)GAME_H / 2 + 9 },
                         { 255, 225, 90, 255 });
    }
    if (W.camLock && W.goTimer == 0 && !W.bossOut)
        DrawTextC("CLEAR THE STREET", GAME_W / 2, GAME_H - 16, 10, { 255, 255, 255, 120 });

    if (g_debug) {
        DrawTextSh(TextFormat("px%.0f cam%d wv%d/%d act%d spwn%d alive%d boss%d st%d",
                              W.player.x, W.camX, W.waveIdx, s.waveCount,
                              (int)W.waveActive, W.waveSpawned, (int)W.enemies.size(),
                              (int)W.bossOut, (int)W.st),
                   4, GAME_H - 12, 10, { 120, 255, 160, 255 });
        if (W.waveIdx < s.waveCount)
            DrawTextSh(TextFormat("nextWaveAtX %.0f  endX %.0f",
                                  s.waves[W.waveIdx].atX, s.endX),
                       4, GAME_H - 24, 10, { 120, 255, 160, 200 });
    }

    // boss announcement
    if (W.bannerT > 0 && W.banner) {
        float a = fminf(1.0f, W.bannerT / 30.0f);
        Color c = { 255, 255, 255, (unsigned char)(255 * a) };
        DrawTextC(W.banner, GAME_W / 2, GAME_H / 2 - 18, 20, c);
        if (W.bannerSub)
            DrawTextC(W.bannerSub, GAME_W / 2, GAME_H / 2 + 4, 10,
                      { 255, 170, 170, (unsigned char)(220 * a) });
    }
}

static void DrawOverlay() {
    const Stage& s = Cur();
    switch (W.st) {
    case GS_TITLE:
        DrawRectangle(0, 0, GAME_W, GAME_H, { 8, 10, 18, 190 });
        DrawTextC("MARCH TO", GAME_W / 2, 46, 14, { 255, 255, 255, 200 });
        DrawTextC("GANABHABAN", GAME_W / 2, 62, 28, RAYWHITE);
        DrawRectangle(GAME_W / 2 - 60, 94, 120, 2, { 31, 143, 78, 255 });
        DrawTextC("JULY 2024", GAME_W / 2, 102, 10, { 255, 209, 102, 255 });
        if ((W.stateT / 30) % 2)
            DrawTextC("PRESS J TO START", GAME_W / 2, GAME_H - 44, 12, RAYWHITE);
        DrawTextC("ARROWS / WASD move    J punch    K jump",
                  GAME_W / 2, GAME_H - 24, 10, { 255, 255, 255, 130 });
        if (W.hiScore > 0)
            DrawTextC(TextFormat("BEST %d", W.hiScore), GAME_W / 2, GAME_H - 12, 10,
                      { 255, 255, 255, 110 });
        break;

    case GS_INTRO: {
        float a = (W.stateT < 20) ? W.stateT / 20.0f
                : (W.stateT > 100) ? fmaxf(0.0f, (130 - W.stateT) / 30.0f) : 1.0f;
        DrawRectangle(0, 0, GAME_W, GAME_H, { 8, 10, 18, (unsigned char)(200 * a) });
        DrawTextC(TextFormat("STAGE %d", W.stage + 1), GAME_W / 2, 78, 11,
                  { 159, 224, 184, (unsigned char)(255 * a) });
        DrawTextC(s.name, GAME_W / 2, 92, 24, { 255, 255, 255, (unsigned char)(255 * a) });
        DrawTextC(s.sub, GAME_W / 2, 118, 10, { 255, 255, 255, (unsigned char)(180 * a) });
        break;
    }
    case GS_CLEAR:
        DrawRectangle(0, 0, GAME_W, GAME_H, { 8, 10, 18, 200 });
        DrawTextC("LINE BROKEN", GAME_W / 2, 56, 22, RAYWHITE);
        DrawTextC(TextFormat("%s has fallen", s.bossName), GAME_W / 2, 80, 10,
                  { 255, 175, 175, 230 });
        DrawTextC(TextFormat("SCORE  %d", W.score), GAME_W / 2, 108, 14,
                  { 255, 209, 102, 255 });
        if (W.stateT > 60 && (W.stateT / 30) % 2)
            DrawTextC(W.stage + 1 >= STAGE_COUNT ? "PRESS J" : "PRESS J TO MARCH ON",
                      GAME_W / 2, GAME_H - 26, 11, RAYWHITE);
        break;

    case GS_GAMEOVER:
        DrawRectangle(0, 0, GAME_W, GAME_H,
                      { 20, 6, 10, (unsigned char)std::min(210, W.stateT * 4) });
        DrawTextC("THE LINE HELD", GAME_W / 2, 62, 22, { 255, 110, 110, 255 });
        DrawTextC(TextFormat("you fell at %s", s.name), GAME_W / 2, 86, 10,
                  { 255, 255, 255, 170 });
        DrawTextC(TextFormat("SCORE  %d", W.score), GAME_W / 2, 110, 14, RAYWHITE);
        DrawTextC(TextFormat("BEST   %d", W.hiScore), GAME_W / 2, 126, 10,
                  { 255, 255, 255, 150 });
        if (W.stateT > 90 && (W.stateT / 30) % 2)
            DrawTextC("PRESS J", GAME_W / 2, GAME_H - 26, 11, RAYWHITE);
        break;

    case GS_VICTORY: {
        DrawRectangle(0, 0, GAME_W, GAME_H, { 8, 20, 14, 200 });
        DrawTextC("SHE IS GONE", GAME_W / 2, 40, 26, RAYWHITE);
        DrawTextC("5 August 2024 - the gate is open", GAME_W / 2, 66, 10,
                  { 159, 224, 184, 255 });
        DrawTextC(TextFormat("FINAL SCORE  %d", W.score), GAME_W / 2, 92, 14,
                  { 255, 209, 102, 255 });
        const char* rank = RANKS[RANK_COUNT - 1].name;
        for (int i = 0; i < RANK_COUNT; i++) if (W.score >= RANKS[i].at) { rank = RANKS[i].name; break; }
        DrawTextC(rank, GAME_W / 2, 114, 18, RAYWHITE);
        if (W.stateT > 90 && (W.stateT / 30) % 2)
            DrawTextC("PRESS J", GAME_W / 2, GAME_H - 24, 11, { 255, 255, 255, 210 });
        break;
    }
    default: break;
    }
}

// ============================================================
// Emscripten owns the event loop in the browser, so the frame body has to be a
// callback rather than a blocking while(). These live at file scope so both the
// desktop loop and the web callback can reach them.
static bool g_demo = false, g_god = false, g_fast = false;
static int  g_shotAt = -1;
static const char* g_shotFile = "shot.png";
static RenderTexture2D g_target;
static long g_frameNo = 0;
static bool g_quit = false;

static void Frame();

int main(int argc, char** argv) {
    bool& demo = g_demo; bool& god = g_god; bool& fast = g_fast;
    int&  shotAt = g_shotAt;
    int   startStage = 0;
    bool  bossNow = false;
    const char*& shotFile = g_shotFile;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--demo")) demo = true;
        else if (!strcmp(argv[i], "--god")) god = true;
        else if (!strcmp(argv[i], "--debug")) g_debug = true;
        else if (!strcmp(argv[i], "--fast")) fast = true;   // uncap fps for testing
        else if (!strcmp(argv[i], "--stage") && i + 1 < argc) startStage = atoi(argv[++i]) - 1;
        else if (!strcmp(argv[i], "--boss")) bossNow = true;   // skip straight to the boss
        else if (!strcmp(argv[i], "--shot") && i + 1 < argc) {
            shotAt = atoi(argv[++i]);
            if (i + 1 < argc && argv[i + 1][0] != '-') shotFile = argv[++i];
        }
    }
    startStage = std::max(0, std::min(startStage, STAGE_COUNT - 1));

    SetTraceLogLevel(g_debug ? LOG_WARNING : LOG_ERROR);
    InitWindow(GAME_W * SCALE, GAME_H * SCALE, "March to Ganabhaban");
    // Combat is frame-counted, not delta-timed, so uncapping only makes a test
    // run finish sooner — it does not change how the game plays.
    SetTargetFPS(fast ? 0 : 60);
    InitAudioDevice();
    InitGameAudio();
    SetExitKey(KEY_ESCAPE);

    g_target = LoadRenderTexture(GAME_W, GAME_H);
    SetTextureFilter(g_target.texture, TEXTURE_FILTER_POINT);

    LoadCharacterSheet(CK_REBEL,    "assets/rebel.png");
    LoadCharacterSheet(CK_KADER,    "assets/kader.png");
    LoadCharacterSheet(CK_HASINA,   "assets/hasina.png");
    LoadCharacterSheet(CK_DBPOLICE, "assets/dbpolice.png");
    LoadCharacterSheet(CK_CHHATRA,  "assets/chhatra.png");
    LoadCharacterSheet(CK_POLICE,   "assets/police.png");
    LoadCharacterSheet(CK_JALLAD,   "assets/jallad.png");

    SpawnPlayer(60);
    LoadStageBackground(0);
    // --stage also works without --demo, so you can jump ahead and play it
    // yourself rather than only watching the bot.
    if (demo || startStage > 0 || bossNow) {
        StartRun();
        if (startStage) StartStage(startStage);
        // --boss: skip the waves and walk straight into the boss arena.
        if (bossNow) {
            W.waveIdx = Cur().waveCount;
            W.player.x = Cur().endX;
            W.camX = std::max(0, (int)(Cur().endX - GAME_W * 0.38f));
        }
    }

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(Frame, 0, 1);
#else
    while (!WindowShouldClose() && !g_quit) Frame();
#endif

    UnloadGameAudio();
    UnloadStageBackground();
    UnloadCharacterSheets();
    UnloadRenderTexture(g_target);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

static void Frame() {
    RenderTexture2D& target = g_target;
    const bool demo = g_demo, god = g_god, fast = g_fast;
    long& frameNo = g_frameNo;
    {
        W.stateT++;
        frameNo++;

        bool bL = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
        bool bR = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
        bool bU = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W);
        bool bD = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
        bool bPunch = IsKeyPressed(KEY_J) || IsKeyPressed(KEY_Z);
        bool bJump  = IsKeyPressed(KEY_K) || IsKeyPressed(KEY_X);
        bool bAny   = bPunch || bJump || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
        ReadTouch(bL, bR, bU, bD, bPunch, bJump, bAny);

        if (IsKeyPressed(KEY_R)) ReloadCharacterSheets();
        if (!fast) UpdateMusic();          // raylib Sounds do not loop by themselves

        if (demo) {
            // A bot that actually fights: close on the nearest enemy, line up
            // the depth lane, swing when in reach, otherwise march right.
            // Punching mid-stride locks movement for the length of the attack,
            // so it must not mash while trying to travel.
            bL = bR = bU = bD = false; bPunch = bJump = false;
            const Fighter* tgt = nullptr; float best = 1e9f;
            for (const auto& e : W.enemies) {
                if (!e.alive) continue;
                float d = fabsf(e.x - W.player.x) + fabsf(e.z - W.player.z) * 0.5f;
                if (d < best) { best = d; tgt = &e; }
            }
            if (tgt) {
                float dx = tgt->x - W.player.x, dz = tgt->z - W.player.z;
                if (fabsf(dz) > 5.0f) { bU = dz < 0; bD = dz > 0; }
                if (fabsf(dx) > 22.0f) { bL = dx < 0; bR = dx > 0; }
                else if (fabsf(dz) < 10.0f) bPunch = (frameNo % 9 == 0);
            } else {
                bR = true;
            }
            for (const auto& b : W.shots)              // hop incoming fire
                if (fabsf(b.x - W.player.x) < 34 && fabsf(b.z - W.player.z) < 10)
                    bJump = (W.player.y <= 0.01f);
            bAny = (frameNo % 40 == 0);
        }
        if (god && W.st == GS_PLAY) W.player.hp = W.player.maxHp;

        // ---- feel: hit-stop freezes the world for a few frames on impact ----
        if (W.hitStop > 0) W.hitStop--;
        else {
            switch (W.st) {
            case GS_TITLE:
                if (bAny) StartRun();
                break;
            case GS_INTRO:
                if (W.stateT > 130 || bAny) { W.st = GS_PLAY; W.stateT = 0; }
                break;
            case GS_PLAY:
                UpdatePlay(bL, bR, bU, bD, bPunch, bJump);
                break;
            case GS_CLEAR:
                if (W.stateT > 60 && bAny) {
                    if (W.stage + 1 >= STAGE_COUNT) {
                        W.score += W.lives * SCORE_LIFE_LEFT;
                        if (W.score > W.hiScore) W.hiScore = W.score;
                        W.st = GS_VICTORY; W.stateT = 0;
                    } else StartStage(W.stage + 1);
                }
                break;
            case GS_GAMEOVER:
            case GS_VICTORY:
                if (W.stateT > 90 && bAny) { W.st = GS_TITLE; W.stateT = 0; }
                break;
            }
        }

        if (W.shake > 0.3f) W.shake *= 0.86f; else W.shake = 0;
        if (W.flash > 0)    W.flash -= 0.05f;

        if (g_debug) {   // report flow transitions so a long run is verifiable
            static int pSt = -1, pStage = -1, pWave = -1, pLives = -1; static bool pBoss = false;
            if ((int)W.st != pSt || W.stage != pStage || W.waveIdx != pWave ||
                W.lives != pLives || W.bossOut != pBoss) {
                static const char* NM[] = { "TITLE","INTRO","PLAY","CLEAR","GAMEOVER","VICTORY" };
                printf("f%-6ld %-8s stage%d wave%d/%d boss%d lives%d score%d\n",
                       frameNo, NM[W.st], W.stage + 1, W.waveIdx, Cur().waveCount,
                       (int)W.bossOut, W.lives, W.score);
                fflush(stdout);
                pSt = W.st; pStage = W.stage; pWave = W.waveIdx;
                pLives = W.lives; pBoss = W.bossOut;
            }
            // periodic dump, so a stall shows exactly who is refusing to die
            if (W.st == GS_PLAY && frameNo % 2500 == 0) {
                printf("  .. f%ld px%.0f cam%d lock%d spawned%d/%d enemies:\n",
                       frameNo, W.player.x, W.camX, (int)W.camLock, W.waveSpawned,
                       W.waveIdx < Cur().waveCount ? Cur().waves[W.waveIdx].total : 0);
                for (auto& e : W.enemies)
                    printf("     %-9s x%.0f z%.0f hp%d/%d st%d tok%d arena%d\n",
                           CHAR_NAMES[e.kind], e.x, e.z, e.hp, e.maxHp,
                           (int)e.state, (int)e.hasToken, (int)e.inArena);
                fflush(stdout);
            }
        }

        // ---- draw ----
        BeginTextureMode(target);
            ClearBackground({ 10, 12, 20, 255 });
            float ox = 0, oy = 0;
            if (W.shake > 0.3f) {
                ox = (float)GetRandomValue(-100, 100) / 100.0f * W.shake;
                oy = (float)GetRandomValue(-100, 100) / 100.0f * W.shake * 0.5f;
            }
            BeginMode2D({ { ox, oy }, { 0, 0 }, 0.0f, 1.0f });
                DrawWorld();
            EndMode2D();
            if (W.flash > 0)
                DrawRectangle(0, 0, GAME_W, GAME_H,
                              { 255, 255, 255, (unsigned char)(W.flash * 190) });
            DrawOverlay();
            DrawTouchUI();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(target.texture,
                { 0, 0, (float)GAME_W, -(float)GAME_H },
                { 0, 0, (float)(GAME_W * SCALE), (float)(GAME_H * SCALE) },
                { 0, 0 }, 0.0f, WHITE);
        EndDrawing();

        if (g_shotAt >= 0 && frameNo >= g_shotAt) {
            TakeScreenshot(g_shotFile);
            g_quit = true;
        }
    }
}


