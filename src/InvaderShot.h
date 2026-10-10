// InvaderShot.h - a bolt fired BY an invader, flying toward the player.
//
// It looks a lot like Rocket on purpose: same idea, opposite direction. We keep
// them as two small separate types for now. (If we ever add many more
// projectile kinds, merging them into one "Projectile" type would make sense.)

#pragma once

#include "raylib.h"

struct InvaderShot {
    Vector3 position;
    float   speed;          // units per second; depends on the level, so each shot remembers its own
    bool    alive = true;

    InvaderShot(Vector3 start, float shotSpeed) : position(start), speed(shotSpeed) {}

    void Update(float dt);
    void Draw() const;
};
