// Display.cpp

#include "Display.h"

#include <algorithm>   // std::min
#include <cmath>       // std::lround

#include "Config.h"
#include "Platform.h"
#include "raylib.h"

namespace display {

namespace {

#if defined(GAME_DESKTOP)
// Our own fullscreen state. We remember where the window was and how big it
// was, so leaving fullscreen puts it back exactly there.
bool fullscreen = false;
Vector2 savedPosition{0.0f, 0.0f};
int savedWidth  = 0;
int savedHeight = 0;

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
#if defined(GAME_WEB)
    // MSAA = smooths jagged edges. Flags must be set BEFORE the window exists.
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(config::kScreenWidth, config::kScreenHeight, title);
#elif defined(GAME_ANDROID)
    // On a phone the game always fills the whole screen; 0 x 0 means "the size
    // of the display". (Portrait only: see android/app/src/main/AndroidManifest.xml.)
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, title);
#else
    // Start hidden, so the player never sees the window jump from its first
    // size to the final one. RESIZABLE lets you also drag the window edges.
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIDDEN);
    InitWindow(config::kScreenWidth, config::kScreenHeight, title);

    // raylib closes the window when Esc is pressed. We switch that off and handle
    // Esc ourselves (see Game::Update): first it leaves fullscreen, then it quits.
    SetExitKey(KEY_NULL);

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
#if !defined(GAME_DESKTOP)
    (void)index;   // the browser or the phone decides the size
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
    if (fullscreen) {
        ExitFullscreen();
    }
    if (IsWindowState(FLAG_WINDOW_MAXIMIZED)) {
        RestoreWindow();
    }
    SetWindowSize(width, height);
    CenterWindow();
    SetWindowFocused();
    TraceLog(LOG_INFO, "DISPLAY: window size %d x %d", width, height);
#endif
}

// Our fullscreen is a normal window WITHOUT a frame ("undecorated") that covers
// the whole monitor. We do not use the operating system's real fullscreen mode:
// on some Linux desktops it can steal the keyboard focus or ignore later size
// changes, and then the game no longer reacts to keys.
void EnterFullscreen() {
#if defined(GAME_DESKTOP)
    if (fullscreen) return;
    savedPosition = GetWindowPosition();
    savedWidth    = GetScreenWidth();
    savedHeight   = GetScreenHeight();

    if (IsWindowState(FLAG_WINDOW_MAXIMIZED)) {
        RestoreWindow();
    }
    const int monitor = GetCurrentMonitor();
    const Vector2 origin = GetMonitorPosition(monitor);
    SetWindowState(FLAG_WINDOW_UNDECORATED);   // no title bar, no border
    SetWindowPosition(static_cast<int>(origin.x), static_cast<int>(origin.y));
    SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
    SetWindowFocused();                        // make sure the keys still come to us
    fullscreen = true;
    TraceLog(LOG_INFO, "DISPLAY: fullscreen on monitor %d (%d x %d), press F or Esc to leave",
             monitor, GetMonitorWidth(monitor), GetMonitorHeight(monitor));
#endif
}

void ExitFullscreen() {
#if defined(GAME_DESKTOP)
    if (!fullscreen) return;
    ClearWindowState(FLAG_WINDOW_UNDECORATED);   // the title bar comes back
    SetWindowSize(savedWidth, savedHeight);
    SetWindowPosition(static_cast<int>(savedPosition.x), static_cast<int>(savedPosition.y));
    SetWindowFocused();
    fullscreen = false;
    TraceLog(LOG_INFO, "DISPLAY: fullscreen off, back to %d x %d", savedWidth, savedHeight);
#endif
}

void ToggleFullscreen() {
    if (IsFullscreen()) {
        ExitFullscreen();
    } else {
        EnterFullscreen();
    }
}

bool IsFullscreen() {
#if !defined(GAME_DESKTOP)
    return false;
#else
    return fullscreen;
#endif
}

Rectangle Column() {
    const float screenW = static_cast<float>(GetScreenWidth());
    const float screenH = static_cast<float>(GetScreenHeight());
    const float width = std::min(screenW, screenH * config::kColumnMaxAspect);
    return Rectangle{(screenW - width) / 2.0f, 0.0f, width, screenH};
}

float UiScale() {
    // The smaller of the two ratios, so the text fits both ways: in a long phone
    // the width decides, in a wider column the height.
    const Rectangle column = Column();
    return std::min(column.width / config::kUiReferenceWidth, column.height / config::kUiReferenceHeight);
}

int Px(int designPixels) {
    return static_cast<int>(std::lround(static_cast<float>(designPixels) * UiScale()));
}

}  // namespace display
