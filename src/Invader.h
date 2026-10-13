// Invader.h - one alien.
//
// An Invader knows where it is, what kind of alien it is, how much damage it can
// still take, and how to draw itself. Moving the whole wave together is the job
// of InvaderGrid.

#pragma once

#include "raylib.h"

// The kinds of aliens. "enum class" = a fixed list of named values.
enum class AlienType {
    Scarab,      // front rows: violet shell, lime circuit lines, two claws
    Reaper,      // middle rows: black and red, horn-wings, shoots more often
    Drone,       // back row: a small spinning cyan crystal with a ring of light
    Commander,   // 1-2 per wave in the back row: bigger, takes several hits
};

struct Invader {
    Vector3   position;    // centre of the alien in world space
    AlienType type;
    int       hitPoints;   // how many rocket hits it can still take
    float     flash = 0.0f;   // > 0 right after a hit: the alien lights up briefly

    Invader(Vector3 start, AlienType alienType);

    // Counts down the hit flash.
    void Update(float dt);

    // "time" (seconds) drives the animations (bobbing, claws, glowing core).
    // commanderModel is the loaded .glb model, or nullptr to use the built-in look.
    void Draw(float time, const Model* commanderModel) const;

    // Size of the collision sphere; commanders are bigger.
    float Radius() const;
};
