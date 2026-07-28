// ============================================================
//  Bangla UI text.
//
//  raylib draws glyphs one codepoint at a time with no shaping,
//  which mangles Bengali: matras have to be reordered and
//  conjuncts joined. So every string is pre-shaped by Windows'
//  DirectWrite (tools/make_bangla.ps1) and baked to a PNG; here
//  we just blit those.
//
//  They are rendered oversized and drawn scaled DOWN to a target
//  height, with bilinear filtering â€” small Bangla is far more
//  legible smoothed than nearest-neighboured into mush.
// ============================================================
#include "text.h"
#include "textids.h"

static Texture2D g_tex[TX_COUNT];
static bool g_loaded = false;

void LoadUIText() {
    if (g_loaded) return;
    for (int i = 0; i < TX_COUNT; i++) {
        if (!FileExists(TEXT_FILES[i])) {
            g_tex[i] = Texture2D{};
            TraceLog(LOG_WARNING, "missing UI text image: %s", TEXT_FILES[i]);
            continue;
        }
        g_tex[i] = LoadTexture(TEXT_FILES[i]);
        if (g_tex[i].id == 0) { TraceLog(LOG_WARNING, "text load failed: %s", TEXT_FILES[i]); continue; }
        SetTextureFilter(g_tex[i], TEXTURE_FILTER_BILINEAR);
    }
    g_loaded = true;
}

void UnloadUIText() {
    if (!g_loaded) return;
    for (int i = 0; i < TX_COUNT; i++)
        if (g_tex[i].id) UnloadTexture(g_tex[i]);
    g_loaded = false;
}

// Coordinates are always given in 384x224 game space. When the UI is drawn in
// the post-upscale pass this multiplies them up to window pixels, so the glyphs
// are resampled ONCE (source PNG -> screen) instead of twice. Squeezing them
// into the small buffer and then blowing that buffer up is what made the Bangla
// unreadable: Bengali matras and dots do not survive a double resample.
static float g_uiScale = 1.0f;
void SetUIScale(float s) { g_uiScale = s; }

float UITextW(int id, float h) {
    if (id < 0 || id >= TX_COUNT || !g_tex[id].id || g_tex[id].height == 0) return 0.0f;
    return g_tex[id].width * (h / (float)g_tex[id].height);
}

void DrawUIText(int id, float x, float y, float h, Color tint) {
    if (id < 0 || id >= TX_COUNT || !g_tex[id].id || g_tex[id].height == 0) return;
    const Texture2D& t = g_tex[id];
    float w = t.width * (h / (float)t.height);
    const float k = g_uiScale;
    DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height },
                      { x * k, y * k, w * k, h * k }, { 0, 0 }, 0.0f, tint);
}

void DrawUITextC(int id, float cx, float y, float h, Color tint) {
    DrawUIText(id, cx - UITextW(id, h) * 0.5f, y, h, tint);
}

