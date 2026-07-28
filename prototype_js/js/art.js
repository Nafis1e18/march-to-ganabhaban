/* ============================================================
   March to Ganabhaban  —  ART
   ------------------------------------------------------------
   Characters are a small skeleton (hip / shoulder / knee /
   elbow) posed per frame, so one system animates the player,
   every thug and every boss. Nothing loads from disk.

   >> TO USE YOUR OWN DRAWINGS: see the SPRITES block at the
      bottom of this file, and read SPRITES.md.
   ============================================================ */

function px(g, x, y, w, h, c) {
  g.fillStyle = c;
  g.fillRect(x | 0, y | 0, Math.max(1, w | 0), Math.max(1, h | 0));
}

function pixLine(g, x1, y1, x2, y2, w, c) {
  const dx = x2 - x1, dy = y2 - y1;
  const steps = Math.max(1, Math.ceil(Math.hypot(dx, dy)));
  const o = (w - 1) / 2;
  g.fillStyle = c;
  for (let i = 0; i <= steps; i++) {
    const t = i / steps;
    g.fillRect(Math.round(x1 + dx * t - o), Math.round(y1 + dy * t - o), w, w);
  }
}

function circle(g, cx, cy, r, c) {
  g.fillStyle = c;
  for (let y = -r; y <= r; y++) {
    const span = Math.floor(Math.sqrt(Math.max(0, r * r - y * y)));
    g.fillRect(Math.round(cx - span), Math.round(cy + y), span * 2 + 1, 1);
  }
}

function ellipse(g, cx, cy, rx, ry, c) {
  g.fillStyle = c;
  for (let y = -ry; y <= ry; y++) {
    const span = rx * Math.sqrt(Math.max(0, 1 - (y * y) / (ry * ry)));
    g.fillRect(Math.round(cx - span), Math.round(cy + y), Math.round(span * 2) + 1, 1);
  }
}

/* angle 0 points straight DOWN, positive swings forward */
function joint(x, y, ang, len) {
  return [x + Math.sin(ang) * len, y + Math.cos(ang) * len];
}

const shade = (hex, amt) => {
  const n = parseInt(hex.slice(1), 16);
  const cl = v => Math.max(0, Math.min(255, v));
  return '#' + [cl((n >> 16) + amt), cl(((n >> 8) & 255) + amt), cl((n & 255) + amt)]
    .map(v => v.toString(16).padStart(2, '0')).join('');
};

/* ============================================================
   POSES — one bag of joint angles per animation frame.
   `t` is 0..1 progress through an attack; `phase` is 0..1
   through a looping cycle.
   ============================================================ */
