// Input.cpp - implementation of what Input.h promises.
//
// The split: the .h file says WHAT exists (a "declaration"), the .cpp file says
// HOW it works (the "definition"). Other files only include the .h.

#include "Input.h"

#include "raylib.h"

PlayerInput ReadInput() {
    PlayerInput input;   // starts with move = 0

    // IsKeyDown is true for as long as the key is held.
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) input.move -= 1.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input.move += 1.0f;

    // If both directions are held, -1 and +1 cancel out to 0, which is exactly
    // what we want.
    return input;
}
