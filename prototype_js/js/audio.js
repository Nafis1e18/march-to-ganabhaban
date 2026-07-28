/* ============================================================
   March to Ganabhaban  —  AUDIO
   Every sound is synthesised at runtime. No .mp3, no loading,
   no licensing headache for the hackathon.
   ============================================================ */
const SFX = {
  ctx: null, master: null, musicGain: null,
  on: true, musicOn: true,
  _timer: null, _step: 0, _nextT: 0,

  /* browsers require a user gesture before audio may start */
  init() {
    if (this.ctx) { if (this.ctx.state === 'suspended') this.ctx.resume(); return; }
    const AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) return;
    this.ctx = new AC();
    this.master = this.ctx.createGain();
    this.master.gain.value = 0.32;
    this.master.connect(this.ctx.destination);
    this.musicGain = this.ctx.createGain();
    this.musicGain.gain.value = 0.42;
    this.musicGain.connect(this.master);
  },

  /* one-shot oscillator blip */
  tone(freq, dur, type = 'square', vol = 0.5, slideTo = null, dest = null) {
    if (!this.ctx || !this.on) return;
    const t = this.ctx.currentTime;
    const o = this.ctx.createOscillator();
    const g = this.ctx.createGain();
    o.type = type;
    o.frequency.setValueAtTime(freq, t);
    if (slideTo) o.frequency.exponentialRampToValueAtTime(Math.max(20, slideTo), t + dur);
    g.gain.setValueAtTime(0, t);
    g.gain.linearRampToValueAtTime(vol, t + 0.008);
    g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    o.connect(g); g.connect(dest || this.master);
    o.start(t); o.stop(t + dur + 0.02);
  },

  /* filtered noise burst — impacts, drums, whooshes */
  noise(dur, vol = 0.4, freq = 1200, q = 1, dest = null) {
    if (!this.ctx || !this.on) return;
    const t = this.ctx.currentTime;
    const n = Math.floor(this.ctx.sampleRate * dur);
    const buf = this.ctx.createBuffer(1, n, this.ctx.sampleRate);
    const d = buf.getChannelData(0);
    for (let i = 0; i < n; i++) d[i] = (Math.random() * 2 - 1) * (1 - i / n);
    const src = this.ctx.createBufferSource(); src.buffer = buf;
    const f = this.ctx.createBiquadFilter();
    f.type = 'bandpass'; f.frequency.value = freq; f.Q.value = q;
    const g = this.ctx.createGain(); g.gain.value = vol;
    src.connect(f); f.connect(g); g.connect(dest || this.master);
    src.start(t);
  },

  /* ---- named game sounds ---- */
  jump()      { this.tone(340, 0.16, 'square', 0.34, 640); },
  land()      { this.noise(0.07, 0.20, 700, 1.2); },
  swing()     { this.noise(0.10, 0.16, 2400, 0.7); },
  hit()       { this.noise(0.10, 0.50, 420, 1.4); this.tone(180, 0.10, 'square', 0.32, 90); },
  kill()      { this.tone(520, 0.09, 'square', 0.30, 260);
                setTimeout(() => this.tone(300, 0.16, 'square', 0.26, 120), 70); },
  hurt()      { this.tone(220, 0.30, 'sawtooth', 0.40, 70); this.noise(0.16, 0.30, 300, 0.9); },
  bossHit()   { this.noise(0.13, 0.55, 260, 1.6); this.tone(120, 0.14, 'square', 0.34, 70); },
  bossRoar()  { this.tone(90, 0.75, 'sawtooth', 0.40, 55);
                this.noise(0.55, 0.28, 200, 0.6); },
  combo(n)    { this.tone(440 + Math.min(n, 8) * 65, 0.10, 'triangle', 0.30); },
  coin()      { this.tone(880, 0.06, 'square', 0.22);
                setTimeout(() => this.tone(1320, 0.10, 'square', 0.22), 55); },
  select()    { this.tone(660, 0.06, 'square', 0.26); },

  victory() {
    [523, 659, 784, 1047].forEach((f, i) =>
      setTimeout(() => this.tone(f, 0.30, 'triangle', 0.34), i * 130));
  },
  defeat() {
    [392, 330, 262, 196].forEach((f, i) =>
      setTimeout(() => this.tone(f, 0.38, 'sawtooth', 0.30), i * 190));
  },

  /* ============================================================
     MUSIC — a marching loop. Bass drum on the beat, snare on the
     off-beat, and a minor-key ostinato that climbs a step every
     stage so later levels feel more urgent.
     ============================================================ */
  MELODY: [0, 3, 7, 3, 5, 3, 0, -2],           // semitones over the root
  root: 220,
  bpm: 132,

  startMusic(stageIndex = 0) {
    if (!this.ctx || !this.musicOn) return;
    this.stopMusic();
    this.root = 220 * Math.pow(2, (stageIndex * 1) / 12);   // +1 semitone per stage
    this._step = 0;
    this._nextT = this.ctx.currentTime + 0.06;
    this._timer = setInterval(() => this._schedule(), 25);
  },

  stopMusic() { if (this._timer) { clearInterval(this._timer); this._timer = null; } },

  setBpm(v) { this.bpm = v; },

  _schedule() {
    if (!this.ctx) return;
    const spb = 60 / this.bpm / 2;                // an eighth note
    while (this._nextT < this.ctx.currentTime + 0.12) {
      const s = this._step % 8;
      const t = this._nextT;

      if (s % 4 === 0) this._drum(t, 62, 0.16, 0.55);        // kick
      if (s % 4 === 2) this._snare(t);
      if (s % 2 === 1) this._hat(t);

      const semi = this.MELODY[s];
      this._blip(t, this.root * Math.pow(2, semi / 12), spb * 0.85);
      if (s === 0) this._blip(t, this.root / 2, spb * 1.8, 0.16, 'triangle');

      this._nextT += spb;
      this._step++;
    }
  },

  _drum(t, f, dur, vol) {
    const o = this.ctx.createOscillator(), g = this.ctx.createGain();
    o.type = 'sine'; o.frequency.setValueAtTime(f * 2.4, t);
    o.frequency.exponentialRampToValueAtTime(f * 0.6, t + dur);
    g.gain.setValueAtTime(vol, t);
    g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    o.connect(g); g.connect(this.musicGain); o.start(t); o.stop(t + dur + 0.02);
  },
  _snare(t) {
    const n = Math.floor(this.ctx.sampleRate * 0.09);
    const buf = this.ctx.createBuffer(1, n, this.ctx.sampleRate);
    const d = buf.getChannelData(0);
    for (let i = 0; i < n; i++) d[i] = (Math.random() * 2 - 1) * (1 - i / n) ** 1.5;
    const src = this.ctx.createBufferSource(); src.buffer = buf;
    const f = this.ctx.createBiquadFilter(); f.type = 'highpass'; f.frequency.value = 1400;
    const g = this.ctx.createGain(); g.gain.value = 0.20;
    src.connect(f); f.connect(g); g.connect(this.musicGain); src.start(t);
  },
  _hat(t) {
    const n = Math.floor(this.ctx.sampleRate * 0.03);
    const buf = this.ctx.createBuffer(1, n, this.ctx.sampleRate);
    const d = buf.getChannelData(0);
    for (let i = 0; i < n; i++) d[i] = (Math.random() * 2 - 1) * (1 - i / n) ** 3;
    const src = this.ctx.createBufferSource(); src.buffer = buf;
    const f = this.ctx.createBiquadFilter(); f.type = 'highpass'; f.frequency.value = 7000;
    const g = this.ctx.createGain(); g.gain.value = 0.09;
    src.connect(f); f.connect(g); g.connect(this.musicGain); src.start(t);
  },
  _blip(t, freq, dur, vol = 0.10, type = 'square') {
    const o = this.ctx.createOscillator(), g = this.ctx.createGain();
    o.type = type; o.frequency.setValueAtTime(freq, t);
    g.gain.setValueAtTime(0, t);
    g.gain.linearRampToValueAtTime(vol, t + 0.01);
    g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    o.connect(g); g.connect(this.musicGain); o.start(t); o.stop(t + dur + 0.02);
  },
};