function pose(state, phase, t) {
  const P = { lean: 0, bob: 0, headTilt: 0, crouch: 0,
              thighF: 0, shinF: 0, thighB: 0, shinB: 0,
              armF: 0, foreF: 0, armB: 0, foreB: 0 };
  const TAU = Math.PI * 2;
  const s = Math.sin(phase * TAU), c = Math.cos(phase * TAU);

  switch (state) {
    case 'walk':
      P.lean = 0.07;
      P.bob = Math.abs(Math.sin(phase * TAU * 2)) * 1.1;
      P.thighF = s * 0.62; P.thighB = -s * 0.62;
      P.shinF = Math.max(0, -c) * 0.85; P.shinB = Math.max(0, c) * 0.85;
      P.armF = -s * 0.5; P.foreF = 0.45;
      P.armB = s * 0.5;  P.foreB = 0.45;
      break;

    case 'dash':
      P.lean = 0.34;
      P.bob = Math.abs(Math.sin(phase * TAU * 2)) * 1.7;
      P.thighF = s * 1.0; P.thighB = -s * 1.0;
      P.shinF = Math.max(0, -c) * 1.3; P.shinB = Math.max(0, c) * 1.3;
      P.armF = -s * 0.9; P.foreF = 0.8;
      P.armB = s * 0.9;  P.foreB = 0.8;
      break;

    /* --- three-hit punch chain: jab, cross, hook --- */
    case 'punch1': {
      const k = t < 0.3 ? -(t / 0.3) * 0.8 : t < 0.5 ? ((t - 0.3) / 0.2) * 2.4 - 0.8 : 1.6 - ((t - 0.5) / 0.5) * 1.6;
      P.lean = 0.12 + k * 0.06;
      P.armF = -1.45 - k * 0.25; P.foreF = Math.max(0, 0.95 - k * 0.95);
      P.armB = 0.75; P.foreB = 0.7;
      P.thighF = 0.42; P.shinF = 0.10; P.thighB = -0.42; P.shinB = 0.42;
      break;
    }
    case 'punch2': {
      const k = t < 0.3 ? -(t / 0.3) * 0.8 : t < 0.5 ? ((t - 0.3) / 0.2) * 2.4 - 0.8 : 1.6 - ((t - 0.5) / 0.5) * 1.6;
      P.lean = 0.18 + k * 0.10;
      P.armB = -1.45 - k * 0.30; P.foreB = Math.max(0, 0.95 - k * 0.95);
      P.armF = 0.85; P.foreF = 0.75;
      P.thighF = 0.50; P.shinF = 0.10; P.thighB = -0.50; P.shinB = 0.48;
      break;
    }
    case 'punch3': {
      const k = t < 0.35 ? -(t / 0.35) * 1.2 : t < 0.55 ? ((t - 0.35) / 0.2) * 3.4 - 1.2 : 2.2 - ((t - 0.55) / 0.45) * 2.2;
      P.lean = 0.24 + k * 0.20;
      P.bob = -k * 1.2;
      P.armF = -1.30 - k * 0.55; P.foreF = Math.max(-0.3, 1.15 - k * 1.35);
      P.armB = 1.15; P.foreB = 0.85;
      P.thighF = 0.62 + k * 0.15; P.shinF = 0.12; P.thighB = -0.58; P.shinB = 0.55;
      break;
    }

    case 'kick': {
      const k = t < 0.3 ? (t / 0.3) * 0.5 : t < 0.55 ? 0.5 + ((t - 0.3) / 0.25) * 1.1 : 1.6 - ((t - 0.55) / 0.45) * 1.6;
      P.lean = -0.16 - k * 0.10;
      P.thighF = 0.30 + k * 1.15; P.shinF = Math.max(0, 0.75 - k * 0.85);
      P.thighB = -0.20; P.shinB = 0.22;
      P.armF = -0.75 - k * 0.30; P.foreF = 0.85;
      P.armB = 0.95 + k * 0.35;  P.foreB = 0.70;
      break;
    }

    case 'jump':
      P.lean = 0.10;
      P.thighF = 1.05; P.shinF = 0.80;
      P.thighB = -0.35; P.shinB = 1.30;
      P.armF = -1.45; P.foreF = 0.55; P.armB = -0.85; P.foreB = 0.85;
      break;

    case 'jumpkick':
      P.lean = -0.10;
      P.thighF = 1.45; P.shinF = 0.05;       // lead leg locked out
      P.thighB = -0.25; P.shinB = 1.45;      // trailing leg tucked
      P.armF = -1.9; P.foreF = 0.5; P.armB = 1.5; P.foreB = 0.6;
      break;

    case 'grabbing':                          // holding an enemy by the collar
      P.lean = 0.16;
      P.armF = -1.55; P.foreF = 0.10;
      P.armB = -1.40; P.foreB = 0.15;
      P.thighF = 0.35; P.shinF = 0.12; P.thighB = -0.35; P.shinB = 0.38;
      break;

    case 'grabbed':                           // being held
      P.lean = -0.30; P.headTilt = -0.2;
      P.armF = -2.35; P.foreF = 0.45;
      P.armB = -2.15; P.foreB = 0.45;
      P.thighF = 0.30; P.shinF = 0.25; P.thighB = -0.28; P.shinB = 0.30;
      break;

    case 'throw': {                           // swing the held body overhead
      const k = t < 0.4 ? -(t / 0.4) : ((t - 0.4) / 0.6) * 2.6 - 1;
      P.lean = 0.20 + k * 0.30;
      P.armF = -1.6 - k * 1.1; P.foreF = 0.15;
      P.armB = -1.5 - k * 1.1; P.foreB = 0.15;
      P.thighF = 0.45; P.shinF = 0.12; P.thighB = -0.45; P.shinB = 0.45;
      break;
    }

    case 'block':
      P.lean = -0.10; P.crouch = 1.5;
      P.armF = -1.75; P.foreF = 1.45;         // forearms crossed up high
      P.armB = -1.55; P.foreB = 1.55;
      P.thighF = 0.32; P.shinF = 0.20; P.thighB = -0.32; P.shinB = 0.34;
      break;

    case 'hurt':
      P.lean = -0.45; P.headTilt = -0.35;
      P.armF = -2.3; P.foreF = 0.55; P.armB = -2.0; P.foreB = 0.55;
      P.thighF = 0.55; P.shinF = 0.40; P.thighB = -0.30; P.shinB = 0.32;
      break;

    case 'down':                              // flat out on the tarmac
      P.lean = 1.48; P.headTilt = 0.25;
      P.armF = 1.85; P.armB = 2.15; P.foreF = 0.25; P.foreB = 0.25;
      P.thighF = 1.65; P.thighB = 1.40; P.shinF = 0.45; P.shinB = 0.35;
      break;

    case 'getup':                             // pushing back up off the floor
      P.lean = 0.75 * (1 - t); P.crouch = 6 * (1 - t);
      P.armF = -0.55; P.foreF = 0.85; P.armB = 0.5; P.foreB = 0.6;
      P.thighF = 0.85; P.shinF = 0.75; P.thighB = -0.55; P.shinB = 0.85;
      break;

    default:                                  // idle — small breathing bob
      P.lean = 0.03; P.bob = s * 0.55;
      P.armF = 0.14 + s * 0.05; P.foreF = 0.22;
      P.armB = -0.14 - s * 0.05; P.foreB = 0.22;
      P.thighF = 0.10; P.thighB = -0.10; P.shinF = 0.05; P.shinB = 0.05;
      break;
  }
  return P;
}

