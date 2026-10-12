// Display.cpp

#include "Display.h"

#include <cmath>   // std::lround

#include "Config.h"
#include "raylib.h"

namespace display {

namespace {

#if !defined(__EMSCRIPTEN__)
// Puts the window in the middle of the monitor it is on.
void CenterWindow() {
    const int monitor = GetCurrentMonitor();
    const Vector2 origin = GetMonitorPosition(monitor);   // top-left corner of that monitor
    SetWindowPosition(
        static_cast<int>(origin.x) + (GetMonitorWidth(monitor)  - GetScreenWidth())  / 2,
        static_cast<int>(origin.y) + (GetMonitorHeight(monitor) - GetScreenHeight()) / 2);
}

// Does a window of this size fit on the monitor, leaving room for the title bar
// and taskbar? "share" is the fraction of the monitor we allow it to use.
bool FitsOnMonitor(int width, int height, float share) {
    const int monitor = GetCurrentMonitor();
    const int monitorW = GetMonitorWidth(monitor);
    const int monitorH = GetMonitorHeight(monitor);
    if (monitorW <= 0 || monitorH <= 0) {
        return true;   // the system would not tell us: trust the player
    }
    return static_cast<float>(width)  <= static_cast<float>(monitorW) * share &&
           static_cast<float>(height) <= static_cast<float>(monitorH) * share;
}
#endif

}  // namespace

void OpenWindow(const char* title) {
#if defined(__EMSCRIPTEN__)
    // MSAA = smooths jagged edges. Flags must be set BEFORE the window exists.
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(config::kScreenWidth, config::kScreenHeight, title);
#else
    // Start hidden, so the player never sees the window jump from its first
    // size to the final one. RESIZABLE lets you also drag the window edges.
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIDDEN);
    InitWindow(config::kScreenWidth, config::kScreenHeight, title);

    // The biggest preset that uses at most kWindowStartShare of the monitor.
    // (The presets go from small to big, so the last one that fits wins.)
    int chosen = 0;
    for (int i = 0; i < config::kWindowPresetCount; ++i) {
        if (FitsOnMonitor(config::kWindowPresets[i][0], config::kWindowPresets[i][1],
                          config::kWindowStartShare)) {
            chosen = i;
        }
    }
    SetWindowSize(config::kWindowPresets[chosen][0], config::kWindowPresets[chosen][1]);
    CenterWindow();
    ClearWindowState(FLAG_WINDOW_HIDDEN);   // now show it
#endif
}

void ApplyPreset(int index) {
#if defined(__EMSCRIPTEN__)
    (void)index;   // the browser decides the size
#else
    if (index < 0 || index >= config::kWindowPresetCount) {
        return;
    }
    const int width  = config::kWindowPresets[index][0];
    const int height = config::kWindowPresets[index][1];
    if (!FitsOnMonitor(width, height, config::kWindowMaxShare)) {
        return;   // too big for this monitor: ignore, nothing changes
    }

    // Leave fullscreen or "maximized" first, otherwise the new size is ignored.
    if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) {
        ToggleBorderlessWindowed();
    }
    if (IsWindowState(FLAG_WINDOW_MAXIMIZED)) {
        ClearWindowState(FLAG_WINDOW_MAXIMIZED);
    }
    SetWindowSize(width, height);
    CenterWindow();
#endif
}

void ToggleFullscreen() {
#if !defined(__EMSCRIPTEN__)
    // "Borderless windowed" = a window without a frame that covers the whole
    // monitor. It switches instantly and does not change the screen resolution.
    ToggleBorderlessWindowed();
#endif
}

float UiScale() {
    return static_cast<float>(GetScreenHeight()) / config::kUiReferenceHeight;
}

int Px(int pixelsAt720p) {
    return static_cast<int>(std::lround(static_cast<float>(pixelsAt720p) * UiScale()));
}

}  // namespace display
