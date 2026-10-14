// InvaderGrid.h - the whole wave of invaders.
//
// It owns all invaders in a std::vector (a resizable array) and moves them as
// one group: sideways until the wave touches the field edge, then it reverses
// and steps toward the player. Every few seconds one alien leaves the group and
// dives at the player ("divers", see UpdateDivers).

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

    // Creates a fresh wave for this level (0-based): its size, speed and alien
    // types depend on the level (see "Levels" in Config.h).
    void Respawn(int level);
    // level = 0-based level (more divers later), playerX = where the ship is (divers aim there).
    void Update(float dt, int level, float playerX);
    void Draw(const AlienModels& models) const;

    // Is there an invader touching the sphere (centre, radius)?
    // If yes: that invader loses one hit point (and is removed when it has none
    // left), and we report what happened. If not: an empty optional.
    std::optional<InvaderHit> TryHit(Vector3 center, float radius);

    // Is a DIVING alien touching the sphere (the player's ship)? If yes, that
    // alien is destroyed (it crashed into the ship) and we report it.
    std::optional<InvaderHit> TryRam(Vector3 center, float radius);

    bool Empty() const { return invaders_.empty(); }

    // Position of a randomly chosen invader that should fire next.
    // std::optional<Vector3> means "a Vector3, or nothing at all". It is the safe
    // way to say "there might be no answer": here, when no invaders are left.
    std::optional<Vector3> PickShooterPosition() const;

    // Z of the invader in formation closest to the player (the largest Z).
    // Divers do not count. Without any alien in formation, a very small number.
    float FrontZ() const;

private:
    void MoveFormation(float dt);
    void UpdateDivers(float dt, int level, float playerX);

    std::vector<Invader> invaders_;
    float   direction_ = 1.0f;   // +1 = moving right, -1 = moving left
    float   time_      = 0.0f;   // drives the bobbing animation
    float   diveTimer_ = 0.0f;   // seconds until the next alien may dive
    float   speed_     = 5.0f;   // sideways speed of this wave, units per second
    int     level_     = 0;      // the level this wave belongs to (for its colours)
    Vector3 waveOffset_{};       // how far the whole wave has moved since it appeared
};
