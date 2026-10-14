// Bomb.h - a big glowing mine that drops in from far away and flies at the player.
//
// Rules (from the 2D game): a bomb appears every few seconds. It is worth +500
// points if you shoot it, and it destroys your ship if it touches you. Higher
// levels make bombs faster, bigger and redder.
//
// The flight path: the bomb starts high up and far away. As it comes closer it
// sinks down to "cruise height", the height where your rockets fly, so you can
// shoot it before it reaches you. In 2D a bomb fell straight down the screen.
//
// Every level has its own bomb, scarier each time (see Draw in Bomb.cpp):
//   1 calm  2 pulsing  3 + energy ring  4 + spikes  5 + sparks  6 + flicker
//   7 a dark cracked core, lightning and a big throbbing glow.
// In 3D "down the screen" is "toward the camera", so the idea carries over.

#pragma once

#include "raylib.h"

struct Bomb {
    Vector3 position;
    float   speed;     // units per second toward the player (+Z); depends on the level
    float   radius;    // size of the sphere AND of its collision sphere
    Color   color;     // a different colour in every level
    int     level;     // 0-based level: decides which effects the bomb has
    bool    alive = true;

    Bomb(Vector3 start, float bombSpeed, float bombRadius, Color bombColor, int bombLevel)
        : position(start), speed(bombSpeed), radius(bombRadius), color(bombColor), level(bombLevel) {}

    void Update(float dt);
    void Draw(float time) const;   // time drives the spinning, pulsing and sparks

    // How big the glow around the bomb is right now (it throbs in later levels).
    float GlowSize(float time) const;
};