/* ---------- ground shadow: sells the belt and the jump height ---------- */
function drawShadow(g, sx, beltY, h, alpha = 0.34) {
  ellipse(g, sx, beltY, Math.max(4, h * 0.30), Math.max(2, h * 0.10),
          `rgba(0,0,0,${alpha})`);
}

/* ============================================================
   drawHuman(g, sx, sy, o)
   sx, sy = screen position of the point between the feet.
   o = { pal, h, facing, state, phase, t, prop, flag, tint, alpha }
   ============================================================ */
function drawHuman(g, sx, sy, o) {
  if (SPRITES.draw(g, sx, sy, o)) return;      // your art wins if it exists

  const P = pose(o.state, o.phase || 0, o.t || 0);
  const h = o.h || 30;
  const f = o.facing || 1;
  const col = PAL[o.pal] || PAL.rebel;
  const [skin, hair, shirt, pant, accent] = o.tint ? col.map(() => o.tint) : col;
  const shirtD = shade(shirt, -34), pantD = shade(pant, -30);

  const y = sy + (P.crouch || 0);
  const bob = P.bob;
  const hipY = y - h * 0.44 + bob;
  const shldY = y - h * 0.74 + bob;
  const lean = P.lean * f;
  const hipX = sx;
  const shldX = sx + Math.sin(lean) * (h * 0.30);
  const headX = shldX + Math.sin(lean + P.headTilt * f) * (h * 0.12);
  const headY = shldY - h * 0.12;
  const headR = Math.max(2, Math.round(h * 0.125));

  const thigh = h * 0.235, shin = h * 0.235;
  const upper = h * 0.195, fore = h * 0.195;
  const limbW = Math.max(2, Math.round(h * 0.085));

  /* --- far-side limbs, drawn darker so depth reads --- */
  const kB = joint(hipX, hipY, P.thighB * f + lean, thigh);
  const fB = joint(kB[0], kB[1], (P.thighB + P.shinB) * f + lean, shin);
  pixLine(g, hipX, hipY, kB[0], kB[1], limbW, pantD);
  pixLine(g, kB[0], kB[1], fB[0], fB[1], limbW, pantD);
  px(g, fB[0] - limbW * f, fB[1] - 1, limbW + 2, 2, shade(pantD, -20));

  const eB = joint(shldX, shldY, P.armB * f + lean, upper);
  const hB = joint(eB[0], eB[1], (P.armB + P.foreB) * f + lean, fore);
  pixLine(g, shldX, shldY, eB[0], eB[1], limbW - 1, shade(shirtD, -18));
  pixLine(g, eB[0], eB[1], hB[0], hB[1], limbW - 1, shade(skin, -35));

  if (o.flag) drawFlag(g, shldX - 3 * f, shldY - 2, f, o.phase || 0);

  /* --- torso --- */
  const tw = Math.max(4, Math.round(h * 0.30));
  for (let i = 0; i <= 8; i++) {
    const q = i / 8;
    const cx = hipX + (shldX - hipX) * q;
    const cy = hipY + (shldY - hipY) * q;
    const w = tw * (0.84 + 0.16 * q);
    px(g, cx - w / 2, cy - 1.5, w, 3, i < 2 ? pant : shirt);
  }
  px(g, shldX - tw / 2, shldY - 2, tw, 3, shade(shirt, 18));

  /* --- head --- */
  circle(g, headX, headY, headR, skin);
  px(g, headX + (headR - 1) * f, headY - 1, 1, 1, shade(skin, -50));
  circle(g, headX, headY - headR * 0.45, headR * 0.92, hair);
  px(g, headX - headR, headY - headR * 0.2, headR * 2 + 1, 1, hair);

  if (o.pal === 'police') {                       // riot helmet + visor
    circle(g, headX, headY - headR * 0.35, headR * 1.15, shade(col[1], 22));
    px(g, headX - headR - 1, headY - headR * 0.1, headR * 2 + 3, 2, shade(col[1], -20));
    px(g, headX - headR, headY - headR * 0.1, headR * 2 + 1, 3, 'rgba(160,200,255,.30)');
  }
  if (o.pal === 'heavy') {                        // shaved head + shades
    px(g, headX - headR, headY - 1, headR * 2 + 1, 2, '#14141c');
  }
  if (o.pal === 'rebel') {                        // red headband, tails flying
    px(g, headX - headR - 1, headY - headR * 0.15, headR * 2 + 3, 2, '#d62828');
    pixLine(g, headX - (headR + 1) * f, headY - headR * 0.1,
               headX - (headR + 4) * f, headY + 1 + Math.sin((o.phase || 0) * 8) * 1.5,
               1, '#d62828');
  }

  /* --- near-side limbs --- */
  const kF = joint(hipX, hipY, P.thighF * f + lean, thigh);
  const fF = joint(kF[0], kF[1], (P.thighF + P.shinF) * f + lean, shin);
  pixLine(g, hipX, hipY, kF[0], kF[1], limbW, pant);
  pixLine(g, kF[0], kF[1], fF[0], fF[1], limbW, pant);
  px(g, fF[0] - limbW * f, fF[1] - 1, limbW + 2, 2, shade(pant, -40));

  const eF = joint(shldX, shldY, P.armF * f + lean, upper);
  const hF = joint(eF[0], eF[1], (P.armF + P.foreF) * f + lean, fore);
  pixLine(g, shldX, shldY, eF[0], eF[1], limbW - 1, shirt);
  pixLine(g, eF[0], eF[1], hF[0], hF[1], limbW - 1, skin);

  if (o.prop) drawProp(g, o.prop, hF[0], hF[1], f, (P.armF + P.foreF) * f + lean, h, col);
}

