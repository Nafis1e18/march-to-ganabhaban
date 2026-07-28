// ============================================================
//  Stages, waves, enemy stats.
//
//  All of it is data. To rebalance the game, change numbers in
//  this file â€” never the engine. To add a stage, append to
//  STAGES and bump STAGE_COUNT.
// ============================================================
#pragma once
#include "game.h"

// ---------- per-enemy stats ----------
// reach/zTol/atk timings are the frame data for that enemy's swing.
struct EnemyStats {
    int   hp;
    int   height;
    float speed;
    int   damage;
    float reach;
    float zTol;
    int   atkTotal;      // whole attack, in frames
    int   atkFrom, atkTo;// hitbox live window
    int   score;
    bool  ranged;        // shoots instead of closing in
    bool  superArmor;    // does not flinch when hit
    float guardChance;   // 0..1, odds of blocking instead of walking in
};

// Indexed by CharKind. The player's row is unused.
constexpr EnemyStats ENEMY_STATS[CK_COUNT] = {
    /* REBEL    */ {  0, 48, 0.00f,  0,  0.f,  0.f,  0, 0, 0,    0, false, false, 0.00f },
    /* KADER    */ { 170, 52, 1.00f, 13, 30.f, 13.f, 26, 7, 12, 4000, false, false,  0.10f },
    /* HASINA   */ { 300, 52, 0.95f, 15, 28.f, 13.f, 24, 6, 11, 8000, true,  false,  0.00f },
    /* DBPOLICE */ {  42, 46, 0.80f,  9, 24.f, 12.f, 22, 6, 10,  600, true,  false,  0.00f },
    /* CHHATRA  */ {  24, 44, 1.00f,  6, 26.f, 12.f, 20, 6,  9,  200, false, false,  0.00f },
    /* POLICE   */ {  38, 46, 0.74f,  8, 25.f, 12.f, 24, 7, 11,  400, false, false,  0.35f },
    /* JALLAD   */ {  72, 54, 0.52f, 16, 32.f, 14.f, 34, 13, 19, 1200, false, true,  0.00f },
};

// ---------- waves ----------
// The camera locks at `atX` and will not scroll on until every enemy
// spawned here is down. That lock is what turns a scrolling background
// into an arcade brawler.
struct WaveDef {
    float atX;
    int   total;          // how many spawn in all
    int   onceMax;        // how many may be alive at the same time
    int   kinds[4];       // pool drawn from, in order, repeating
    int   kindCount;
};

struct Stage {
    const char* name;
    const char* sub;
    int   bg;                 // assets/bg<N>_far.png / _near.png
    WaveDef  waves[5];
    int   waveCount;
    float endX;               // boss arena

    int   bossKind;
    const char* bossName;
    const char* bossTitle;
    float bossHpMul;
    bool  bossElite;          // a scaled-up regular enemy rather than a named boss

    // difficulty, applied on top of ENEMY_STATS
    float hpMul, dmgMul, spdMul;
    int   maxAttackers;       // how many may swing at once â€” the key dial
};

constexpr int STAGE_COUNT = 5;

constexpr Stage STAGES[STAGE_COUNT] = {
    // ---------------------------------------------------------
    {   "SHAHBAGH", "the first barricade", 0,
        {
            { 340,  3, 2, { CK_CHHATRA }, 1 },
            { 700,  4, 3, { CK_CHHATRA }, 1 },
            { 1060, 5, 3, { CK_CHHATRA, CK_POLICE }, 2 },
        }, 3, 1420,
        CK_CHHATRA, "THE RINGLEADER", "he brought the sticks", 4.5f, true,
        1.00f, 1.00f, 1.00f, 2
    },
    // ---------------------------------------------------------
    {   "RAMPURA", "the bridge holds", 1,
        {
            { 340,  4, 3, { CK_CHHATRA, CK_POLICE }, 2 },
            { 720,  5, 3, { CK_POLICE, CK_CHHATRA }, 2 },
            { 1120, 5, 3, { CK_POLICE, CK_DBPOLICE }, 2 },
        }, 3, 1500,
        CK_JALLAD, "JALLAD", "the executioner", 1.0f, false,
        1.15f, 1.10f, 1.05f, 2
    },
    // ---------------------------------------------------------
    {   "UTTARA", "they cut the internet", 2,
        {
            { 340,  5, 3, { CK_DBPOLICE, CK_CHHATRA }, 2 },
            { 760,  6, 4, { CK_POLICE, CK_DBPOLICE }, 2 },
            { 1180, 6, 4, { CK_CHHATRA, CK_POLICE, CK_DBPOLICE }, 3 },
        }, 3, 1560,
        CK_DBPOLICE, "DB HARUN", "the one who fires first", 5.5f, true,
        1.30f, 1.20f, 1.10f, 3
    },
    // ---------------------------------------------------------
    {   "JATRABARI", "nothing left to lose", 3,
        {
            { 340,  6, 4, { CK_POLICE, CK_CHHATRA }, 2 },
            { 780,  6, 4, { CK_JALLAD, CK_POLICE }, 2 },
            { 1200, 7, 4, { CK_DBPOLICE, CK_POLICE, CK_JALLAD }, 3 },
        }, 3, 1620,
        CK_KADER, "KAWWA KADER", "he called you a crow", 1.0f, false,
        1.50f, 1.30f, 1.15f, 3
    },
    // ---------------------------------------------------------
    {   "GANABHABAN", "5 August - the gate is open", 4,
        {
            { 340,  6, 4, { CK_DBPOLICE, CK_POLICE }, 2 },
            { 800,  7, 4, { CK_JALLAD, CK_DBPOLICE }, 2 },
            { 1240, 8, 5, { CK_JALLAD, CK_POLICE, CK_DBPOLICE, CK_CHHATRA }, 4 },
        }, 3, 1700,
        CK_HASINA, "SHEIKH HASINA", "the final chair", 1.0f, false,
        1.75f, 1.45f, 1.20f, 3
    },
};

// ---------- scoring ----------
constexpr int SCORE_HIT       = 10;
constexpr int SCORE_STAGE     = 5000;
constexpr int SCORE_LIFE_LEFT = 10000;

constexpr int START_LIVES = 3;

// End-of-run ranks.
struct Rank { int at; const char* name; };
constexpr Rank RANKS[] = {
    { 120000, "JULY LEGEND" },
    {  80000, "GANABHABAN"  },
    {  50000, "FRONT LINE"  },
    {  25000, "MARCHER"     },
    {      0, "BYSTANDER"   },
};
constexpr int RANK_COUNT = 5;

