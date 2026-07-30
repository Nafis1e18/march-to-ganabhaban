//  Audio — gameplay sound is synthesised at start-up.
//
//  The one exception is the user-supplied MP3 played once on the final
//  victory screen. All combat SFX and the marching stage music remain
//  generated here, keeping the rest of the audio compact.
//
//  The music is a four-bar marching loop: kick on the beat,
//  snare on the off-beat, and a minor ostinato that transposes
//  up one semitone per stage so later levels feel more urgent
//  without needing a second piece of music.
#include "audio.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

static bool  g_ready = false;
static Sound g_sfx[SFX_COUNT];
static Sound g_music;
static bool  g_musicOn = false;
static int   g_musicStage = -1;
static Sound g_finalVictory;
static bool  g_finalVictoryLoaded = false;

static constexpr const char* FINAL_VICTORY_AUDIO =
    "assets/audio/seikh_hasina_palay na.mp3";

static constexpr int SR = 22050;    // plenty for chiptune-style material

// ---- tiny synth helpers -------------------------------------------------
struct Buf {
    std::vector<float> s;
    explicit Buf(float seconds) : s((size_t)(SR * seconds), 0.0f) {}
    size_t n() const { return s.size(); }
    void add(size_t i, float v) { if (i < s.size()) s[i] += v; }
};

// square/saw/sine partials with an exponential decay
static void Tone(Buf& b, float t0, float dur, float f0, float f1,
                 float amp, int shape, float decay = 6.0f) {
    size_t a = (size_t)(t0 * SR), n = (size_t)(dur * SR);
    for (size_t i = 0; i < n; i++) {
        float u = (float)i / n;
        float f = f0 + (f1 - f0) * u;
        float ph = 2.0f * PI * f * ((float)i / SR);
        float v;
        switch (shape) {
        case 0:  v = sinf(ph) >= 0 ? 1.0f : -1.0f; break;              // square
        case 1:  v = 2.0f * (fmodf(ph / (2 * PI), 1.0f)) - 1.0f; break; // saw
        default: v = sinf(ph); break;                                   // sine
        }
        b.add(a + i, v * amp * expf(-decay * u));
    }
}

static void Noise(Buf& b, float t0, float dur, float amp, float decay = 12.0f) {
    size_t a = (size_t)(t0 * SR), n = (size_t)(dur * SR);
    float lp = 0;
    for (size_t i = 0; i < n; i++) {
        float u = (float)i / n;
        float w = ((float)rand() / (float)((unsigned)RAND_MAX + 1u)) * 2.0f - 1.0f;
        lp = lp * 0.6f + w * 0.4f;                       // soften the hiss
        b.add(a + i, lp * amp * expf(-decay * u));
    }
}