/* ---------- held props ---------- */
function drawProp(g, kind, hx, hy, f, ang, h, col) {
  switch (kind) {
    case 'stick': case 'baton': case 'pipe': {
      const L = h * (kind === 'pipe' ? 0.62 : kind === 'baton' ? 0.52 : 0.44);
      const e = joint(hx, hy, ang, L);
      pixLine(g, hx, hy, e[0], e[1], 2,
              kind === 'pipe' ? '#9aa4b8' : kind === 'baton' ? '#1e1e28' : '#6b4a2a');
      break;
    }
    case 'knife': {
      const e = joint(hx, hy, ang, h * 0.24);
      pixLine(g, hx, hy, e[0], e[1], 2, '#cfd6e4');
      break;
    }
    case 'brick': px(g, hx - 2, hy - 2, 5, 4, '#8a5a44'); break;
    case 'shield': {
      const w = Math.round(h * 0.28), ht = Math.round(h * 0.58);
      px(g, hx - w / 2 + 2 * f, hy - ht * 0.55, w, ht, '#2f3a5c');
      px(g, hx - w / 2 + 2 * f, hy - ht * 0.55, w, 2, '#7f93c4');
      px(g, hx - w / 2 + 3 * f, hy - ht * 0.42, w - 2, ht * 0.4, 'rgba(170,205,255,.22)');
      break;
    }
    case 'gavel': {
      const e = joint(hx, hy, ang, h * 0.34);
      pixLine(g, hx, hy, e[0], e[1], 2, '#6b4a2a');
      px(g, e[0] - 4, e[1] - 3, 8, 5, '#8a6a3a');
      break;
    }
    case 'antenna': {
      const e = joint(hx, hy, ang - 0.4 * f, h * 0.70);
      pixLine(g, hx, hy, e[0], e[1], 1, '#9aa4b8');
      circle(g, e[0], e[1], 2, '#d62828');
      break;
    }
  }
}

