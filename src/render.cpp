// ============================================================
//  Placeholder art.
//  Characters are a small skeleton â€” hip, shoulder, knee, elbow â€”
//  posed per frame. This exists so the game is playable and
//  readable BEFORE your drawings are finished. When a sprite
//  sheet loads, sprites.cpp takes over and this is never called.
// ============================================================
#include "game.h"
#include "sprites.h"
#include <cmath>

const Palette PALETTES[] = {
    /* rebel  */ { {201,138, 90,255}, { 36, 26, 20,255}, { 31,143, 78,255}, { 37, 50, 74,255}, {214, 40, 40,255} },
    /* chhatra*/ { {192,128, 80,255}, {122, 59, 42,255}, {201, 85, 61,255}, { 58, 58, 68,255}, {224,224,224,255} },
    /* police */ { {184,121, 76,255}, { 42, 53, 80,255}, { 58, 74,114,255}, { 36, 44, 68,255}, {143,163,208,255} },
    /* jallad */ { {168,106, 60,255}, { 30, 30, 40,255}, {107,111,122,255}, { 46, 46, 54,255}, {192,196,204,255} },
};
const int PALETTE_COUNT = 4;

static Color Shade(Color c, int amt) {
    auto cl = [](int v) { return (unsigned char)(v < 0 ? 0 : v > 255 ? 255 : v); };
    return { cl(c.r + amt), cl(c.g + amt), cl(c.b + amt), c.a };
}

// integer-stepped line, so limbs stay crisp instead of antialiasing into mush
static void PixLine(float x1, float y1, float x2, float y2, int w, Color c) {
    float dx = x2 - x1, dy = y2 - y1;
    int steps = (int)fmaxf(1.0f, ceilf(sqrtf(dx * dx + dy * dy)));
    float o = (w - 1) / 2.0f;
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        DrawRectangle((int)roundf(x1 + dx * t - o), (int)roundf(y1 + dy * t - o), w, w, c);
    }
}

// angle 0 points straight DOWN, positive swings forward
static Vector2 Joint(float x, float y, float a, float len) {
    return { x + sinf(a) * len, y + cosf(a) * len };
}

struct Pose {
    float lean = 0, bob = 0, crouch = 0;
    float thighF = 0, shinF = 0, thighB = 0, shinB = 0;
    float armF = 0, foreF = 0, armB = 0, foreB = 0;
};

