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

    // True ONLY during the single frame in which Enter was pressed. This is
    // different from "fire": holding Enter must not restart the game 60 times
    // per second. (Raylib: IsKeyDown = held, IsKeyPressed = just went down.)
    bool  confirm = false;

    // True ONLY in the frame the mute key (M) was pressed.
    bool  toggleMute = false;

    // True ONLY in the frame the pause key (P) was pressed.
    bool  togglePause = false;

    // True ONLY in the frame the fullscreen key (F) was pressed.
    bool  toggleFullscreen = false;

    // True ONLY in the frame Esc was pressed: leave fullscreen, or quit the game.
    bool  back = false;

    // Which window size the player chose with the keys 1 .. 5 (counted from 0),
    // or -1 when no such key was pressed in this frame.
    int   windowPreset = -1;
};

// Reads the keyboard and returns what the player wants right now.
PlayerInput ReadInput();
