// InvaderGrid.h - the whole wave of invaders.
//
// It owns all invaders in a std::vector (a resizable array) and moves them as
// one group: sideways until the wave touches the field edge, then it reverses
// and steps toward the player.

#pragma once

#include <optional>
#include <vector>

#include "AlienModels.h"
#include "Invader.h"
#include "raylib.h"

// What happened when a rocket hit an alien.
struct InvaderHit {
    Vector3   position;    // where the alien is (or was)
    AlienType type;
    bool      destroyed;   // false = it took damage but is still alive
};

class InvaderGrid {
public:
    InvaderGrid();

    void Respawn();            // creates a fresh wave of random size
    void Update(float dt);
    void Draw(const AlienModels& models) const;

    // Is there an invader touching the sphere (centre, radius)?
    // If yes: that invader loses one hit point (and is removed when it has none
    // left), and we report what happened. If not: an empty optional.
    std::optional<InvaderHit> TryHit(Vector3 center, float radius);

    bool Empty() const { return invaders_.empty(); }

    // Position of a randomly chosen invader that should fire next.
    // std::optional<Vector3> means "a Vector3, or nothing at all". It is the safe
    // way to say "there might be no answer": here, when no invaders are left.
    std::optional<Vector3> PickShooterPosition() const;

    // Z of the invader closest to the player (the largest Z). Only call this
    // when the wave is not Empty().
    float FrontZ() const;

private:
    std::vector<Invader> invaders_;
    float direction_ = 1.0f;   // +1 = moving right, -1 = moving left
    float time_      = 0.0f;   // drives the bobbing animation
};
