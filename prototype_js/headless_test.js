/* Headless smoke test.  node tools/headless_test.js
   Stubs just enough DOM/Canvas/Audio to run the real game files, then plays
   the game with a bot so runtime errors surface without opening a browser. */
const fs = require('fs'), path = require('path'), vm = require('vm');

const ROOT = path.join(__dirname, '..');
let drawCalls = 0;

function ctx2d() {
  const noop = () => {};
  return {
    fillStyle: '', strokeStyle: '', lineWidth: 1, font: '', textAlign: '',
    textBaseline: '', globalAlpha: 1, imageSmoothingEnabled: false,
    fillRect: () => { drawCalls++; }, strokeRect: noop, clearRect: noop,
    save: noop, restore: noop, translate: noop, scale: noop, rotate: noop,
    setTransform: noop, drawImage: noop, beginPath: noop, closePath: noop,
    moveTo: noop, lineTo: noop, arc: noop, fill: noop, stroke: noop,
    fillText: () => { drawCalls++; }, strokeText: noop,
    measureText: () => ({ width: 10 }),
    createLinearGradient: () => ({ addColorStop: noop }),
    createRadialGradient: () => ({ addColorStop: noop }),
  };
}

function el() {
  const e = {
    style: {}, width: 320, height: 180,
    classList: { add: noop2, remove: noop2, contains: () => false },
    addEventListener: noop2, removeEventListener: noop2,
    getContext: () => ctx2d(), appendChild: noop2,
  };
  function noop2() {}
  return e;
}

const sandbox = {
  console,
  performance: { now: () => Date.now() },
  requestAnimationFrame: () => 0,
  setTimeout: () => 0, clearTimeout: () => {}, setInterval: () => 0, clearInterval: () => {},
  localStorage: { _d: {}, getItem(k) { return this._d[k] ?? null; }, setItem(k, v) { this._d[k] = String(v); } },
  matchMedia: () => ({ matches: false }),
  addEventListener: () => {},
  Image: function () { return { set src(v) {}, onload: null }; },
  document: {
    body: el(),
    getElementById: () => el(),
    createElement: () => el(),
    addEventListener: () => {},
    hidden: false,
  },
};
sandbox.window = sandbox;
sandbox.globalThis = sandbox;
vm.createContext(sandbox);

for (const f of ['data', 'art', 'audio', 'game', 'main']) {
  const src = fs.readFileSync(path.join(ROOT, 'js', f + '.js'), 'utf8');
  try { vm.runInContext(src, sandbox, { filename: 'js/' + f + '.js' }); }
  catch (e) { console.error(`LOAD FAIL js/${f}.js:`, e.message); process.exit(1); }
}

/* top-level `const` lives in the context's lexical scope, not on globalThis,
   so pull the bindings out with one more script in that same scope */
vm.runInContext(
  'globalThis.__x = { Game, CFG, STAGES, PAL, ENEMY, OBST, drawHUD };',
  sandbox, { filename: 'export.js' });
const { Game, CFG, STAGES } = sandbox.__x;
sandbox.drawHUD = sandbox.__x.drawHUD;
const fail = [];
const check = (cond, label) => {
  console.log((cond ? '  PASS  ' : '  FAIL  ') + label);
  if (!cond) fail.push(label);
};

/* One simulated frame. Rendering every frame of a 120k-frame playthrough takes
   minutes, so the long run draws 1 frame in DRAW_EVERY; the short scripted
   sections set it to 1 and render everything. */
let DRAW_EVERY = 1, frameNo = 0;
const scratchCtx = ctx2d();

function step(inp) {
  Game.update(inp);
  if (frameNo++ % DRAW_EVERY === 0) {
    Game.draw(scratchCtx);
    sandbox.drawHUD(scratchCtx);
  }
}

/* a bot that jumps obstacles and swings at whatever is in front of it */
function botInput(prevHeld) {
  const p = Game.player;
  if (!p || Game.state !== 'play') return { jump: false, hit: false, back: false, jumpPressed: false, hitPressed: false, anyPressed: true };
  let wantJump = false, wantHit = false, wantBack = false;

  for (const o of Game.obstacles) {
    const d = o.x - p.x;
    if (d > 10 && d < 44) wantJump = true;
  }
  // the player now turns to face whatever is nearest, so swing on |distance|
  for (const e of Game.enemies) {
    if (e.down) continue;
    const d = e.x - p.x;
    if (d > -6 && d < 30) wantHit = true;
  }
  const b = Game.boss;
  if (b && b.dying === 0) {
    const d = b.x - p.x;
    if (Math.abs(d) < 34) wantHit = true;
    if (b.act === 'charge' && b.actT > 22 && Math.abs(d) < 80) wantJump = true;
    // He charged past us — hold BACK to chase him down. Keep holding until we
    // are inside swinging range, otherwise auto-run shoves us straight back
    // out and the bot oscillates in the gap instead of ever landing a hit.
    if (d < -20) wantBack = true;
  }
  for (const s of Game.shots) {
    const d = s.x - p.x;
    if (d > 4 && d < 30) wantJump = true;
  }
  return {
    jump: wantJump, hit: wantHit, back: wantBack,
    jumpPressed: wantJump && !prevHeld.jump,
    hitPressed: wantHit && !prevHeld.hit,
    anyPressed: false,
  };
}

