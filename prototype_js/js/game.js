/* ============================================================
   March to Ganabhaban  —  GAME LOGIC
   ============================================================ */

const rnd  = (a, b) => a + Math.random() * (b - a);
const irnd = (a, b) => Math.floor(rnd(a, b + 1));
const clamp = (v, a, b) => v < a ? a : v > b ? b : v;
const pick = arr => arr[Math.floor(Math.random() * arr.length)];

/* axis-aligned box overlap; boxes are {x,y,w,h} with y = TOP */
function hits(a, b) {
  return a.x < b.x + b.w && a.x + a.w > b.x &&
         a.y < b.y + b.h && a.y + a.h > b.y;
}

const Game = {
  /* ---------------- lifecycle ---------------- */
  init() {
    this.hi = +(localStorage.getItem('mtg_hi') || 0);
    this.state = 'title';
    this.t = 0;
    this.shake = 0;
    this.flash = 0;
    this.fade = 0;
    this.msg = null;
    this.stageIndex = 0;
    this.resetRun();
  },

  resetRun() {
    this.score = 0;
    this.hearts = CFG.maxHearts;
    this.combo = 1;
    this.comboTimer = 0;
    this.kills = 0;
    this.stageIndex = 0;
    this.distTotal = 0;
  },

  startStage(i) {
    this.stageIndex = i;
    this.stage = STAGES[i];
    this.camX = 0;
    this.stageT = 0;
    this.bossFight = false;
    this.boss = null;
    this.enemies = [];
    this.obstacles = [];
    this.parts = [];
    this.shots = [];
    this.speed = CFG.runSpeed + i * 0.10;

    this.player = {
      x: 90, y: CFG.GROUND, vy: 0, onGround: true,
      state: 'run', phase: 0, facing: 1,
      atk: -1, atkCool: 0, invuln: 0, hitStop: 0,
      coyote: 0, buffer: 0, holdT: 0,
      w: 11, h: 27,
    };

    this.buildLayout();
    this.state = 'intro';
    this.introT = 0;
    SFX.startMusic(i);
  },

  /* lay out the whole stage up front so pacing is readable */
  buildLayout() {
    const st = this.stage;
    let x = 320;
    let lastWasObstacle = false;
    while (x < st.len - 260) {
      const gap = rnd(105, 210) * (1.15 - st.density * 0.35);
      x += gap;
      if (Math.random() < st.density && !lastWasObstacle) {
        const k = pick(['barricade', 'fire', 'crate']);
        const o = OBST[k];
        this.obstacles.push({ kind: k, x, y: CFG.GROUND, w: o.w, h: o.h, dmg: o.dmg });
        lastWasObstacle = true;
      } else {
        this.spawnEnemy(pick(st.enemies), x);
        lastWasObstacle = false;
        // packs get denser as the stage goes on
        if (Math.random() < st.density * 0.45 && x > st.len * 0.4) {
          this.spawnEnemy(pick(st.enemies), x + rnd(26, 48));
        }
      }
    }
  },

  spawnEnemy(type, x) {
    const e = ENEMY[type];
    this.enemies.push({
      type, x, y: CFG.GROUND, vy: 0, onGround: true,
      hp: e.hp, w: e.w, h: e.h, dmg: e.dmg, speed: e.speed,
      pal: e.pal, prop: e.prop,
      state: 'idle', phase: Math.random(), facing: -1,
      awake: false, down: 0, flashT: 0,
    });
  },

  /* ---------------- main update ---------------- */
  update(inp) {
    this.t++;
    if (this.shake > 0) this.shake *= 0.86;
    if (this.flash > 0) this.flash -= 0.06;

    switch (this.state) {
      case 'title':     this.upTitle(inp); break;
      case 'intro':     this.upIntro(inp); break;
      case 'play':      this.upPlay(inp);  break;
      case 'clear':     this.upClear(inp); break;
      case 'gameover':  this.upEnd(inp);   break;
      case 'win':       this.upEnd(inp);   break;
    }
  },

  upTitle(inp) {
    if (inp.anyPressed) {
      SFX.select();
      this.resetRun();
      this.startStage(0);
    }
  },

  upIntro(inp) {
    this.introT++;
    if (this.introT > 100 || inp.anyPressed) this.state = 'play';
  },

  upClear(inp) {
    this.clearT++;
    if (this.clearT > 60 && inp.anyPressed) {
      SFX.select();
      if (this.stageIndex + 1 >= STAGES.length) {
        this.state = 'win';
        this.endT = 0;
        this.saveHi();
      } else {
        this.startStage(this.stageIndex + 1);
      }
    }
  },

  upEnd(inp) {
    this.endT++;
    if (this.endT > 70 && inp.anyPressed) { SFX.select(); this.state = 'title'; }
  },

  /* ---------------- the run ---------------- */
  upPlay(inp) {
    const p = this.player;
    this.stageT++;

    if (p.hitStop > 0) { p.hitStop--; return; }   // brief freeze sells the impact

    /* --- combo decay --- */
    if (this.comboTimer > 0) { this.comboTimer--; if (this.comboTimer === 0) this.combo = 1; }

    /* --- horizontal motion: the march never stops unless you brake --- */
    const arenaL = this.bossFight ? this.arenaX - 40 : -1e9;
    const arenaR = this.bossFight ? this.arenaX + 268 : 1e9;

    let vx = inp.back ? -CFG.backSpeed : this.speed;
    // In the arena the boss charges straight past you, so always turn to face
    // him — otherwise backing away would leave you swinging at empty air.
    p.facing = this.bossFight && this.boss ? (this.boss.x >= p.x ? 1 : -1) : 1;

    // in the arena you may push forward but not walk out the sides
    p.x = clamp(p.x + vx, this.bossFight ? arenaL + 14 : p.x - 999, arenaR - 14);
    if (!this.bossFight) this.distTotal += Math.max(0, vx);

    /* --- score from distance travelled --- */
    if (!this.bossFight && this.t % 6 === 0) {
      this.score += Math.round(CFG.scorePerStep * this.combo);
    }

    /* --- jump: coyote time + input buffering, variable height --- */
    if (p.onGround) p.coyote = CFG.coyote; else if (p.coyote > 0) p.coyote--;
    if (inp.jumpPressed) p.buffer = CFG.jumpBuffer; else if (p.buffer > 0) p.buffer--;

    if (p.buffer > 0 && p.coyote > 0) {
      p.vy = -CFG.jumpPower;
      p.onGround = false; p.coyote = 0; p.buffer = 0; p.holdT = CFG.jumpHoldMax;
      SFX.jump();
      this.puff(p.x, CFG.GROUND, 4);
    }
    if (!p.onGround && inp.jump && p.holdT > 0 && p.vy < 0) {
      p.vy -= CFG.jumpHold; p.holdT--;
    } else p.holdT = 0;

    p.vy += CFG.gravity;
    p.y += p.vy;
    if (p.y >= CFG.GROUND) {
      if (!p.onGround) { SFX.land(); this.puff(p.x, CFG.GROUND, 3); }
      p.y = CFG.GROUND; p.vy = 0; p.onGround = true;
    }

    /* --- attack --- */
    if (p.atkCool > 0) p.atkCool--;
    // Held, not press-edge: on a touch screen holding HIT has to keep swinging.
    // attackCool already paces it, so this reads as a steady combo, not a blur.
    if (inp.hit && p.atk < 0 && p.atkCool === 0) {
      p.atk = 0; SFX.swing();
    }
    if (p.atk >= 0) {
      p.atk++;
      if (p.atk >= CFG.attackFrames) { p.atk = -1; p.atkCool = CFG.attackCool; }
    }

    if (p.invuln > 0) p.invuln--;

    /* --- animation state --- */
    p.phase = (p.phase + (p.onGround ? 0.055 * (vx > 0 ? 1 : 0.7) : 0.02)) % 1;
    p.state = p.atk >= 0 ? 'attack'
            : !p.onGround ? (p.vy < 0 ? 'jump' : 'fall')
            : 'run';
    p.atkT = p.atk >= 0 ? p.atk / CFG.attackFrames : 0;

    /* --- camera --- */
    this.camX = this.bossFight ? this.arenaX - 40 : p.x - 90;

    /* --- entity updates --- */
    this.updateEnemies();
    this.updateObstacles();
    this.updateShots();
    this.updateParts();

    /* --- reach the arena --- */
    if (!this.bossFight && p.x >= this.stage.len - 240) this.enterBoss();
    if (this.bossFight) this.updateBoss();
  },

  playerBox() {
    const p = this.player;
    return { x: p.x - p.w / 2, y: p.y - p.h, w: p.w, h: p.h };
  },

  /* the swing hitbox, live only during the damage frames */
  attackBox() {
    const p = this.player;
    if (p.atk < CFG.attackHit[0] || p.atk > CFG.attackHit[1]) return null;
    return { x: p.facing > 0 ? p.x : p.x - CFG.attackReach,
             y: p.y - p.h + 2, w: CFG.attackReach, h: p.h - 2 };
  },

  /* ---------------- enemies ---------------- */
  updateEnemies() {
    const p = this.player, pb = this.playerBox(), ab = this.attackBox();

    for (let i = this.enemies.length - 1; i >= 0; i--) {
      const e = this.enemies[i];

      if (e.x < this.camX - 90 || e.x > this.camX + 640) {
        if (e.down) this.enemies.splice(i, 1);
        continue;
      }
      if (e.flashT > 0) e.flashT--;

      /* --- knocked down: slide, fade, remove --- */
      if (e.down > 0) {
        e.down--;
        e.x += e.kbv || 0;
        e.kbv = (e.kbv || 0) * 0.90;
        e.state = 'down';
        if (e.down <= 0) this.enemies.splice(i, 1);
        continue;
      }

      /* --- wake when you are close, then walk at you --- */
      if (!e.awake && Math.abs(e.x - p.x) < 150) e.awake = true;
      if (e.awake) {
        e.facing = p.x < e.x ? -1 : 1;
        e.x += e.speed * e.facing * 0.55;
        e.state = 'run';
        e.phase = (e.phase + 0.045) % 1;
      } else {
        e.state = 'idle';
        e.phase = (e.phase + 0.01) % 1;
      }

      const eb = { x: e.x - e.w / 2, y: e.y - e.h, w: e.w, h: e.h };

      /* --- you hit them --- */
      if (ab && hits(ab, eb)) {
        e.hp--;
        e.flashT = 5;
        this.shake = 4;
        this.spark(e.x, e.y - e.h * 0.6);
        p.hitStop = 3;
        if (e.hp <= 0) {
          e.down = 90;
          e.kbv = 2.6 * p.facing;
          this.kills++;
          this.combo = Math.min(CFG.comboMax, this.combo + CFG.comboStep);
          this.comboTimer = CFG.comboDecay;
          const gain = Math.round(CFG.scoreEnemy * this.combo);
          this.score += gain;
          this.float(e.x, e.y - e.h - 4, '+' + gain);
          SFX.kill();
          if (this.combo > 1.2) SFX.combo(Math.round((this.combo - 1) / CFG.comboStep));
        } else {
          e.x += 3 * p.facing;
          SFX.hit();
        }
        continue;
      }

      /* --- they hit you --- */
      if (hits(pb, eb)) this.damage(e.dmg, e.x);
    }
  },

  updateObstacles() {
    const pb = this.playerBox(), ab = this.attackBox();
    for (let i = this.obstacles.length - 1; i >= 0; i--) {
      const o = this.obstacles[i];
      if (o.x < this.camX - 60) { this.obstacles.splice(i, 1); continue; }
      const ob = { x: o.x, y: o.y - o.h, w: o.w, h: o.h };

      // crates and barricades can be smashed through
      if (ab && o.kind !== 'fire' && hits(ab, ob)) {
        this.obstacles.splice(i, 1);
        this.spark(o.x + o.w / 2, o.y - o.h / 2);
        this.puff(o.x + o.w / 2, o.y, 6);
        this.score += Math.round(30 * this.combo);
        this.shake = 3; this.player.hitStop = 2;
        SFX.hit();
        continue;
      }
      if (hits(pb, ob)) this.damage(o.dmg, o.x);
    }
  },

  updateShots() {
    const pb = this.playerBox();
    for (let i = this.shots.length - 1; i >= 0; i--) {
      const s = this.shots[i];
      s.x += s.vx; s.vy += 0.16; s.y += s.vy; s.rot += 0.25;
      if (s.y >= CFG.GROUND) { this.puff(s.x, CFG.GROUND, 5); this.shots.splice(i, 1); continue; }
      if (s.x < this.camX - 40 || s.x > this.camX + 400) { this.shots.splice(i, 1); continue; }
      if (hits(pb, { x: s.x - 3, y: s.y - 3, w: 6, h: 6 })) {
        this.shots.splice(i, 1);
        this.damage(1, s.x);
      }
    }
  },

  updateParts() {
    for (let i = this.parts.length - 1; i >= 0; i--) {
      const q = this.parts[i];
      q.life -= q.decay;
      q.x += q.vx || 0; q.y += q.vy || 0;
      if (q.vy !== undefined && q.grav) q.vy += 0.08;
      if (q.life <= 0) this.parts.splice(i, 1);
    }
  },

  /* ---------------- damage & feedback ---------------- */
  damage(n, fromX) {
    const p = this.player;
    if (p.invuln > 0) return;
    this.hearts -= n;
    p.invuln = CFG.hurtInvuln;
    p.hitStop = 6;
    p.x -= CFG.knockback * (fromX > p.x ? 1 : -1);
    p.vy = -3.2; p.onGround = false;
    this.combo = 1; this.comboTimer = 0;
    this.shake = 9;
    this.flash = 0.5;
    SFX.hurt();

    if (this.hearts <= 0) {
      this.hearts = 0;
      this.state = 'gameover';
      this.endT = 0;
      SFX.stopMusic();
      SFX.defeat();
      this.saveHi();
    }
  },

  spark(x, y) { this.parts.push({ kind: 'spark', x, y, life: 1, decay: 0.12 }); },
  puff(x, y, n) {
    for (let i = 0; i < n; i++)
      this.parts.push({ kind: 'dust', x: x + rnd(-4, 4), y: y + rnd(-2, 1),
                        vx: rnd(-0.6, 0.6), vy: rnd(-0.5, -0.1), grav: 1,
                        life: 1, decay: rnd(0.05, 0.09) });
  },
  float(x, y, text) {
    this.parts.push({ kind: 'text', x, y, text, vy: -0.55, life: 1, decay: 0.018 });
  },

  saveHi() {
    if (this.score > this.hi) { this.hi = this.score; localStorage.setItem('mtg_hi', this.hi); }
  },

  /* ============================================================
     BOSS
     ============================================================ */
  enterBoss() {
    this.bossFight = true;
    this.arenaX = this.player.x;
    const b = this.stage.boss;
    this.boss = {
      def: b, x: this.arenaX + 230, y: CFG.GROUND, vy: 0, onGround: true,
      hp: b.hp, hpMax: b.hp,
      w: Math.round(14 * b.scale), h: Math.round(30 * b.scale),
      state: 'idle', phase: 0, facing: -1,
      timer: 90, act: 'idle', actT: 0, stun: 0, flashT: 0,
      phase2: false, dying: 0, entered: 0,
    };
    this.enemies = this.enemies.filter(e => e.x > this.arenaX - 60 && !e.down);
    this.obstacles = [];
    SFX.bossRoar();
    SFX.setBpm(150);
    this.shake = 10;
    this.msg = { text: b.name, sub: b.title, life: 150 };
  },

  updateBoss() {
    const b = this.boss, p = this.player;
    if (!b) return;

    if (this.msg) { this.msg.life--; if (this.msg.life <= 0) this.msg = null; }

    /* --- death throes --- */
    if (b.dying > 0) {
      b.dying--;
      b.state = 'down';
      if (b.dying % 12 === 0) { this.spark(b.x + rnd(-10, 10), b.y - rnd(5, b.h)); this.shake = 6; }
      if (b.dying === 0) this.finishStage();
      return;
    }

    b.flashT = Math.max(0, b.flashT - 1);
    b.facing = p.x < b.x ? -1 : 1;

    /* --- second phase for the final boss: faster, angrier --- */
    if (b.def.phases === 2 && !b.phase2 && b.hp <= b.hpMax * 0.5) {
      b.phase2 = true; this.shake = 12; this.flash = 0.6;
      SFX.bossRoar(); SFX.setBpm(168);
      this.msg = { text: 'NO WAY OUT', sub: 'the gate is already open', life: 110 };
    }
    const rate = b.phase2 ? 1.45 : 1.0;

    /* --- pattern picker --- */
    b.timer--;
    if (b.stun > 0) {
      b.stun--; b.state = 'hurt';
      if (b.stun === 0) { b.act = 'idle'; b.timer = 40; }
    } else if (b.timer <= 0) {
      const roll = Math.random();
      if (roll < 0.42)      { b.act = 'charge'; b.actT = 0; b.timer = 130; }
      else if (roll < 0.75) { b.act = 'throw';  b.actT = 0; b.timer = 80;  }
      else                  { b.act = 'summon'; b.actT = 0; b.timer = 110; }
    }

    /* --- act out the pattern --- */
    if (b.stun === 0) {
      b.actT++;
      switch (b.act) {
        case 'charge': {
          if (b.actT < 26) {                       // telegraph: lean back
            b.state = 'idle'; b.phase = (b.phase + 0.05) % 1;
          } else if (b.actT < 90) {
            b.x += b.def.speed * 2.35 * rate * b.facing;
            b.state = 'run'; b.phase = (b.phase + 0.11) % 1;
            if (b.actT % 7 === 0) this.puff(b.x, CFG.GROUND, 2);
          } else {                                 // overshoots and is wide open
            b.state = 'hurt'; b.stun = 46; b.act = 'idle';
          }
          break;
        }
        case 'throw': {
          b.state = 'attack'; b.atkT = clamp(b.actT / 40, 0, 1);
          b.phase = 0;
          if (b.actT === 18) {
            this.shots.push({ x: b.x + 8 * b.facing, y: b.y - b.h * 0.8,
                              vx: -3.1 * rate, vy: -2.5, rot: 0 });
            SFX.swing();
          }
          if (b.actT > 46) { b.act = 'idle'; b.state = 'idle'; }
          break;
        }
        case 'summon': {
          b.state = 'idle'; b.phase = (b.phase + 0.03) % 1;
          if (b.actT === 24) {
            for (let i = 0; i < (b.phase2 ? 3 : 2); i++)
              this.spawnEnemy(pick(this.stage.enemies), b.x - 20 - i * 24);
            SFX.bossRoar();
          }
          break;
        }
        default:
          b.state = 'idle'; b.phase = (b.phase + 0.03) % 1;
          // Walk back toward the player between attacks. After a charge he ends
          // up across the arena, and without this you would have to hold BACK
          // for several seconds every time to re-engage him. Stop short rather
          // than grinding into the player at point-blank range.
          if (Math.abs(b.x - p.x) > 22) b.x += b.def.speed * 0.6 * rate * b.facing;
      }
    }

    b.x = clamp(b.x, this.arenaX - 20, this.arenaX + 268);

    /* --- collisions --- */
    const bb = { x: b.x - b.w / 2, y: b.y - b.h, w: b.w, h: b.h };
    const ab = this.attackBox();

    if (ab && hits(ab, bb) && b.flashT === 0) {
      const big = b.stun > 0;                       // punish the opening
      b.hp -= big ? 2 : 1;
      b.flashT = 9;
      this.shake = big ? 9 : 5;
      this.spark(b.x, b.y - b.h * 0.6);
      p.hitStop = big ? 6 : 3;
      SFX.bossHit();
      if (big) this.float(b.x, b.y - b.h - 6, 'CRITICAL');

      if (b.hp <= 0) {
        b.hp = 0; b.dying = 90;
        SFX.stopMusic();
        this.flash = 0.8; this.shake = 14;
        const gain = Math.round(CFG.scoreBoss * (this.stageIndex + 1) * this.combo);
        this.score += gain;
        this.float(b.x, b.y - b.h - 10, '+' + gain);
        SFX.victory();
      }
    }

    // contact damage — hardest while he is charging
    if (b.stun === 0 && b.dying === 0 && hits(this.playerBox(), bb)) {
      this.damage(b.act === 'charge' && b.actT >= 26 ? 2 : 1, b.x);
    }
  },

  finishStage() {
    this.state = 'clear';
    this.clearT = 0;
    this.bossFight = false;
    SFX.stopMusic();
    SFX.setBpm(132);
    if (this.hearts < CFG.maxHearts) this.hearts++;   // one heart back per stage
    this.saveHi();
  },

  /* ============================================================
     DRAW — pixel buffer (320x180)
     ============================================================ */
  draw(g) {
    g.imageSmoothingEnabled = false;

    if (this.state === 'title') { this.drawTitleScene(g); return; }
    if (this.state === 'win')   { this.drawWinScene(g);  return; }

    const st = this.stage || STAGES[0];
    const sx = this.shake > 0.4 ? rnd(-this.shake, this.shake) * 0.4 : 0;
    const sy = this.shake > 0.4 ? rnd(-this.shake, this.shake) * 0.25 : 0;

    g.save();
    g.translate(Math.round(sx), Math.round(sy));

    drawBackground(g, st, this.camX, this.t);

    const ox = -this.camX;

    // obstacles
    for (const o of this.obstacles) {
      if (o.x + ox < -30 || o.x + ox > CFG.W + 30) continue;
      drawObstacle(g, o.kind, o.x + ox, o.y, this.t);
    }

    // enemies
    for (const e of this.enemies) {
      const x = e.x + ox;
      if (x < -30 || x > CFG.W + 30) continue;
      if (e.down > 0) g.globalAlpha = clamp(e.down / 45, 0, 1);
      drawHuman(g, x, e.y, {
        pal: e.pal, h: e.h, facing: e.facing, state: e.state,
        phase: e.phase, prop: e.prop,
        tint: e.flashT > 0 ? '#ffffff' : null,
      });
      g.globalAlpha = 1;
    }

    // boss
    const b = this.boss;
    if (b) {
      // telegraph flare before a charge
      if (b.act === 'charge' && b.actT < 26 && b.stun === 0 && b.dying === 0) {
        px(g, b.x + ox - 16, CFG.GROUND - 2, 32, 2,
           this.t % 8 < 4 ? 'rgba(255,60,60,.75)' : 'rgba(255,120,60,.35)');
      }
      drawBoss(g, b.x + ox, b.y, b.def, {
        pal: b.def.pal, h: b.h, facing: b.facing, state: b.state,
        phase: b.phase, atkT: b.atkT || 0, prop: b.def.prop,
        tint: b.flashT > 0 && b.flashT % 4 < 2 ? '#ffffff' : null,
      });
      if (b.stun > 0 && b.dying === 0) {             // "hit me" marker
        const yy = b.y - b.h - 8 + Math.sin(this.t * 0.2) * 2;
        px(g, b.x + ox - 1, yy, 3, 3, '#ffe066');
        px(g, b.x + ox - 3, yy + 3, 7, 2, '#ffe066');
      }
    }

    // projectiles
    for (const s of this.shots) {
      px(g, s.x + ox - 3, s.y - 3, 6, 6, '#4a4a55');
      px(g, s.x + ox - 2, s.y - 2, 4, 2, '#9aa4b8');
      this.parts.push({ kind: 'dust', x: s.x, y: s.y, vx: 0, vy: 0, life: 0.5, decay: 0.08 });
    }

    // player (flicker while invulnerable)
    const p = this.player;
    if (!(p.invuln > 0 && this.t % 6 < 3)) {
      drawHuman(g, p.x + ox, p.y, {
        pal: 'rebel', h: p.h, facing: p.facing, state: p.state,
        phase: p.phase, atkT: p.atkT, flag: true, prop: null,
      });
      // swing arc
      if (p.atk >= CFG.attackHit[0] && p.atk <= CFG.attackHit[1]) {
        const a = (p.atk - CFG.attackHit[0]) / (CFG.attackHit[1] - CFG.attackHit[0]);
        for (let i = 0; i < 5; i++) {
          const ang = -0.9 + a * 1.9 + i * 0.13;
          px(g, p.x + ox + Math.cos(ang) * 20 * p.facing,
                p.y - 15 + Math.sin(ang) * 13, 2, 2,
                `rgba(255,255,255,${0.55 - i * 0.09})`);
        }
      }
    }

    // particles
    for (const q of this.parts) {
      if (q.kind === 'spark') drawHitSpark(g, q.x + ox, q.y, q.life);
      else if (q.kind === 'dust') drawDust(g, q.x + ox, q.y, q.life);
    }

    g.restore();

    if (this.flash > 0) {
      g.fillStyle = `rgba(255,255,255,${this.flash * 0.5})`;
      g.fillRect(0, 0, CFG.W, CFG.H);
    }
  },

  drawTitleScene(g) {
    drawBackground(g, STAGES[4], this.t * 0.35, this.t);
    // a marching line across the bottom
    for (let i = 0; i < 7; i++) {
      const x = ((this.t * 0.5 + i * 46) % (CFG.W + 60)) - 30;
      drawHuman(g, x, CFG.GROUND, {
        pal: i === 3 ? 'rebel' : 'student', h: i === 3 ? 28 : 24, facing: 1,
        state: 'run', phase: ((this.t * 0.02) + i * 0.2) % 1, flag: i === 3 || i === 6,
      });
    }
    g.fillStyle = 'rgba(0,0,0,.35)'; g.fillRect(0, 0, CFG.W, 96);
  },

  drawWinScene(g) {
    drawBackground(g, STAGES[4], 500 + this.endT * 0.2, this.t);
    // the crowd floods in from both sides
    for (let i = 0; i < 9; i++) {
      const x = 20 + i * 34 + Math.sin(this.t * 0.03 + i) * 4;
      drawHuman(g, x, CFG.GROUND, {
        pal: i % 3 === 0 ? 'rebel' : 'student', h: 24 + (i % 3),
        facing: i % 2 ? 1 : -1, state: 'idle',
        phase: (this.t * 0.01 + i * 0.3) % 1, flag: i % 3 === 0,
      });
    }
    g.fillStyle = 'rgba(0,0,0,.40)'; g.fillRect(0, 0, CFG.W, 110);
  },
};
