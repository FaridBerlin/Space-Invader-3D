// Display.h - the window: its size, fullscreen, and how big the 2D text is.
//
// The 3D picture always fills the whole window, whatever its size. The 2D text
// (score, menus) is drawn in pixels, so it must be scaled with the window: on a
// 4K monitor, text that is 32 pixels high would be tiny. Px() does that scaling.
//
// The size presets and fullscreen only exist on desktop. In the browser the web
// page (web/shell.html) already scales the game to the browser window.

#pragma once

namespace display {

// Opens the game window. On desktop it is resizable, and it starts at the biggest
// size from Config.h (kWindowPresets) that comfortably fits your monitor.
void OpenWindow(const char* title);

// Switches to one of the sizes in Config.h (index 0 .. kWindowPresetCount-1).
// Sizes that do not fit on this monitor are ignored. Desktop only.
void ApplyPreset(int index);

// Borderless fullscreen on / off. Desktop only.
void ToggleFullscreen();

// How much bigger than the 720-pixel-high original the window is right now.
float UiScale();

// Converts a size in "720p pixels" into real pixels for the current window.
// Example: at 1440 pixels height, Px(32) is 64.
int Px(int pixelsAt720p);

}  // namespace display