// `t` is 0..1 progress through a one-shot move; `ph` cycles 0..1 for loops
static Pose GetPose(Anim a, float ph, float t) {
    Pose p;
    const float TAU = 6.28318f;
    float s = sinf(ph * TAU), c = cosf(ph * TAU);

    switch (a) {
    case A_RUN:
        p.lean = 0.34f;
        p.bob = fabsf(sinf(ph * TAU * 2)) * 1.7f;
        p.thighF = s * 1.0f;  p.thighB = -s * 1.0f;
        p.shinF = fmaxf(0, -c) * 1.3f; p.shinB = fmaxf(0, c) * 1.3f;
        p.armF = -s * 0.9f; p.foreF = 0.8f;
        p.armB =  s * 0.9f; p.foreB = 0.8f;
        break;

    case A_WALK:
        p.lean = 0.07f;
        p.bob = fabsf(sinf(ph * TAU * 2)) * 1.1f;
        p.thighF = s * 0.62f;  p.thighB = -s * 0.62f;
        p.shinF = fmaxf(0, -c) * 0.85f; p.shinB = fmaxf(0, c) * 0.85f;
        p.armF = -s * 0.5f; p.foreF = 0.45f;
        p.armB =  s * 0.5f; p.foreB = 0.45f;
        break;

    // the three-hit chain: jab, cross, hook. Each winds up, snaps, recovers.
    case A_PUNCH1: case A_PUNCH2: {
        float k = t < 0.30f ? -(t / 0.30f) * 0.8f
                : t < 0.50f ? ((t - 0.30f) / 0.20f) * 2.4f - 0.8f
                            : 1.6f - ((t - 0.50f) / 0.50f) * 1.6f;
        bool second = (a == A_PUNCH2);
        p.lean = (second ? 0.18f : 0.12f) + k * 0.08f;
        if (second) { p.armB = -1.45f - k * 0.30f; p.foreB = fmaxf(0, 0.95f - k * 0.95f);
                      p.armF = 0.85f;  p.foreF = 0.75f; }
        else        { p.armF = -1.45f - k * 0.25f; p.foreF = fmaxf(0, 0.95f - k * 0.95f);
                      p.armB = 0.75f;  p.foreB = 0.70f; }
        p.thighF = 0.46f; p.shinF = 0.10f; p.thighB = -0.46f; p.shinB = 0.45f;
        break;
    }
    case A_PUNCH3: {
        float k = t < 0.35f ? -(t / 0.35f) * 1.2f
                : t < 0.55f ? ((t - 0.35f) / 0.20f) * 3.4f - 1.2f
                            : 2.2f - ((t - 0.55f) / 0.45f) * 2.2f;
        p.lean = 0.24f + k * 0.20f;  p.bob = -k * 1.2f;
        p.armF = -1.30f - k * 0.55f; p.foreF = fmaxf(-0.3f, 1.15f - k * 1.35f);
        p.armB = 1.15f; p.foreB = 0.85f;
        p.thighF = 0.62f + k * 0.15f; p.shinF = 0.12f; p.thighB = -0.58f; p.shinB = 0.55f;
        break;
    }
    case A_KICK: {
        float k = t < 0.30f ? (t / 0.30f) * 0.5f
                : t < 0.55f ? 0.5f + ((t - 0.30f) / 0.25f) * 1.1f
                            : 1.6f - ((t - 0.55f) / 0.45f) * 1.6f;
        p.lean = -0.16f - k * 0.10f;
        p.thighF = 0.30f + k * 1.15f; p.shinF = fmaxf(0, 0.75f - k * 0.85f);
        p.thighB = -0.20f; p.shinB = 0.22f;
        p.armF = -0.75f - k * 0.30f; p.foreF = 0.85f;
        p.armB =  0.95f + k * 0.35f; p.foreB = 0.70f;
        break;
    }
    case A_JUMP:
        p.lean = 0.10f;
        p.thighF = 1.05f; p.shinF = 0.80f; p.thighB = -0.35f; p.shinB = 1.30f;
        p.armF = -1.45f; p.foreF = 0.55f; p.armB = -0.85f; p.foreB = 0.85f;
        break;
    case A_JUMPKICK:
        p.lean = -0.10f;
        p.thighF = 1.45f; p.shinF = 0.05f; p.thighB = -0.25f; p.shinB = 1.45f;
        p.armF = -1.9f; p.foreF = 0.5f; p.armB = 1.5f; p.foreB = 0.6f;
        break;
    case A_HURT:
        p.lean = -0.45f;
        p.armF = -2.3f; p.foreF = 0.55f; p.armB = -2.0f; p.foreB = 0.55f;
        p.thighF = 0.55f; p.shinF = 0.40f; p.thighB = -0.30f; p.shinB = 0.32f;
        break;
    case A_DOWN:
        p.lean = 1.48f;
        p.armF = 1.85f; p.armB = 2.15f; p.foreF = 0.25f; p.foreB = 0.25f;
        p.thighF = 1.65f; p.thighB = 1.40f; p.shinF = 0.45f; p.shinB = 0.35f;
        break;
    case A_GETUP:
        p.lean = 0.75f * (1 - t); p.crouch = 6 * (1 - t);
        p.armF = -0.55f; p.foreF = 0.85f; p.armB = 0.5f; p.foreB = 0.6f;
        p.thighF = 0.85f; p.shinF = 0.75f; p.thighB = -0.55f; p.shinB = 0.85f;
        break;
    default:                                  // idle â€” small breathing bob
        p.lean = 0.03f; p.bob = s * 0.55f;
        p.armF = 0.14f + s * 0.05f; p.foreF = 0.22f;
        p.armB = -0.14f - s * 0.05f; p.foreB = 0.22f;
        p.thighF = 0.10f; p.thighB = -0.10f;
        break;
    }
    return p;
}

void DrawShadowAt(float sx, float beltY, int h, float alpha) {
    // The shadow stays on the ground while the body rises. Without it you
    // cannot tell how high a jump is, or which lane someone is standing in.
    DrawEllipse((int)sx, (int)beltY, h * 0.30f, h * 0.10f,
                { 0, 0, 0, (unsigned char)(alpha * 255.0f) });
}