console.log('\n--- load ---');
check(typeof Game === 'object', 'game files loaded into one context');
check(Game.state === 'title', 'boots to the title screen');
check(STAGES.length === 5, `${STAGES.length} stages defined`);

console.log('\n--- title renders ---');
try { for (let i = 0; i < 60; i++) step({ jump: 0, hit: 0, back: 0, jumpPressed: 0, hitPressed: 0, anyPressed: false }); check(true, 'title screen draws 60 frames'); }
catch (e) { check(false, 'title screen draws: ' + e.message); }

console.log('\n--- full playthrough (damage disabled, to reach every stage) ---');
const realDamage = Game.damage.bind(Game);
Game.damage = () => {};                       // invincible bot, so we exercise all 5 stages
step({ anyPressed: true });                   // leave title

let prev = { jump: false, hit: false };
let frames = 0;
const seen = new Set();
const MAXF = 120000;
DRAW_EVERY = 15;                              // still ~8k rendered frames
try {
  while (Game.state !== 'win' && frames < MAXF) {
    let inp;
    if (Game.state === 'intro' || Game.state === 'clear' || Game.state === 'gameover') {
      inp = { jump: 0, hit: 0, back: 0, jumpPressed: 0, hitPressed: 0, anyPressed: true };
    } else {
      inp = botInput(prev);
    }
    step(inp);
    prev = { jump: inp.jump, hit: inp.hit };
    if (Game.stage) seen.add(Game.stage.name);
    frames++;
  }
} catch (e) {
  check(false, `crashed after ${frames} frames: ${e.stack.split('\n').slice(0, 3).join(' | ')}`);
}

DRAW_EVERY = 1;
check(seen.size === STAGES.length, `bot visited all stages (${seen.size}/${STAGES.length}: ${[...seen].join(', ')})`);
check(Game.state === 'win', `reached the win screen in ${frames} frames (~${(frames / 3600).toFixed(1)} min of play)`);
check(Game.score > 0, `score accumulated: ${Game.score}`);
check(Game.kills > 0, `enemies defeated: ${Game.kills}`);
check(drawCalls > 100000, `renderer produced ${drawCalls} draw ops`);

console.log('\n--- win screen renders ---');
try { for (let i = 0; i < 90; i++) step({ jump: 0, hit: 0, back: 0, jumpPressed: 0, hitPressed: 0, anyPressed: false }); check(true, 'win screen draws 90 frames'); }
catch (e) { check(false, 'win screen draws: ' + e.message); }

console.log('\n--- death path ---');
Game.damage = realDamage;
try {
  Game.init(); step({ anyPressed: true });
  for (let i = 0; i < 200 && Game.state !== 'play'; i++) step({ anyPressed: true });
  let guard = 0;
  while (Game.state !== 'gameover' && guard++ < 6000) {
    Game.hearts = 1;
    Game.player.invuln = 0;
    step({ jump: 0, hit: 0, back: 0, jumpPressed: 0, hitPressed: 0, anyPressed: false });
  }
  check(Game.state === 'gameover', `game over reached after ${guard} frames`);
  for (let i = 0; i < 90; i++) step({ jump: 0, hit: 0, back: 0, jumpPressed: 0, hitPressed: 0, anyPressed: false });
  check(true, 'game over screen draws');
  check(Game.hi > 0, `high score persisted: ${Game.hi}`);
} catch (e) { check(false, 'death path: ' + e.message); }

console.log('\n--- boss behaviour ---');
/* Watch the pattern rotation with a passive player. An attacking bot kills the
   boss long before he has cycled every move, which tells us nothing. */
try {
  Game.init(); Game.startStage(4); Game.state = 'play';
  Game.player.x = STAGES[4].len - 239;
  Game.damage = () => {};
  const idle = { jump: 0, hit: 0, back: 0, jumpPressed: 0, hitPressed: 0, anyPressed: false };
  const acts = new Set();
  for (let i = 0; i < 2500; i++) {
    step(idle);
    if (Game.boss) acts.add(Game.boss.act);
  }
  check(acts.has('charge') && acts.has('throw') && acts.has('summon'),
        `final boss cycled all patterns: ${[...acts].join(', ')}`);
  check(Game.shots.length >= 0 && Game.enemies.length > 0,
        `summon actually spawned reinforcements (${Game.enemies.length} on screen)`);

  // second phase should trip once he is down to half health
  Game.boss.hp = Math.floor(Game.boss.hpMax * 0.5);
  for (let i = 0; i < 5; i++) step(idle);
  check(Game.boss.phase2, 'second phase triggers at half health');

  // and he should actually die and hand off to the clear screen
  Game.boss.hp = 1;
  let g2 = 0;
  while (Game.state === 'play' && g2++ < 900) {
    step({ jump: 0, hit: 1, back: 0, jumpPressed: 0, hitPressed: 1, anyPressed: false });
  }
  check(Game.state === 'clear' || Game.state === 'win',
        `boss death handed off to '${Game.state}' after ${g2} frames`);
} catch (e) { check(false, 'boss behaviour: ' + e.stack.split('\n')[0]); }

console.log('\n' + (fail.length ? `${fail.length} FAILURE(S)\n - ` + fail.join('\n - ') : 'ALL CHECKS PASSED'));
process.exit(fail.length ? 1 : 0);
