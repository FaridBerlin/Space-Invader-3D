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
    float   sideSpeed;      // sideways speed (X); 0 = straight. The bosses fire fans of shots.
    bool    alive = true;

    // "= 0.0f" is a default argument: callers that do not pass a side speed get 0.
    InvaderShot(Vector3 start, float shotSpeed, float shotSideSpeed = 0.0f)
        : position(start), speed(shotSpeed), sideSpeed(shotSideSpeed) {}

    void Update(float dt);
    void Draw() const;
};
