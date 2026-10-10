// InvaderShot.h - a bolt fired BY an invader, flying toward the player.
//
// It looks a lot like Rocket on purpose: same idea, opposite direction. We keep
// them as two small separate types for now. (If we ever add many more
// projectile kinds, merging them into one "Projectile" type would make sense.)

#pragma once

#include "raylib.h"

struct InvaderShot {
    Vector3 position;
    bool    alive = true;

    explicit InvaderShot(Vector3 start) : position(start) {}

    void Update(float dt);
    void Draw() const;
};