void DrawFighter(const Fighter& f, int camX) {
    float sx = f.x - camX;
    float belt = f.z;
    // The hand-drawn frames already have a shadow painted under them, so keep
    // ours faint there â€” it still has to exist, because the painted one rises
    // with the body during a jump and yours is what actually reads as height.
    DrawShadowAt(sx, belt, f.height);

    // flicker while invulnerable
    if (f.invuln > 0 && (f.invuln / 3) % 2 == 0) return;

    float sy = f.screenY();

    // Your drawings take priority. This returns false when the character, or
    // just this one animation, has not been painted yet â€” then we fall through
    // to the placeholder skeleton below.
    if (DrawFighterSprite(f, sx, sy)) return;

    const AnimDef& ad = GetAnim(f.kind, f.anim);
    float dur = ad.count / ad.fps;
    float t = ad.loop ? 0.0f : fminf(1.0f, f.animT / dur);
    float ph = ad.loop ? fmodf(f.animT * ad.fps / ad.count, 1.0f) : 0.0f;

    Pose p = GetPose(f.anim, ph, t);

    const Palette& pal = PALETTES[f.palette % PALETTE_COUNT];
    Color skin = pal.skin, hair = pal.hair, shirt = pal.shirt, pant = pal.pant;
    Color shirtD = Shade(shirt, -34), pantD = Shade(pant, -30);

    float h = (float)f.height;
    int fc = f.facing;
    float y = sy + p.crouch;
    float hipY = y - h * 0.44f + p.bob;
    float shldY = y - h * 0.74f + p.bob;
    float lean = p.lean * fc;
    float hipX = sx;
    float shldX = sx + sinf(lean) * (h * 0.30f);
    float headX = shldX + sinf(lean) * (h * 0.12f);
    float headY = shldY - h * 0.12f;
    float headR = fmaxf(2.0f, h * 0.125f);

    float thigh = h * 0.235f, shin = h * 0.235f;
    float upper = h * 0.195f, fore = h * 0.195f;
    int   lw = (int)fmaxf(2.0f, h * 0.085f);

    // far-side limbs first, darker, so depth reads
    Vector2 kB = Joint(hipX, hipY, p.thighB * fc + lean, thigh);
    Vector2 fB = Joint(kB.x, kB.y, (p.thighB + p.shinB) * fc + lean, shin);
    PixLine(hipX, hipY, kB.x, kB.y, lw, pantD);
    PixLine(kB.x, kB.y, fB.x, fB.y, lw, pantD);

    Vector2 eB = Joint(shldX, shldY, p.armB * fc + lean, upper);
    Vector2 hB = Joint(eB.x, eB.y, (p.armB + p.foreB) * fc + lean, fore);
    PixLine(shldX, shldY, eB.x, eB.y, lw - 1, Shade(shirtD, -18));
    PixLine(eB.x, eB.y, hB.x, hB.y, lw - 1, Shade(skin, -35));

    // torso, tapering from hip to shoulder
    float tw = fmaxf(4.0f, h * 0.30f);
    for (int i = 0; i <= 8; i++) {
        float q = i / 8.0f;
        float cx = hipX + (shldX - hipX) * q;
        float cy = hipY + (shldY - hipY) * q;
        float w = tw * (0.84f + 0.16f * q);
        DrawRectangle((int)(cx - w / 2), (int)(cy - 1.5f), (int)w, 3, i < 2 ? pant : shirt);
    }

    // head
    DrawCircle((int)headX, (int)headY, headR, skin);
    DrawCircle((int)headX, (int)(headY - headR * 0.45f), headR * 0.92f, hair);
    if (f.palette == PAL_POLICE) {                       // riot helmet + visor
        DrawCircle((int)headX, (int)(headY - headR * 0.35f), headR * 1.15f, Shade(pal.hair, 22));
        DrawRectangle((int)(headX - headR), (int)(headY - headR * 0.1f),
                      (int)(headR * 2 + 1), 3, { 160, 200, 255, 76 });
    }
    if (f.palette == PAL_REBEL) {                        // red headband
        DrawRectangle((int)(headX - headR - 1), (int)(headY - headR * 0.15f),
                      (int)(headR * 2 + 3), 2, { 214, 40, 40, 255 });
    }

    // near-side limbs
    Vector2 kF = Joint(hipX, hipY, p.thighF * fc + lean, thigh);
    Vector2 fF = Joint(kF.x, kF.y, (p.thighF + p.shinF) * fc + lean, shin);
    PixLine(hipX, hipY, kF.x, kF.y, lw, pant);
    PixLine(kF.x, kF.y, fF.x, fF.y, lw, pant);

    Vector2 eF = Joint(shldX, shldY, p.armF * fc + lean, upper);
    Vector2 hF = Joint(eF.x, eF.y, (p.armF + p.foreF) * fc + lean, fore);
    PixLine(shldX, shldY, eF.x, eF.y, lw - 1, shirt);
    PixLine(eF.x, eF.y, hF.x, hF.y, lw - 1, skin);
}

