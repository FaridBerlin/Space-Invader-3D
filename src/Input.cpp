// Input.cpp - implementation of what Input.h promises.
//
// The split: the .h file says WHAT exists (a "declaration"), the .cpp file says
// HOW it works (the "definition"). Other files only include the .h.

#include "Input.h"

#include "raylib.h"

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

    // Keys 1 .. 5 pick a window size. KEY_ONE .. KEY_FIVE are consecutive numbers,
    // so one loop covers them all.
    for (int i = 0; i < 5; ++i) {
        if (IsKeyPressed(KEY_ONE + i)) input.windowPreset = i;
    }
    return input;
}
