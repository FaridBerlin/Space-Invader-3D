// Bomb.h - a big glowing mine that drops in from far away and flies at the player.
//
// Rules (from the 2D game): a bomb appears every few seconds. It is worth +500
// points if you shoot it, and it destroys your ship if it touches you. Higher
// levels make bombs faster, bigger and redder.
//
// The flight path: the bomb starts high up and far away. As it comes closer it
// sinks down to "cruise height", the height where your rockets fly, so you can
// shoot it before it reaches you. In 2D a bomb fell straight down the screen.
// In 3D "down the screen" is "toward the camera", so the idea carries over.

#pragma once

#include "raylib.h"

struct Bomb {
    Vector3 position;
    float   speed;     // units per second toward the player (+Z); depends on the level
    float   radius;    // size of the sphere AND of its collision sphere
    Color   color;     // yellow / orange / red by level
    bool    alive = true;

    Bomb(Vector3 start, float bombSpeed, float bombRadius, Color bombColor)
        : position(start), speed(bombSpeed), radius(bombRadius), color(bombColor) {}

    void Update(float dt);
    void Draw(float time) const;   // time makes the warning rings spin
};
