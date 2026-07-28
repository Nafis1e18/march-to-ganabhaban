/* ============================================================
   March to Ganabhaban  —  BOOT / LOOP / INPUT / HUD
   ------------------------------------------------------------
   The world is drawn into a 320x180 buffer and blown up with
   smoothing off, so it stays chunky pixel art. The HUD is then
   drawn on top at the screen's real resolution, so text stays
   sharp instead of turning into blurry 4x mush.
   ============================================================ */

const cv  = document.getElementById('cv');
const ctx = cv.getContext('2d');

const buf = document.createElement('canvas');
buf.width = CFG.W; buf.height = CFG.H;
const bg = buf.getContext('2d');

let SC = 1, VW = CFG.W, VH = CFG.H;

function resize() {
  const dpr = Math.min(window.devicePixelRatio || 1, 2);
  const w = window.innerWidth, h = window.innerHeight;
  SC = Math.min(w / CFG.W, h / CFG.H);
  VW = Math.floor(CFG.W * SC); VH = Math.floor(CFG.H * SC);
  cv.style.width = VW + 'px'; cv.style.height = VH + 'px';
  cv.width = Math.floor(VW * dpr); cv.height = Math.floor(VH * dpr);
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.imageSmoothingEnabled = false;
}
addEventListener('resize', resize);
addEventListener('orientationchange', () => setTimeout(resize, 120));

/* ============================================================
   INPUT
   ============================================================ */
const held = { jump: false, hit: false, back: false };
const prev = { jump: false, hit: false, back: false };
let anyEdge = false;

function firstGesture() { SFX.init(); }

const KEYMAP = {
  Space: 'jump', ArrowUp: 'jump', KeyW: 'jump',
  KeyX:  'hit',  KeyJ:  'hit',   ArrowRight: 'hit',
  ArrowLeft: 'back', KeyA: 'back', KeyS: 'back', ShiftLeft: 'back',
};

addEventListener('keydown', e => {
  if (e.repeat) return;
  firstGesture();
  const k = KEYMAP[e.code];
  if (k) { held[k] = true; e.preventDefault(); }
  anyEdge = true;
  if (e.code === 'KeyM') { SFX.musicOn = !SFX.musicOn; SFX.musicOn ? SFX.startMusic(Game.stageIndex) : SFX.stopMusic(); }
  if (e.code === 'KeyN') { SFX.on = !SFX.on; }
});
addEventListener('keyup', e => { const k = KEYMAP[e.code]; if (k) held[k] = false; });

/* --- touch buttons --- */
function bindBtn(id, key) {
  const el = document.getElementById(id);
  const down = e => { e.preventDefault(); firstGesture(); held[key] = true; anyEdge = true; el.classList.add('on'); };
  const up   = e => { e.preventDefault(); held[key] = false; el.classList.remove('on'); };
  el.addEventListener('pointerdown', down);
  el.addEventListener('pointerup', up);
  el.addEventListener('pointercancel', up);
  el.addEventListener('pointerleave', up);
}
bindBtn('bJump', 'jump');
bindBtn('bHit',  'hit');
bindBtn('bBack', 'back');

/* tapping anywhere advances menus (and unlocks audio) */
cv.addEventListener('pointerdown', e => { e.preventDefault(); firstGesture(); anyEdge = true; });

if (matchMedia('(pointer:coarse)').matches || 'ontouchstart' in window) {
  document.body.classList.add('touch');
}

/* pause the music if the player tabs away */
document.addEventListener('visibilitychange', () => { if (document.hidden) SFX.stopMusic(); });

/* ============================================================
   LOOP — fixed 60Hz physics with an accumulator
   ============================================================ */
let last = performance.now(), acc = 0;
const STEP = 1000 / 60;

function frame(now) {
  requestAnimationFrame(frame);
  acc += Math.min(now - last, 100);        // never simulate more than 100ms of catch-up
  last = now;

  while (acc >= STEP) {
    const inp = {
      jump: held.jump, hit: held.hit, back: held.back,
      jumpPressed: held.jump && !prev.jump,
      hitPressed:  held.hit  && !prev.hit,
      anyPressed:  anyEdge,
    };
    Game.update(inp);
    prev.jump = held.jump; prev.hit = held.hit; prev.back = held.back;
    anyEdge = false;
    acc -= STEP;
  }

  bg.clearRect(0, 0, CFG.W, CFG.H);
  Game.draw(bg);
  ctx.clearRect(0, 0, VW, VH);
  ctx.drawImage(buf, 0, 0, VW, VH);
  drawHUD(ctx);
}

/* ============================================================
   HUD  (drawn at native resolution — crisp text)
   ============================================================ */
const F = (s, w = 700) => `${w} ${Math.round(s * SC)}px "Segoe UI",system-ui,sans-serif`;

function txt(c, s, x, y, size, col, align = 'left', weight = 700, shadow = true) {
  c.font = F(size, weight);
  c.textAlign = align;
  c.textBaseline = 'alphabetic';
  if (shadow) {
    c.fillStyle = 'rgba(0,0,0,.75)';
    c.fillText(s, x * SC + 1.5 * SC, y * SC + 1.5 * SC);
  }
  c.fillStyle = col;
  c.fillText(s, x * SC, y * SC);
}

