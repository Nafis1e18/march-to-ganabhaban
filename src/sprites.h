// ============================================================
//  Sprite sheets — your drawings
//
//  Geometry must match tools/make_sprite_template.py exactly.
//  If you change the template, change these five numbers.
// ============================================================
#pragma once
#include "game.h"

constexpr int SHEET_FW    = 64;   // one frame
constexpr int SHEET_FH    = 64;
constexpr int SHEET_COLS  = 8;    // frames per row
constexpr int SHEET_BASE  = 60;   // feet baseline inside a cell (red line)
constexpr int SHEET_MID   = 26;   // body centre inside a cell (blue line)
constexpr int SHEET_CHARH = 52;   // drawn character height, head to feet

// Load assets/<name>.png for a palette slot. Safe to call when the file does
// not exist — the placeholder skeleton simply keeps being used.
void LoadCharacterSheet(int palette, const char* path);

// Re-read every sheet from disk. Bound to R so you can redraw a frame,
// save, and see it in the running game without a rebuild.
void ReloadCharacterSheets();

void UnloadCharacterSheets();

// Draw from your art. Returns false when this character (or this specific
// animation) has not been painted yet, so the caller falls back to the
// skeleton. That is what lets you ship a half-finished sheet.
bool DrawFighterSprite(const Fighter& f, float sx, float sy);

// For the debug overlay: how much of a sheet is done.
int  SheetPaintedCount(int palette);
bool SheetLoaded(int palette);
