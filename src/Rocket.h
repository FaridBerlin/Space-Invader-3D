// Rocket.h - a rocket fired by the player.
//
// A struct is the same thing as a class, except everything is public by default.
// By habit: use "struct" for small bundles of data with a few helper functions
// (like this one), and "class" when an object guards its data (like Player).

#pragma once

#include "raylib.h"

struct Rocket {
    Vector3 position;
    Color   color;          // the rocket's colour; depends on the ship (see Game::FireRocket)
    float   sideSpeed = 0.0f;   // units per second to the side (+ = right): angled shots of the higher weapon levels
    bool    alive = true;   // set to false when it hits something or leaves the field

    // The constructor. ": position(start)" is an "initializer list": it sets the
    // members directly instead of creating them empty first and assigning later.
    // angleDegrees: 0 = straight ahead (-Z), negative = to the left, positive = to the right.
    Rocket(Vector3 start, Color rocketColor, float angleDegrees = 0.0f);

    void Update(float dt);
    void Draw() const;
};
