// Rocket.h - a rocket fired by the player.
//
// A struct is the same thing as a class, except everything is public by default.
// By habit: use "struct" for small bundles of data with a few helper functions
// (like this one), and "class" when an object guards its data (like Player).

#pragma once

#include "raylib.h"

struct Rocket {
    Vector3 position;
    bool    alive = true;   // set to false when it hits something or leaves the field

    // The constructor. ": position(start)" is an "initializer list": it sets the
    // member directly instead of creating it empty first and assigning later.
    // "explicit" stops the compiler from silently turning a Vector3 into a Rocket.
    explicit Rocket(Vector3 start) : position(start) {}

    void Update(float dt);
    void Draw() const;
};