function drawFlag(g, x, y, f, phase) {
  const top = y - 22;
  pixLine(g, x, y + 4, x, top, 1, '#6b4a2a');
  for (let i = 0; i < 12; i++) {
    const w = Math.sin(phase * 6 + i * 0.55) * 1.8;
    px(g, x - (i + 1) * f, top + 1 + i * 0.9 + w, 1, 8, '#0f7a3d');
  }
  circle(g, x - 6 * f, top + 6, 2, '#d62828');
}

/* ---------- bosses get a signature on top of the base body ---------- */
function drawBoss(g, sx, sy, def, o) {
  drawHuman(g, sx, sy, o);
  if (SPRITES.has(o.pal, o.state)) return;      // your art already includes it
  const h = o.h;
  const P = pose(o.state, o.phase, o.t || 0);
  const headY = sy + (P.crouch || 0) - h * 0.86 + P.bob;
  const headX = sx + Math.sin(P.lean * o.facing) * (h * 0.42);
  if (def.prop === 'crown') {
    const r = Math.round(h * 0.14);
    px(g, headX - r - 1, headY - r * 1.9, r * 2 + 3, 2, '#e8c547');
    for (let i = 0; i < 3; i++) px(g, headX - r + i * r, headY - r * 2.6, 2, 2, '#e8c547');
  }
  pixLine(g, sx - h * 0.14, sy - h * 0.72, sx + h * 0.14, sy - h * 0.42, 3, 'rgba(200,40,40,.75)');
}

/* ============================================================
   BACKGROUND — sky and skyline above the belt, road below.
   ============================================================ */