function bar(c, x, y, w, h, pct, fill, back = 'rgba(0,0,0,.55)') {
  c.fillStyle = back;
  c.fillRect(x * SC, y * SC, w * SC, h * SC);
  c.fillStyle = fill;
  c.fillRect(x * SC, y * SC, w * SC * clamp(pct, 0, 1), h * SC);
  c.strokeStyle = 'rgba(255,255,255,.35)';
  c.lineWidth = Math.max(1, SC * 0.7);
  c.strokeRect(x * SC, y * SC, w * SC, h * SC);
}

function heart(c, x, y, s, filled) {
  c.fillStyle = filled ? '#e0384a' : 'rgba(255,255,255,.18)';
  const u = s * SC / 8;
  const X = x * SC, Y = y * SC;
  // chunky pixel heart
  const rows = ['01100110', '11111111', '11111111', '01111110', '00111100', '00011000'];
  rows.forEach((r, ry) => {
    for (let cx = 0; cx < 8; cx++) if (r[cx] === '1') c.fillRect(X + cx * u, Y + ry * u, u + 0.5, u + 0.5);
  });
}

function dim(c, a) { c.fillStyle = `rgba(4,6,14,${a})`; c.fillRect(0, 0, VW, VH); }

function drawHUD(c) {
  const S = Game.state;

  if (S === 'title')    return hudTitle(c);
  if (S === 'win')      return hudWin(c);

  /* ---- hearts ---- */
  for (let i = 0; i < CFG.maxHearts; i++)
    heart(c, 6 + i * 11, 6, 8, i < Game.hearts);

  /* ---- score ---- */
  txt(c, String(Game.score).padStart(6, '0'), CFG.W - 6, 14, 13, '#ffffff', 'right', 800);
  txt(c, 'HI ' + Game.hi, CFG.W - 6, 24, 7, 'rgba(255,255,255,.55)', 'right', 600);

  /* ---- combo ---- */
  if (Game.combo > 1.01) {
    const pulse = 1 + Math.sin(Game.t * 0.25) * 0.06;
    c.save();
    c.translate((CFG.W - 6) * SC, 36 * SC);
    c.scale(pulse, pulse);
    txt(c, '×' + Game.combo.toFixed(2), 0, 0, 11, '#ffd166', 'right', 800);
    c.restore();
    bar(c, CFG.W - 46, 39, 40, 2, Game.comboTimer / CFG.comboDecay, '#ffd166');
  }

  /* ---- stage banner + progress ---- */
  if (Game.stage) {
    txt(c, `${Game.stageIndex + 1}/${STAGES.length}  ${Game.stage.name}`,
        CFG.W / 2, 12, 8, 'rgba(255,255,255,.80)', 'center', 700);
    if (!Game.bossFight) {
      const pct = clamp(Game.player.x / (Game.stage.len - 240), 0, 1);
      bar(c, CFG.W / 2 - 44, 16, 88, 3, pct, '#7fd1a0');
      // marker showing where you are
      c.fillStyle = '#ffffff';
      c.fillRect((CFG.W / 2 - 44 + 88 * pct - 0.5) * SC, 14.5 * SC, 1.5 * SC, 6 * SC);
    }
  }

  /* ---- boss health ---- */
  const b = Game.boss;
  if (b && Game.bossFight) {
    txt(c, b.def.name, CFG.W / 2, 32, 10, '#ff8a8a', 'center', 800);
    bar(c, CFG.W / 2 - 62, 36, 124, 5, b.hp / b.hpMax,
        b.phase2 ? '#ff3b3b' : '#d64545');
    if (b.stun > 0) txt(c, 'OPENING!', CFG.W / 2, 51, 8, '#ffe066', 'center', 800);
  }

  /* ---- big centred announcements ---- */
  if (Game.msg) {
    const a = clamp(Game.msg.life / 40, 0, 1);
    c.globalAlpha = a;
    txt(c, Game.msg.text, CFG.W / 2, CFG.H / 2 - 4, 22, '#ffffff', 'center', 800);
    txt(c, Game.msg.sub,  CFG.W / 2, CFG.H / 2 + 10, 9, '#ffb3b3', 'center', 600);
    c.globalAlpha = 1;
  }

  if (S === 'intro')    hudIntro(c);
  if (S === 'clear')    hudClear(c);
  if (S === 'gameover') hudOver(c);

  /* ---- first-run control hint ---- */
  if (S === 'play' && Game.stageIndex === 0 && Game.stageT < 260) {
    const a = clamp((260 - Game.stageT) / 60, 0, 1) * clamp(Game.stageT / 30, 0, 1);
    c.globalAlpha = a;
    const touch = document.body.classList.contains('touch');
    txt(c, touch ? 'JUMP over barricades  ·  HIT to break the line'
                 : 'SPACE = jump   X = hit   ← = hold back',
        CFG.W / 2, CFG.H - 12, 8, 'rgba(255,255,255,.85)', 'center', 600);
    c.globalAlpha = 1;
  }
}

