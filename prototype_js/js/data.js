/* ============================================================
   March to Ganabhaban  —  DATA / TUNING
   A belt-scroll brawler in the Cadillacs & Dinosaurs mould.
   ------------------------------------------------------------
   Everything here is data. Timings, reach, damage, waves and
   stages live in this file so you never have to edit engine
   code to rebalance the game.
   ============================================================ */

const CFG = {
  W: 384, H: 224,           // CPS-1 arcade resolution, the real thing

  /* ---- the belt ----
     Characters have three coordinates:
       x  = along the street        (world space, scrolls)
       z  = depth up/down the road  (BELT_TOP..BELT_BOT, bigger = nearer camera)
       y  = height off the ground   (jumping only)
     Screen position is  sx = x - camX,  sy = z - y                      */
  BELT_TOP: 150,
  BELT_BOT: 206,

  /* ---- movement ---- */
  walkX: 1.45,              // horizontal walk speed
  walkZ: 0.85,              // up/down the belt is deliberately slower
  gravity: 0.52,
  jumpPower: 7.6,
  runWindow: 14,            // double-tap inside this many frames to dash
  dashX: 3.1,

  /* ---- combat ----
     Every attack is: total length, the frames its hitbox is live,
     reach in x, tolerance in z, and damage.                            */
  atk: {
    punch1:   { len: 13, hit: [3, 6],  reach: 24, zTol: 12, dmg: 4,  push: 1.4 },
    punch2:   { len: 13, hit: [3, 6],  reach: 25, zTol: 12, dmg: 4,  push: 1.6 },
    punch3:   { len: 20, hit: [4, 8],  reach: 28, zTol: 13, dmg: 8,  push: 4.5, knock: true },
    kick:     { len: 18, hit: [5, 9],  reach: 30, zTol: 13, dmg: 6,  push: 3.0 },
    jumpkick: { len: 26, hit: [4, 18], reach: 26, zTol: 13, dmg: 7,  push: 3.4, knock: true },
    throw:    { len: 26, hit: [8, 10], reach: 20, zTol: 14, dmg: 12, push: 0,   knock: true },
  },
  comboWindow: 26,          // frames after a punch to chain into the next
  grabRange: 17,            // walk into a stunned enemy this close to grab
  grabZ: 11,
  grabHold: 150,            // frames before he wriggles free

  hurtStun: 22,             // frames an enemy is staggered after a normal hit
  downTime: 70,             // frames spent on the floor
  getUpInvuln: 40,          // mercy invulnerability while standing back up

  playerHP: 100,
  lives: 3,
  stageTime: 99,            // arcade countdown, in seconds

  /* ---- arcade AI pacing ----
     Only a couple of enemies may commit to an attack at once. Without
     this they all pile in simultaneously and the game is unplayable —
     it is the single most important number in the file.               */
  maxAttackers: 2,
  maxActive: 5,             // enemies awake on screen at once

  /* ---- scoring ---- */
  scoreHit: 10,
  scoreKill: 500,
  scoreBoss: 5000,
  scoreTimeBonus: 50,       // per second left on the clock
};

/* ---- colour sets: [skin, hair, shirt, trousers, accent] ---- */
const PAL = {
  rebel:   ['#c98a5a', '#241a14', '#1f8f4e', '#25324a', '#d62828'],
  punk:    ['#c08050', '#7a3b2a', '#c9553d', '#3a3a44', '#e0e0e0'],
  police:  ['#b8794c', '#2a3550', '#3a4a72', '#242c44', '#8fa3d0'],
  heavy:   ['#a86a3c', '#1e1e28', '#6b6f7a', '#2e2e36', '#c0c4cc'],
  student: ['#c98a5a', '#221a16', '#e8e2d0', '#2c3550', '#d62828'],
};

/* ============================================================
   ENEMY TYPES
   hp / speed / reach are the levers. `aggro` is how likely he is
   to take the attack token when one is free.
   ============================================================ */
const ENEMY = {
  punk: {
    pal: 'punk', hp: 20, h: 30, speed: 0.95, dmg: 5,
    reach: 22, zTol: 12, atkLen: 20, atkHit: [6, 9],
    aggro: 0.8, score: 300, prop: null,
  },
  stick: {
    pal: 'punk', hp: 26, h: 31, speed: 0.85, dmg: 7,
    reach: 30, zTol: 12, atkLen: 24, atkHit: [7, 11],
    aggro: 0.7, score: 400, prop: 'stick',
  },
  police: {
    pal: 'police', hp: 34, h: 31, speed: 0.78, dmg: 8,
    reach: 26, zTol: 12, atkLen: 22, atkHit: [6, 10],
    aggro: 0.6, score: 500, prop: 'shield', guard: 0.4,   // sometimes blocks
  },
  heavy: {
    pal: 'heavy', hp: 55, h: 35, speed: 0.55, dmg: 12,
    reach: 28, zTol: 14, atkLen: 32, atkHit: [12, 17],
    aggro: 0.5, score: 900, prop: null, superArmor: true, // does not flinch
  },
};

/* ---- pickups lying in the street ---- */
const WEAPON = {
  pipe:  { dmg: 11, reach: 34, len: 20, hit: [4, 8],  uses: 14, prop: 'pipe'  },
  knife: { dmg: 9,  reach: 22, len: 13, hit: [3, 6],  uses: 18, prop: 'knife' },
  brick: { dmg: 14, reach: 0,  len: 16, hit: [0, 0],  uses: 1,  prop: 'brick', thrown: true },
};
const FOOD = { biryani: 40, water: 18 };   // heal amounts

