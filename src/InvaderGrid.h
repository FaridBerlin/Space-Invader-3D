// InvaderGrid.h - the whole wave of invaders.
//
// It owns all invaders in a std::vector (a resizable array) and moves them as
// one group: sideways until the wave touches the field edge, then it reverses
// and steps toward the player.

#pragma once

#include <vector>

#include "Invader.h"
#include "raylib.h"

class InvaderGrid {
public:
    InvaderGrid();

    void Respawn();            // creates a fresh wave of random size
    void Update(float dt);
    void Draw() const;

    // Is there an invader touching the sphere (centre, radius)?
    // If yes: that invader is destroyed and the function returns true.
    bool TryHit(Vector3 center, float radius);

    bool Empty() const { return invaders_.empty(); }

private:
    std::vector<Invader> invaders_;
    float direction_ = 1.0f;   // +1 = moving right, -1 = moving left
    float time_      = 0.0f;   // drives the bobbing animation
};