function drawBackground(g, st, camX, t) {
  const W = CFG.W, H = CFG.H, TOP = CFG.BELT_TOP, BOT = CFG.BELT_BOT;

  const grd = g.createLinearGradient(0, 0, 0, TOP);
  grd.addColorStop(0, st.sky[0]); grd.addColorStop(0.6, st.sky[1]); grd.addColorStop(1, st.sky[2]);
  g.fillStyle = grd; g.fillRect(0, 0, W, TOP);

  circle(g, W * 0.76, TOP * 0.28, 12, 'rgba(255,240,210,.16)');
  circle(g, W * 0.76, TOP * 0.28, 8, 'rgba(255,246,225,.55)');

  buildings(g, camX * 0.10, TOP, 70, 38, 'rgba(0,0,0,.30)', 0, false);
  buildings(g, camX * 0.30, TOP, 52, 28, 'rgba(0,0,0,.50)', 7, true);
  crowd(g, camX * 0.55, TOP, t);

  // kerb line at the top of the belt, then the road itself
  px(g, 0, TOP - 5, W, 5, shade(st.ground, 18));
  px(g, 0, TOP - 1, W, 2, shade(st.ground, 40));
  px(g, 0, TOP + 1, W, BOT - TOP + 18, st.road);

  // perspective lane dashes: wider apart as they come toward the camera
  for (let row = 0; row < 4; row++) {
    const z = TOP + 8 + row * 14;
    const spacing = 26 + row * 8;
    const off = -((camX * (0.75 + row * 0.08)) % spacing);
    for (let i = -1; i < W / spacing + 2; i++)
      px(g, off + i * spacing, z, 9 + row * 2, 1, 'rgba(255,255,255,.07)');
  }
  // gutter at the very front
  px(g, 0, BOT + 6, W, 3, shade(st.road, -18));
}

function buildings(g, ox, GY, maxH, minH, col, winDensity, lit) {
  const W = CFG.W, base = Math.floor(ox / 28);
  for (let i = -1; i < W / 28 + 2; i++) {
    const idx = base + i;
    const r = Math.abs(Math.sin(idx * 12.9898) * 43758.5453) % 1;
    const bh = minH + r * (maxH - minH), bw = 20 + (r * 14 | 0);
    const bx = idx * 28 - ox;
    px(g, bx, GY - bh, bw, bh, col);
    if (lit) {
      for (let wy = 4; wy < bh - 4; wy += 6)
        for (let wx = 3; wx < bw - 3; wx += 5) {
          const q = Math.abs(Math.sin(idx * 31 + wx * 7 + wy * 3) * 1000) % 10;
          if (q < winDensity)
            px(g, bx + wx, GY - bh + wy, 2, 3,
               q < 2 ? 'rgba(255,214,120,.55)' : 'rgba(255,200,90,.22)');
        }
    }
  }
}

function crowd(g, ox, GY, t) {
  const W = CFG.W, base = Math.floor(ox / 15);
  for (let i = -1; i < W / 15 + 2; i++) {
    const idx = base + i;
    const r = Math.abs(Math.sin(idx * 7.13) * 9371.7) % 1;
    const x = idx * 15 - ox + (r * 6 | 0);
    const bob = Math.sin(t * 0.06 + idx) * 1.1;
    const hh = 16 + (r * 5 | 0);
    px(g, x, GY - hh + bob - 5, 5, hh, 'rgba(0,0,0,.42)');
    circle(g, x + 2, GY - hh + bob - 7, 3, 'rgba(0,0,0,.42)');
    if (r > 0.74) {
      pixLine(g, x + 5, GY - hh + bob - 5, x + 5, GY - hh - 14 + bob, 1, 'rgba(0,0,0,.42)');
      px(g, x + 6, GY - hh - 14 + bob, 5, 4, 'rgba(15,122,61,.55)');
    }
  }
}

/* ---------- street furniture, weapons and pickups ---------- */
function drawItem(g, kind, sx, sy, t) {
  drawShadow(g, sx, sy, 14, 0.25);
  switch (kind) {
    case 'pipe':  px(g, sx - 8, sy - 3, 16, 2, '#9aa4b8'); break;
    case 'knife': px(g, sx - 5, sy - 3, 10, 2, '#cfd6e4'); break;
    case 'brick': px(g, sx - 4, sy - 4, 8, 5, '#8a5a44');
                  px(g, sx - 4, sy - 4, 8, 1, '#a87a5c'); break;
    case 'biryani':
      ellipse(g, sx, sy - 3, 6, 3, '#d9b06a');
      px(g, sx - 5, sy - 6, 10, 3, '#e8c98a');
      circle(g, sx + 1, sy - 6, 1, '#b03a2a');
      break;
    case 'water':
      px(g, sx - 2, sy - 9, 5, 9, 'rgba(150,210,255,.75)');
      px(g, sx - 1, sy - 11, 3, 2, '#3a7ab0');
      break;
  }
  // gentle glint so pickups read against the road
  if ((t | 0) % 40 < 20) px(g, sx - 1, sy - 13, 2, 2, 'rgba(255,255,255,.5)');
}

