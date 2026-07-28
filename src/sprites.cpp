// ============================================================
//  Sprite sheets — loading and drawing your art
//
//  Two things here are worth understanding:
//
//  1. PER-ANIMATION FALLBACK. On load we scan every cell for any
//     non-transparent pixel. An animation counts as "painted"
//     only when all of its frames have something in them. So a
//     sheet with just idle and walk drawn works immediately —
//     those use your art, everything else keeps using the
//     placeholder skeleton. You never have a broken half-state.
//
//  2. THE ANCHOR. Your drawing has its feet on the red baseline
//     at y=60 and its body on the blue line at x=26. The engine
//     positions characters by the point between their feet, so
//     we offset the destination rectangle by that anchor rather
//     than by the corner of the cell.
// ============================================================
#include "sprites.h"
#include <cstdio>
#include <cstring>

struct Sheet {
    Texture2D tex{};
    bool loaded = false;
    char path[256] = {0};
    bool framePainted[SHEET_COLS * 8] = {false};
    bool animOK[A_COUNT] = {false};
    int  painted = 0;
};

static Sheet g_sheets[8];

static void ScanPaintedFrames(Sheet& s, Image img, int kind) {
    // Force a known layout so we can read alpha bytes directly; GetImageColor
    // per pixel would be ~120k virtual calls per sheet.
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const unsigned char* px = (const unsigned char*)img.data;
    const int cols = img.width / SHEET_FW;
    const int rows = img.height / SHEET_FH;
    const int total = cols * rows;

    s.painted = 0;
    for (int i = 0; i < total && i < (int)(sizeof(s.framePainted) / sizeof(bool)); i++) {
        const int cx = (i % cols) * SHEET_FW;
        const int cy = (i / cols) * SHEET_FH;
        bool any = false;
        for (int y = 0; y < SHEET_FH && !any; y++) {
            const unsigned char* row = px + ((size_t)(cy + y) * img.width + cx) * 4;
            for (int x = 0; x < SHEET_FW; x++)
                if (row[x * 4 + 3] > 8) { any = true; break; }
        }
        s.framePainted[i] = any;
        if (any) s.painted++;
    }

    // An animation is usable only when every one of its frames is drawn.
    // Partially-painted animations would flicker between your art and the
    // skeleton mid-swing, which looks like a bug rather than a work in progress.
    for (int a = 0; a < A_COUNT; a++) {
        const AnimDef& d = GetAnim(kind, a);
        bool ok = d.count > 0;
        for (int k = 0; k < d.count; k++) {
            int idx = d.first + k;
            if (idx >= total || !s.framePainted[idx]) { ok = false; break; }
        }
        s.animOK[a] = ok;
    }
}

void LoadCharacterSheet(int palette, const char* path) {
    if (palette < 0 || palette >= 8) return;
    Sheet& s = g_sheets[palette];

    if (!FileExists(path)) {
        // Perfectly normal before you have drawn that character.
        TraceLog(LOG_INFO, "sheet not found (using placeholder art): %s", path);
        return;
    }
    Image img = LoadImage(path);
    if (img.data == nullptr) {
        TraceLog(LOG_WARNING, "sheet failed to load: %s", path);
        return;
    }
    if (img.width % SHEET_FW || img.height % SHEET_FH) {
        TraceLog(LOG_WARNING,
                 "sheet %s is %dx%d which is not a whole number of %dx%d cells",
                 path, img.width, img.height, SHEET_FW, SHEET_FH);
    }

    if (s.loaded) UnloadTexture(s.tex);
    ScanPaintedFrames(s, img, palette);
    s.tex = LoadTextureFromImage(img);
    SetTextureFilter(s.tex, TEXTURE_FILTER_POINT);   // keep it crisp when scaled
    UnloadImage(img);
    s.loaded = true;
    snprintf(s.path, sizeof(s.path), "%s", path);

    int anims = 0;
    for (int a = 0; a < A_COUNT; a++) if (s.animOK[a]) anims++;
    TraceLog(LOG_INFO, "sheet %s: %d frames painted, %d/%d animations live",
             path, s.painted, anims, A_COUNT);
}

void ReloadCharacterSheets() {
    for (int i = 0; i < 8; i++)
        if (g_sheets[i].path[0]) LoadCharacterSheet(i, g_sheets[i].path);
}

void UnloadCharacterSheets() {
    for (int i = 0; i < 8; i++)
        if (g_sheets[i].loaded) { UnloadTexture(g_sheets[i].tex); g_sheets[i].loaded = false; }
}

int  SheetPaintedCount(int palette) { return (palette >= 0 && palette < 8) ? g_sheets[palette].painted : 0; }
bool SheetLoaded(int palette)       { return (palette >= 0 && palette < 8) && g_sheets[palette].loaded; }

bool DrawFighterSprite(const Fighter& f, float sx, float sy) {
    if (f.kind < 0 || f.kind >= 8) return false;
    const Sheet& s = g_sheets[f.kind];
    if (!s.loaded || !s.animOK[f.anim]) return false;      // -> skeleton fallback

    const AnimDef& d = GetAnim(f.kind, f.anim);
    float dur = d.count / d.fps;
    int k;
    if (d.loop) {
        k = (int)(f.animT * d.fps) % d.count;
    } else {
        float t = (dur > 0) ? (f.animT / dur) : 1.0f;
        k = (int)(t * d.count);
        if (k >= d.count) k = d.count - 1;                 // hold the last frame
        if (k < 0) k = 0;
    }
    const int idx = d.first + k;
    const int cols = s.tex.width / SHEET_FW;
    const int cx = (idx % cols) * SHEET_FW;
    const int cy = (idx / cols) * SHEET_FH;

    // Scale so the drawn character matches this fighter's height in the world.
    const float scale = (float)f.height / (float)SHEET_CHARH;

    // Mirror by flipping the source rect. When flipped, the anchor column
    // measured from the left becomes (FW - MID), so the offset must flip too
    // or the character slides sideways every time they turn around.
    const bool flip = (f.facing < 0);
    const float anchorX = flip ? (float)(SHEET_FW - SHEET_MID) : (float)SHEET_MID;

    Rectangle src = { (float)cx, (float)cy,
                      flip ? -(float)SHEET_FW : (float)SHEET_FW, (float)SHEET_FH };
    Rectangle dst = { sx - anchorX * scale,
                      sy - (float)SHEET_BASE * scale,
                      SHEET_FW * scale, SHEET_FH * scale };

    // (invulnerability flicker is handled by the caller, in DrawFighter)
    DrawTexturePro(s.tex, src, dst, { 0, 0 }, 0.0f, WHITE);
    return true;
}
