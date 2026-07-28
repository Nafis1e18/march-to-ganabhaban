// ============================================================
//  Audio — synthesised at start-up, no sound files on disk.
// ============================================================
#pragma once
#include "raylib.h"

enum SfxId {
    SFX_HIT, SFX_SWING, SFX_KILL, SFX_HURT, SFX_JUMP, SFX_LAND,
    SFX_BOSS, SFX_SCORE, SFX_CLEAR, SFX_GAMEOVER, SFX_SELECT,
    SFX_COUNT
};

void InitGameAudio();
void UnloadGameAudio();
void PlaySfx(int id);

// The loop is rebuilt per stage: a semitone higher and a few bpm faster each
// time, so difficulty is audible as well as visible.
void StartMusic(int stage);
void UpdateMusic();      // call once a frame; re-triggers the loop
void StopMusic();