function hudIntro(c) {
  const a = clamp(Game.introT / 12, 0, 1) * clamp((100 - Game.introT) / 25, 0, 1);
  dim(c, a * 0.55);
  c.globalAlpha = a;
  txt(c, `STAGE ${Game.stageIndex + 1}`, CFG.W / 2, CFG.H / 2 - 20, 9, '#9fe0b8', 'center', 700);
  txt(c, Game.stage.name, CFG.W / 2, CFG.H / 2 + 2, 26, '#ffffff', 'center', 800);
  txt(c, Game.stage.sub,  CFG.W / 2, CFG.H / 2 + 18, 10, 'rgba(255,255,255,.70)', 'center', 500);
  c.globalAlpha = 1;
}

function hudClear(c) {
  dim(c, 0.72);
  const final = Game.stageIndex + 1 >= STAGES.length;
  txt(c, final ? 'GANABHABAN IS OPEN' : 'LINE BROKEN',
      CFG.W / 2, 46, 20, '#ffffff', 'center', 800);
  txt(c, Game.stage.boss.name + ' has fallen',
      CFG.W / 2, 62, 9, '#ffb3b3', 'center', 600);

  txt(c, 'SCORE',  CFG.W / 2 - 50, 88, 8, 'rgba(255,255,255,.55)', 'center', 600);
  txt(c, String(Game.score), CFG.W / 2 - 50, 102, 15, '#ffffff', 'center', 800);
  txt(c, 'DEFEATED', CFG.W / 2 + 50, 88, 8, 'rgba(255,255,255,.55)', 'center', 600);
  txt(c, String(Game.kills), CFG.W / 2 + 50, 102, 15, '#ffffff', 'center', 800);

  if (Game.clearT > 60 && Game.t % 60 < 38)
    txt(c, document.body.classList.contains('touch') ? 'TAP TO MARCH ON' : 'PRESS ANY KEY',
        CFG.W / 2, CFG.H - 16, 10, '#ffd166', 'center', 700);
}

function hudOver(c) {
  dim(c, clamp(Game.endT / 40, 0, 1) * 0.80);
  txt(c, 'THE LINE HELD', CFG.W / 2, 58, 22, '#ff6b6b', 'center', 800);
  txt(c, `you fell at ${Game.stage.name}`, CFG.W / 2, 74, 9, 'rgba(255,255,255,.65)', 'center', 500);
  txt(c, 'SCORE ' + Game.score, CFG.W / 2, 100, 14, '#ffffff', 'center', 800);
  txt(c, 'BEST ' + Game.hi, CFG.W / 2, 114, 9, 'rgba(255,255,255,.55)', 'center', 600);
  if (Game.endT > 70 && Game.t % 60 < 38)
    txt(c, document.body.classList.contains('touch') ? 'TAP TO TRY AGAIN' : 'PRESS ANY KEY',
        CFG.W / 2, CFG.H - 16, 10, '#ffd166', 'center', 700);
}

function hudTitle(c) {
  txt(c, 'MARCH TO', CFG.W / 2, 42, 15, 'rgba(255,255,255,.85)', 'center', 600);
  txt(c, 'GANABHABAN', CFG.W / 2, 68, 30, '#ffffff', 'center', 800);
  c.fillStyle = '#1f8f4e';
  c.fillRect((CFG.W / 2 - 44) * SC, 74 * SC, 88 * SC, 1.5 * SC);
  txt(c, 'JULY · 2024', CFG.W / 2, 86, 9, '#ffd166', 'center', 600);

  if (Game.t % 60 < 38)
    txt(c, document.body.classList.contains('touch') ? 'TAP TO START' : 'PRESS ANY KEY',
        CFG.W / 2, CFG.H - 30, 11, '#ffffff', 'center', 700);
  txt(c, 'BEST ' + Game.hi, CFG.W / 2, CFG.H - 12, 8, 'rgba(255,255,255,.50)', 'center', 600);
}

function hudWin(c) {
  txt(c, 'SHE IS GONE', CFG.W / 2, 40, 26, '#ffffff', 'center', 800);
  txt(c, '5 August 2024  ·  the gate is open',
      CFG.W / 2, 56, 9, '#9fe0b8', 'center', 600);
  txt(c, 'FINAL SCORE', CFG.W / 2, 78, 8, 'rgba(255,255,255,.55)', 'center', 600);
  txt(c, String(Game.score), CFG.W / 2, 95, 22, '#ffd166', 'center', 800);

  let rank = RANKS[RANKS.length - 1][1];
  for (const [th, nm] of RANKS) if (Game.score >= th) { rank = nm; break; }
  txt(c, rank, CFG.W / 2, 112, 12, '#ffffff', 'center', 800);

  if (Game.endT > 70 && Game.t % 60 < 38)
    txt(c, document.body.classList.contains('touch') ? 'TAP TO PLAY AGAIN' : 'PRESS ANY KEY',
        CFG.W / 2, CFG.H - 14, 10, 'rgba(255,255,255,.85)', 'center', 700);
}

/* ============================================================ */
resize();
Game.init();
requestAnimationFrame(frame);
