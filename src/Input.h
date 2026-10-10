// Input.h - the ONE place that knows which keys do what.
//
// The rest of the game never asks "is the A key down?". It asks "how much does
// the player want to move?". That way, when we add touch controls for Android
// or a gamepad later, only Input.cpp changes and the game code stays the same.

#pragma once

// A struct is a simple bundle of named values. This one describes what the
// player wants to do during this frame.
struct PlayerInput {
    float move = 0.0f;    // -1 = full left, 0 = nothing, +1 = full right
    bool  fire = false;   // true while the fire button is held
};

// Reads the keyboard and returns what the player wants right now.
PlayerInput ReadInput();