// ---------- projectiles ----------
void DrawProjectile(const Projectile& p, int camX, int frame) {
    float sx = p.x - camX;
    float sy = p.z - p.y;
    DrawShadowAt(sx, p.z, 10, 0.20f);          // ground marker, so depth reads

    if (p.kind == 1) {                          // thrown tissue â€” tumbles
        float w = 4 + sinf(frame * 0.4f) * 2;
        DrawRectangle((int)(sx - w / 2), (int)(sy - 3), (int)w, 4, { 245, 245, 238, 255 });
        DrawRectangle((int)(sx - w / 2), (int)(sy - 3), (int)w, 1, { 200, 200, 195, 255 });
    } else if (p.kind == 2) {                   // grenade
        DrawCircle((int)sx, (int)sy - 2, 3, { 70, 88, 56, 255 });
        DrawRectangle((int)sx - 1, (int)sy - 6, 2, 2, { 140, 140, 140, 255 });
    } else {                                    // bullet + tracer
        int dir = (p.vx > 0) ? 1 : -1;
        for (int i = 0; i < 4; i++)
            DrawRectangle((int)(sx - dir * i * 3), (int)sy - 2, 3, 2,
                          { 255, (unsigned char)(230 - i * 40), 120,
                            (unsigned char)(230 - i * 50) });
        DrawRectangle((int)sx, (int)sy - 3, 3, 3, { 255, 245, 200, 255 });
    }
}

// The wind-up needs to be unmissable â€” a red line along the lane he is about
// to fire down, blinking faster as the shot approaches.
void DrawAimLine(const Fighter& f, int camX, int frame) {
    float sx = f.x - camX;
    float sy = f.z - 14;
    int blink = (f.stateT > AIM_FRAMES - 12) ? 3 : 7;
    if ((frame / blink) % 2) return;
    for (int i = 1; i < 26; i++) {
        float x = sx + f.facing * (10 + i * 5);
        if (x < -8 || x > GAME_W + 8) break;
        DrawRectangle((int)x, (int)sy, 3, 1, { 255, 60, 60, (unsigned char)(150 - i * 4) });
    }
}

void DrawParticle(const Particle& p, int camX) {
    float sx = p.x - camX;
    float sy = p.z - p.y;
    float t = (float)p.life / (float)p.maxLife;      // 1 -> 0

    if (p.kind == 2) {                                // rising damage / score text
        Color c = p.col; c.a = (unsigned char)(255 * fminf(1.0f, t * 2.0f));
        DrawText(p.text, (int)sx - 5, (int)sy + 1, 10, { 0, 0, 0, c.a });
        DrawText(p.text, (int)sx - 6, (int)sy, 10, c);
        return;
    }
    if (p.kind == 1) {                                // dust puff
        DrawCircle((int)sx, (int)sy, (1 - t) * 4 + 1,
                   { 200, 190, 170, (unsigned char)(t * 110) });
        return;
    }
    // impact spark: a burst that expands and cools from white to orange
    float r = (1 - t) * 9 + 2;
    Color c = (t > 0.5f) ? Color{ 255, 243, 196, 255 } : Color{ 244, 162, 58, 220 };
    for (int i = 0; i < 7; i++) {
        float a = i * 0.897f + (1 - t) * 3.0f;
        DrawRectangle((int)(sx + cosf(a) * r), (int)(sy + sinf(a) * r), 2, 2, c);
    }
}

// ============================================================
//  Painted background layers
//  far  = skyline, scrolls slowly     -> depth
//  near = the road, scrolls 1:1       -> or walking feels like ice
// ============================================================
static Texture2D g_bgFar{}, g_bgNear{};
static bool g_bgLoaded = false;

