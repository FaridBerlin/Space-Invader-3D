// Display.h - the window: its size, fullscreen, and how big the 2D text is.
//
// The 3D picture always fills the whole window, whatever its size. The 2D text
// (score, menus) is drawn in pixels, so it must be scaled with the window: on a
// big phone or monitor, text that is 32 pixels high would be tiny. Px() does that.
//
// The size presets and fullscreen only exist on desktop. In the browser the web
// page (web/shell.html) already scales the game to the browser window.

#pragma once

#include "raylib.h"   // Rectangle

namespace display {

// Opens the game window. On desktop it is resizable, and it starts at the biggest
// size from Config.h (kWindowPresets) that comfortably fits your monitor.
void OpenWindow(const char* title);

// Switches to one of the sizes in Config.h (index 0 .. kWindowPresetCount-1).
// Sizes that do not fit on this monitor are ignored. Desktop only.
void ApplyPreset(int index);

// Fullscreen on / off (a frameless window covering the monitor). Desktop only.
void ToggleFullscreen();
void EnterFullscreen();
void ExitFullscreen();
bool IsFullscreen();

// The "play column": the part of the window the game uses (see kColumnMaxAspect
// in Config.h). On a phone and in a normal tall window it is the whole window; on
// a wide screen it is a tall strip in the middle. The HUD is laid out inside it.
Rectangle Column();

// How much bigger than the 540 x 960 design size the column is right now.
float UiScale();

// Converts a size in "design pixels" (a 540 x 960 column) into real pixels for
// the current window. Example: in a 1080 x 1920 column, Px(32) is 64.
int Px(int designPixels);

}  // namespace display