/* ============================================================
   STAGES
   Each stage is a list of waves. The camera locks at a wave's
   `x` until every enemy in it is down, then GO -> appears.
   `drop` puts a weapon or food on the ground at that wave.
   ============================================================ */
const STAGES = [
  {
    name: 'SHAHBAGH', sub: 'the first barricade', len: 1900,
    sky: ['#2a1f3d', '#6b3a52', '#c56b4a'], ground: '#3a3340', road: '#2a2530',
    waves: [
      { x: 260, spawn: [['punk', 30], ['punk', 55]] },
      { x: 620, spawn: [['punk', 25], ['stick', 45], ['punk', 60]], drop: 'pipe' },
      { x: 1000, spawn: [['stick', 30], ['punk', 50], ['punk', 40]], drop: 'water' },
      { x: 1400, spawn: [['police', 35], ['punk', 55], ['stick', 25]] },
    ],
    boss: { name: 'THE HOME MINISTER', title: 'keeper of the batons',
            pal: 'police', hp: 130, h: 40, speed: 0.9, dmg: 12,
            reach: 32, zTol: 14, atkLen: 26, atkHit: [8, 13], prop: 'baton' },
  },
  {
    name: 'RAMPURA', sub: 'the bridge holds', len: 2100,
    sky: ['#3d4a6b', '#7a8fb0', '#d4b483'], ground: '#4a4438', road: '#332f28',
    waves: [
      { x: 280, spawn: [['punk', 28], ['stick', 52]] },
      { x: 660, spawn: [['police', 30], ['punk', 48], ['punk', 58]], drop: 'brick' },
      { x: 1080, spawn: [['heavy', 40], ['punk', 25]], drop: 'biryani' },
      { x: 1520, spawn: [['stick', 30], ['police', 50], ['punk', 42], ['punk', 20]] },
    ],
    boss: { name: 'THE LAW MINISTER', title: 'author of the curfew',
            pal: 'police', hp: 170, h: 41, speed: 1.0, dmg: 13,
            reach: 33, zTol: 14, atkLen: 24, atkHit: [7, 12], prop: 'gavel' },
  },
  {
    name: 'UTTARA', sub: 'they cut the internet', len: 2200,
    sky: ['#1a1a2e', '#2d3561', '#4a5a8a'], ground: '#2a2a38', road: '#1e1e28',
    waves: [
      { x: 300, spawn: [['police', 30], ['police', 55]] },
      { x: 700, spawn: [['heavy', 42], ['stick', 26], ['punk', 58]], drop: 'pipe' },
      { x: 1140, spawn: [['punk', 24], ['punk', 38], ['punk', 52], ['stick', 60]], drop: 'water' },
      { x: 1600, spawn: [['police', 32], ['heavy', 48], ['stick', 22]] },
    ],
    boss: { name: 'THE INFORMATION MINISTER', title: 'silencer of signals',
            pal: 'heavy', hp: 200, h: 41, speed: 1.05, dmg: 14,
            reach: 31, zTol: 14, atkLen: 22, atkHit: [6, 11], prop: 'antenna' },
  },
  {
    name: 'JATRABARI', sub: 'nothing left to lose', len: 2300,
    sky: ['#4a1520', '#8a2f2a', '#c4562e'], ground: '#3d2a25', road: '#2a1d1a',
    waves: [
      { x: 300, spawn: [['stick', 28], ['stick', 54]] },
      { x: 720, spawn: [['heavy', 36], ['police', 52], ['punk', 22]], drop: 'biryani' },
      { x: 1180, spawn: [['police', 26], ['police', 44], ['police', 58]], drop: 'pipe' },
      { x: 1660, spawn: [['heavy', 30], ['heavy', 52], ['stick', 42]] },
    ],
    boss: { name: 'THE CHIEF OF POLICE', title: 'the last cordon',
            pal: 'police', hp: 240, h: 42, speed: 1.1, dmg: 15,
            reach: 34, zTol: 15, atkLen: 24, atkHit: [7, 12], prop: 'shield' },
  },
  {
    name: 'GANABHABAN', sub: '5 August — the gate is open', len: 1700,
    sky: ['#0d1b2a', '#1b4332', '#40916c'], ground: '#3a4a3a', road: '#2a3428',
    waves: [
      { x: 300, spawn: [['police', 26], ['police', 42], ['police', 58]] },
      { x: 760, spawn: [['heavy', 34], ['heavy', 50], ['police', 24]], drop: 'biryani' },
      { x: 1180, spawn: [['heavy', 30], ['police', 46], ['police', 56], ['stick', 20]] },
    ],
    boss: { name: 'SHEIKH HASINA', title: 'the final chair',
            pal: 'police', hp: 320, h: 46, speed: 1.15, dmg: 16,
            reach: 34, zTol: 15, atkLen: 22, atkHit: [6, 11], prop: 'crown',
            phases: 2, final: true },
  },
];

const RANKS = [
  [120000, 'JULY LEGEND'],
  [ 80000, 'GANABHABAN'],
  [ 50000, 'FRONT LINE'],
  [ 25000, 'MARCHER'],
  [     0, 'BYSTANDER'],
];
