// PowerUp.h - a capsule dropped by a destroyed alien. Fly into it to collect it.

#pragma once

#include "raylib.h"

enum class PowerUpType {
    Shield,      // a bubble around the ship that absorbs one hit
    RapidFire,   // shoot twice as fast for a few seconds
};

struct PowerUp {
    Vector3     position;
    PowerUpType type;
    bool        alive = true;
    float       age   = 0.0f;   // seconds since it was dropped, drives the spin

    PowerUp(Vector3 start, PowerUpType powerUpType) : position(start), type(powerUpType) {}

    void Update(float dt);   // drifts toward the player; dies when it flies past
    void Draw() const;

    Color MainColor() const;   // cyan for the shield, orange for rapid fire

    // A random type, used when a capsule drops.
    static PowerUpType RandomType();
};