static Sound Bake(const Buf& b) {
    Wave w{};
    w.frameCount = (unsigned int)b.n();
    w.sampleRate = SR;
    w.sampleSize = 16;
    w.channels = 1;
    short* d = (short*)RL_MALLOC(b.n() * sizeof(short));
    for (size_t i = 0; i < b.n(); i++) {
        float v = b.s[i];
        v = v > 1.0f ? 1.0f : (v < -1.0f ? -1.0f : v);
        d[i] = (short)(v * 32000.0f);
    }
    w.data = d;
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

// ---- music --------------------------------------------------------------
// Minor ostinato, in semitones over the root.
static const int MELODY[8] = { 0, 3, 7, 3, 5, 3, 0, -2 };

static Sound BakeMusic(int stage) {
    const float bpm = 132.0f + stage * 6.0f;
    const float beat = 60.0f / bpm;
    const float eighth = beat * 0.5f;
    const int   bars = 4, steps = bars * 8;
    Buf b(steps * eighth + 0.4f);

    const float root = 110.0f * powf(2.0f, stage / 12.0f);

    for (int i = 0; i < steps; i++) {
        float t = i * eighth;
        int s = i % 8;
        if (s % 4 == 0) Tone(b, t, 0.16f, 150.0f, 45.0f, 0.55f, 2, 9.0f);   // kick
        if (s % 4 == 2) Noise(b, t, 0.10f, 0.26f, 26.0f);                   // snare
        if (s % 2 == 1) Noise(b, t, 0.03f, 0.07f, 60.0f);                   // hat
        float f = root * powf(2.0f, MELODY[s] / 12.0f);
        Tone(b, t, eighth * 0.9f, f, f, 0.11f, 0, 5.0f);                    // lead
        if (s == 0) Tone(b, t, beat * 1.6f, root * 0.5f, root * 0.5f, 0.15f, 2, 2.2f);
    }
    return Bake(b);
}

// ---- public -------------------------------------------------------------
void InitGameAudio() {
    if (g_ready) return;

    { Buf b(0.16f); Noise(b, 0, 0.09f, 0.55f, 30.0f);
      Tone(b, 0, 0.10f, 190, 70, 0.40f, 0, 16.0f);      g_sfx[SFX_HIT]      = Bake(b); }
    { Buf b(0.10f); Noise(b, 0, 0.07f, 0.16f, 40.0f);   g_sfx[SFX_SWING]    = Bake(b); }
    { Buf b(0.30f); Tone(b, 0, 0.10f, 520, 300, 0.32f, 0, 10.0f);
      Tone(b, 0.07f, 0.18f, 300, 120, 0.28f, 0, 8.0f);  g_sfx[SFX_KILL]     = Bake(b); }
    { Buf b(0.34f); Tone(b, 0, 0.30f, 230, 70, 0.42f, 1, 7.0f);
      Noise(b, 0, 0.16f, 0.30f, 16.0f);                 g_sfx[SFX_HURT]     = Bake(b); }
    { Buf b(0.18f); Tone(b, 0, 0.15f, 330, 620, 0.30f, 0, 7.0f); g_sfx[SFX_JUMP] = Bake(b); }
    { Buf b(0.12f); Noise(b, 0, 0.08f, 0.22f, 26.0f);   g_sfx[SFX_LAND]     = Bake(b); }
    { Buf b(0.90f); Tone(b, 0, 0.75f, 105, 48, 0.50f, 1, 2.6f);
      Noise(b, 0, 0.55f, 0.26f, 5.0f);                  g_sfx[SFX_BOSS]     = Bake(b); }
    { Buf b(0.24f); Tone(b, 0, 0.06f, 880, 880, 0.24f, 0, 8.0f);
      Tone(b, 0.06f, 0.12f, 1320, 1320, 0.22f, 0, 8.0f); g_sfx[SFX_SCORE]   = Bake(b); }
    { Buf b(1.30f);
      const float n[4] = { 523, 659, 784, 1047 };
      for (int i = 0; i < 4; i++) Tone(b, i * 0.14f, 0.42f, n[i], n[i], 0.30f, 2, 3.0f);
      g_sfx[SFX_CLEAR] = Bake(b); }
    { Buf b(1.60f);
      const float n[4] = { 392, 330, 262, 196 };
      for (int i = 0; i < 4; i++) Tone(b, i * 0.20f, 0.55f, n[i], n[i], 0.32f, 1, 2.4f);
      g_sfx[SFX_GAMEOVER] = Bake(b); }
    { Buf b(0.20f); Tone(b, 0, 0.16f, 660, 990, 0.26f, 0, 6.0f); g_sfx[SFX_SELECT] = Bake(b); }

    if (FileExists(FINAL_VICTORY_AUDIO)) {
        g_finalVictory = LoadSound(FINAL_VICTORY_AUDIO);
        g_finalVictoryLoaded = IsSoundValid(g_finalVictory);
        if (g_finalVictoryLoaded) SetSoundVolume(g_finalVictory, 1.0f);
        else TraceLog(LOG_WARNING, "final victory audio failed to load: %s",
                      FINAL_VICTORY_AUDIO);
    } else {
        TraceLog(LOG_WARNING, "final victory audio is missing: %s",
                 FINAL_VICTORY_AUDIO);
    }

    g_ready = true;
}

void PlaySfx(int id) {
    if (!g_ready || id < 0 || id >= SFX_COUNT) return;
    PlaySound(g_sfx[id]);
}

void StartMusic(int stage) {
    if (!g_ready) return;
    if (g_musicOn && g_musicStage == stage) return;
    StopMusic();
    g_music = BakeMusic(stage);
    SetSoundVolume(g_music, 0.42f);
    PlaySound(g_music);
    g_musicOn = true;
    g_musicStage = stage;
}

// raylib Sounds do not loop, so re-trigger when the buffer runs out.
void UpdateMusic() {
    if (g_musicOn && !IsSoundPlaying(g_music)) PlaySound(g_music);
}

void StopMusic() {
    if (!g_musicOn) return;
    StopSound(g_music);
    UnloadSound(g_music);
    g_musicOn = false;
    g_musicStage = -1;
}

void PlayFinalVictoryAudio() {
    if (!g_ready || !g_finalVictoryLoaded) return;
    // The state transition calls this once. Stop first only as protection
    // against an accidental second transition; the clip itself never loops.
    StopSound(g_sfx[SFX_CLEAR]);
    StopSound(g_finalVictory);
    PlaySound(g_finalVictory);
}

void StopFinalVictoryAudio() {
    if (g_finalVictoryLoaded && IsSoundPlaying(g_finalVictory))
        StopSound(g_finalVictory);
}

void UnloadGameAudio() {
    if (!g_ready) return;
    StopMusic();
    StopFinalVictoryAudio();
    if (g_finalVictoryLoaded) {
        UnloadSound(g_finalVictory);
        g_finalVictory = Sound{};
        g_finalVictoryLoaded = false;
    }
    for (int i = 0; i < SFX_COUNT; i++) UnloadSound(g_sfx[i]);
    g_ready = false;
}