function drawHitSpark(g, x, y, life) {
  const r = (1 - life) * 10 + 2;
  for (let i = 0; i < 7; i++) {
    const a = i * 0.897 + life * 3;
    px(g, x + Math.cos(a) * r, y + Math.sin(a) * r, 2, 2,
       life > 0.5 ? '#fff3c4' : '#f4a23a');
  }
}
function drawDust(g, x, y, life) {
  circle(g, x, y, (1 - life) * 4 + 1, `rgba(200,190,170,${life * 0.4})`);
}

/* ============================================================
   SPRITES — drop your own drawings in here
   ------------------------------------------------------------
   Per-ANIMATION override. If you have painted `walk` but not
   `kick`, walk uses your art and kick falls back to the drawn
   skeleton. So you can ship at any stage of finishing the art.

   Full instructions and a drawing template: see SPRITES.md
   ============================================================ */
const SPRITES = {
  chars: {},          // pal name -> { img, fw, fh, anchorY, anims, ready }

  /* Register a character sheet.
       name    : must match a PAL key ('rebel', 'punk', 'police', 'heavy')
       url     : path to your PNG strip
       fw, fh  : size of ONE frame in that PNG
       anims   : { animName: [frame indexes...] }  frames count left-to-right
       anchorY : how far above the frame's bottom edge the FEET sit (px)
       fps     : animation speed for looping anims                        */
  load(name, url, { fw, fh, anims, anchorY = 0, fps = 10 }) {
    const img = new Image();
    const rec = { img, fw, fh, anims, anchorY, fps, ready: false };
    img.onload = () => { rec.ready = true; };
    img.onerror = () => { console.warn('sprite sheet missing:', url); };
    img.src = url;
    this.chars[name] = rec;
  },

  has(pal, state) {
    const c = this.chars[pal];
    return !!(c && c.ready && c.anims[state] && c.anims[state].length);
  },

  draw(g, sx, sy, o) {
    const c = this.chars[o.pal];
    if (!c || !c.ready) return false;
    const list = c.anims[o.state];
    if (!list || !list.length) return false;    // not painted yet -> skeleton

    // attacks play once across their duration; loops cycle on phase
    const isAtk = o.t !== undefined && o.t > 0 && o.t <= 1 &&
                  /punch|kick|throw|getup/.test(o.state);
    const idx = isAtk
      ? Math.min(list.length - 1, Math.floor(o.t * list.length))
      : Math.floor((o.phase || 0) * list.length) % list.length;

    const scale = (o.h || 30) / (c.fh - c.anchorY);
    const w = c.fw * scale, hh = c.fh * scale;
    g.save();
    g.translate(Math.round(sx), Math.round(sy + c.anchorY * scale));
    if ((o.facing || 1) < 0) g.scale(-1, 1);
    if (o.tint) g.globalAlpha = 0.85;
    g.drawImage(c.img, list[idx] * c.fw, 0, c.fw, c.fh, -w / 2, -hh, w, hh);
    g.restore();
    return true;
  },
};

/* ------------------------------------------------------------
   EXAMPLE — uncomment and edit once your PNG exists.
   The order of numbers is the order frames appear in your strip.

SPRITES.load('rebel', 'assets/rebel.png', {
  fw: 48, fh: 64, anchorY: 0, fps: 10,
  anims: {
    idle:   [0, 1, 2, 1],
    walk:   [3, 4, 5, 6, 7, 8],
    punch1: [9, 10],
    punch2: [11, 12],
    punch3: [13, 14, 15],
    kick:   [16, 17, 18],
    jump:   [19],
    jumpkick:[20],
    hurt:   [21],
    down:   [22, 23],
    getup:  [24, 25],
  },
});
------------------------------------------------------------ */
