// Input.h - the ONE place that knows which keys do what.
//
// The rest of the game never asks "is the A key down?". It asks "how much does
// the player want to move?". That way, when we add touch controls for Android
// or a gamepad later, only Input.cpp changes and the game code stays the same.

#pragma once

#include <optional>

#include "raylib.h"

// A struct is a simple bundle of named values. This one describes what the
// player wants to do during this frame.
struct PlayerInput {
    float move = 0.0f;    // -1 = full left, 0 = nothing, +1 = full right

    // Touch / mouse steering: when set, the ship flies to this X position (in
    // world units) instead of using "move". Game fills it in from the finger.
    std::optional<float> targetX;
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

    // True ONLY in the frame F3 was pressed: show or hide the FPS counter.
    bool  toggleFps = false;

    // Which window size the player chose with the keys 1 .. 5 (counted from 0),
    // or -1 when no such key was pressed in this frame.
    int   windowPreset = -1;

    // The "pointer" is a finger on a touch screen, or the mouse with its left
    // button on a computer. Positions are in screen pixels.
    bool    pointerDown    = false;   // a finger touches (or the button is held)
    bool    pointerPressed = false;   // ... and it started in THIS frame (a tap)
    Vector2 pointer{0.0f, 0.0f};
};

// True on phones and tablets (a browser with a touch screen). Used to choose the
// on-screen hints ("Tap to start" instead of "Press ENTER").
bool IsTouchDevice();

// True once after the game was in the background: the browser tab was hidden
// (a phone call, another app, another tab) or the desktop window lost focus.
// The game pauses itself then.
bool WentToBackground();

// Reads the keyboard and returns what the player wants right now.
PlayerInput ReadInput();