void LoadStageBackground(int idx) {
    UnloadStageBackground();
    const char* f = TextFormat("assets/bg%d_far.png", idx);
    const char* n = TextFormat("assets/bg%d_near.png", idx);
    if (!FileExists(f) || !FileExists(n)) {
        TraceLog(LOG_INFO, "no painted background for stage %d, using procedural", idx);
        return;
    }
    g_bgFar = LoadTexture(f);
    g_bgNear = LoadTexture(n);
    SetTextureFilter(g_bgFar, TEXTURE_FILTER_POINT);
    SetTextureFilter(g_bgNear, TEXTURE_FILTER_POINT);
    // repeat horizontally instead of clamping, so the edge pixel does not
    // smear into a long streak between tiles
    SetTextureWrap(g_bgFar, TEXTURE_WRAP_REPEAT);
    SetTextureWrap(g_bgNear, TEXTURE_WRAP_REPEAT);
    g_bgLoaded = true;
}

void UnloadStageBackground() {
    if (!g_bgLoaded) return;
    UnloadTexture(g_bgFar);
    UnloadTexture(g_bgNear);
    g_bgLoaded = false;
}

bool StageBackgroundLoaded() { return g_bgLoaded; }

// Tile a texture across the screen at a given scroll offset.
static void TileX(Texture2D t, float offset, int y, int h) {
    if (t.width <= 0) return;
    float o = fmodf(offset, (float)t.width);
    if (o > 0) o -= t.width;                 // fmodf keeps the sign, we want <= 0
    for (float x = -o - t.width; x < GAME_W; x += t.width) {
        DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height },
                          { x, (float)y, (float)t.width, (float)h },
                          { 0, 0 }, 0.0f, WHITE);
    }
}

// ---------- background ----------
static float Hash(int i) {
    float s = sinf(i * 12.9898f) * 43758.5453f;
    return fabsf(s - floorf(s));
}

void DrawBelt(int camX, int frame) {
    if (g_bgLoaded) {
        // Painted layers. The skyline at 0.30 and the road at 1.0 is what
        // makes the road read as the thing you are actually standing on.
        TileX(g_bgFar,  camX * 0.30f, 0, (int)BELT_TOP);
        TileX(g_bgNear, camX * 1.00f, (int)BELT_TOP, GAME_H - (int)BELT_TOP);
        return;
    }

    // sky above the belt
    DrawRectangleGradientV(0, 0, GAME_W, (int)BELT_TOP, { 26, 31, 61, 255 }, { 197, 107, 74, 255 });

    // two parallax skylines: far layer barely moves, near layer has lit windows
    for (int layer = 0; layer < 2; layer++) {
        float par = layer == 0 ? 0.10f : 0.30f;
        float ox = camX * par;
        int base = (int)floorf(ox / 28.0f);
        Color col = layer == 0 ? Color{ 0, 0, 0, 76 } : Color{ 0, 0, 0, 127 };
        for (int i = -1; i < GAME_W / 28 + 2; i++) {
            int idx = base + i;
            float r = Hash(idx);
            float bh = (layer == 0 ? 38 : 28) + r * (layer == 0 ? 32 : 24);
            float bw = 20 + r * 14;
            float bx = idx * 28 - ox;
            DrawRectangle((int)bx, (int)(BELT_TOP - bh), (int)bw, (int)bh, col);
            if (layer == 1) {
                for (float wy = 4; wy < bh - 4; wy += 6)
                    for (float wx = 3; wx < bw - 3; wx += 5)
                        if (Hash(idx * 31 + (int)wx * 7 + (int)wy * 3) < 0.35f)
                            DrawRectangle((int)(bx + wx), (int)(BELT_TOP - bh + wy), 2, 3,
                                          { 255, 214, 120, 140 });
            }
        }
    }

    // kerb, then the road itself
    DrawRectangle(0, (int)BELT_TOP - 5, GAME_W, 5, { 74, 68, 80, 255 });
    DrawRectangle(0, (int)BELT_TOP - 1, GAME_W, 2, { 104, 96, 110, 255 });
    DrawRectangle(0, (int)BELT_TOP + 1, GAME_W, GAME_H - (int)BELT_TOP, { 42, 37, 48, 255 });

    // lane dashes spaced wider as they come toward the camera â€” cheap perspective
    for (int row = 0; row < 4; row++) {
        float z = BELT_TOP + 8 + row * 14;
        float spacing = 26 + row * 8;
        float off = -fmodf(camX * (0.75f + row * 0.08f), spacing);
        for (int i = -1; i < GAME_W / spacing + 2; i++)
            DrawRectangle((int)(off + i * spacing), (int)z, 9 + row * 2, 1, { 255, 255, 255, 18 });
    }
    DrawRectangle(0, (int)BELT_BOT + 6, GAME_W, 3, { 30, 26, 34, 255 });
    (void)frame;
}


