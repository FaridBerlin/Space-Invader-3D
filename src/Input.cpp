// Input.cpp - implementation of what Input.h promises.
//
// The split: the .h file says WHAT exists (a "declaration"), the .cpp file says
// HOW it works (the "definition"). Other files only include the .h.

#include "Input.h"

#include "raylib.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>

// A little JavaScript: does this browser have a touch screen?
EM_JS(int, BrowserHasTouch, (), {
    return (navigator.maxTouchPoints > 0 || 'ontouchstart' in window) ? 1 : 0;
});

// Every touch or click on the game area, however short, is noted by a JavaScript
// listener (a very quick tap can begin AND end between two frames, so the game
// would never see the finger "down"). The game asks once per frame: was there a
// press since last time, and where? The position comes back as 0..1 of the
// canvas size, because the page may show the canvas bigger or smaller than the
// game's own pixel size.
EM_JS(int, BrowserTakePress, (float* x, float* y), {
    if (!Module.pressWatch) {
        Module.pressWatch = true;
        Module.pressPending = false;
        var canvas = document.getElementById('canvas');
        var note = function (clientX, clientY) {
            var r = canvas.getBoundingClientRect();
            Module.pressX = (clientX - r.left) / r.width;
            Module.pressY = (clientY - r.top) / r.height;
            Module.pressPending = true;
        };
        canvas.addEventListener('touchstart', function (e) {
            if (e.changedTouches.length > 0) note(e.changedTouches[0].clientX, e.changedTouches[0].clientY);
        }, { passive: true });
        canvas.addEventListener('mousedown', function (e) { note(e.clientX, e.clientY); });
    }
    if (!Module.pressPending) return 0;
    Module.pressPending = false;
    HEAPF32[x >> 2] = Module.pressX;   // write into the C++ variables x and y
    HEAPF32[y >> 2] = Module.pressY;
    return 1;
});

// While the tab is hidden the browser stops running the game, so we cannot
// notice it ourselves. Instead a JavaScript listener raises a flag when the
// page is hidden, and the game reads (and lowers) the flag when it runs again.
EM_JS(int, BrowserWasHidden, (), {
    if (!Module.hiddenWatch) {
        Module.hiddenWatch = true;
        Module.wasHidden = false;
        document.addEventListener('visibilitychange', function () {
            if (document.hidden) Module.wasHidden = true;
        });
    }
    var hidden = Module.wasHidden;
    Module.wasHidden = false;
    return hidden ? 1 : 0;
});
#endif

bool IsTouchDevice() {
#if defined(__EMSCRIPTEN__)
    return BrowserHasTouch() != 0;
#else
    return false;
#endif
}

namespace {
bool pointerWasDown = false;   // the pointer state in the previous frame
#if !defined(__EMSCRIPTEN__)
bool windowWasFocused = false; // desktop: was the window focused in the previous frame?
#endif
}  // namespace

bool WentToBackground() {
#if defined(__EMSCRIPTEN__)
    return BrowserWasHidden() != 0;
#else
    // Only the CHANGE from focused to not focused counts, so a window that never
    // had the focus does not pause the game again and again.
    const bool focused = IsWindowFocused() && !IsWindowMinimized();
    const bool lostFocus = windowWasFocused && !focused;
    windowWasFocused = focused;
    return lostFocus;
#endif
}

PlayerInput ReadInput() {
    PlayerInput input;   // starts with move = 0 and fire = false

    // IsKeyDown is true for as long as the key is held.
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) input.move -= 1.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input.move += 1.0f;

    // If both directions are held, -1 and +1 cancel out to 0, which is exactly
    // what we want.

    input.fire = IsKeyDown(KEY_SPACE);

    // IsKeyPressed is true for exactly one frame per key press.
    input.confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    input.toggleMute = IsKeyPressed(KEY_M);
    input.toggleFullscreen = IsKeyPressed(KEY_F);
    input.togglePause = IsKeyPressed(KEY_P);
    input.back = IsKeyPressed(KEY_ESCAPE);
    input.toggleFps = IsKeyPressed(KEY_F3);
    input.openStore = IsKeyPressed(KEY_G);

    // The pointer: a finger (touch screen) or the left mouse button. A "tap" is
    // the frame in which it goes down: down now, but not in the previous frame.
    if (GetTouchPointCount() > 0) {
        input.pointerDown = true;
        input.pointer = GetTouchPosition(0);
    } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        input.pointerDown = true;
        input.pointer = GetMousePosition();
    }
    input.pointerPressed = input.pointerDown && !pointerWasDown;
    pointerWasDown = input.pointerDown;

#if defined(__EMSCRIPTEN__)
    // In the browser, also catch presses too short to be seen "down" (see above).
    float pressX = 0.0f;
    float pressY = 0.0f;
    if (BrowserTakePress(&pressX, &pressY)) {
        input.pointerPressed = true;
        input.pointer = Vector2{pressX * static_cast<float>(GetScreenWidth()),
                                pressY * static_cast<float>(GetScreenHeight())};
    }
#endif

    // Keys 1 .. 5 pick a window size. KEY_ONE .. KEY_FIVE are consecutive numbers,
    // so one loop covers them all.
    for (int i = 0; i < 5; ++i) {
        if (IsKeyPressed(KEY_ONE + i)) input.windowPreset = i;
    }
    return input;
}
