// Invader.h - one alien.
//
// An Invader only knows where it is and how to draw itself. Moving the whole
// wave together is the job of InvaderGrid.

#pragma once

#include "raylib.h"

struct Invader {
    Vector3 position;   // centre of the alien in world space

    explicit Invader(Vector3 start) : position(start) {}

    // "time" (seconds) makes the alien bob up and down a little.
    void Draw(float time) const;
};
