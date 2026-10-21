// PowerUp.h - a glowing orb dropped by a destroyed alien. Fly into it to collect it.

#pragma once

#include "raylib.h"

enum class PowerUpType {
    Weapon,      // the weapon level goes up by one (more shots)
    Spread,      // switch to the spread gun (or, if you have it, its level +1)
    Laser,       // switch to the lightning laser (or its level +1)
    Shield,      // a bubble around the ship that absorbs one hit
    RapidFire,   // shoot twice as fast for a few seconds
    ExtraLife,   // one more ship (rarer than the others)
};

struct PowerUp {
    Vector3     position;
    PowerUpType type;
    bool        alive = true;
    float       age   = 0.0f;   // seconds since it was dropped, drives the spin

    PowerUp(Vector3 start, PowerUpType powerUpType) : position(start), type(powerUpType) {}

    void Update(float dt);   // drifts toward the player; dies when it flies past
    void Draw() const;       // the 3D part: a see-through ball (between BeginMode3D and EndMode3D)
    // The 2D part, drawn over the 3D picture: the bright rim and the symbol inside
    // (arrows, fan, lightning, "x2", shield, "+1"). 2D keeps the symbol sharp and always
    // facing the player, at any camera angle.
    void DrawIcon(const Camera3D& camera) const;

    // purple: weapon, green: spread, blue: laser, yellow: rapid fire, cyan: shield, pink: extra life
    Color MainColor() const;

    // A random type, used when an orb drops (the chances are in Config.h).
    static PowerUpType RandomType();

private:
    float Bob() const;   // the gentle up-and-down movement (height offset)
};
