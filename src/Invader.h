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
    Wasp,        // a striped insect with fast wings; dives more often
    Moth,        // a big soft insect with eye-spot wings; takes two hits
};

// The colours of the aliens in one level: every level paints the whole wave in
// its own colours, so you see at a glance that a new level has begun. The shapes
// stay the same, and each type keeps one small mark of its own (the Reaper's red
// pods, the Drone's white ring), so the types stay easy to tell apart.
struct LevelAlienColors {
    Color body;   // shells, armour and wings
    Color glow;   // glowing lines, stripes and the core
};
LevelAlienColors AlienColorsForLevel(int level);   // level is 0-based

// What an alien is doing right now. Divers leave the formation (see InvaderGrid).
enum class DiveState {
    Entering,      // flying in along a curve to its place, when the wave appears
    InFormation,   // moving with the wave
    Diving,        // swooping at the player in an S-curve
    Returning,     // missed: flying back from the far end into its place
};

struct Invader {
    Vector3   position;    // centre of the alien in world space
    AlienType type;
    int       hitPoints;   // how many rocket hits it can still take
    float     flash = 0.0f;   // > 0 right after a hit: the alien lights up briefly

    // Diving. Only InvaderGrid changes these.
    DiveState dive      = DiveState::InFormation;
    float     diveTime  = 0.0f;    // seconds since the dive (or the return) started
    Vector3   diveStart{};         // where the dive (or the return) started
    float     diveTargetX = 0.0f;  // the player's X when the dive started
    float     diveSide  = 1.0f;    // +1 or -1: which way the S-curve swings first
    Vector3   slot{};              // its place in the formation, relative to the wave
    float     roll      = 0.0f;    // tilt in degrees while diving or flying in

    // Flying in (Entering): it waits entryDelay seconds outside the screen at
    // diveStart, then follows a curve pulled toward entryPull1 and entryPull2 into
    // its place (a "cubic Bezier curve", see InvaderGrid.cpp).
    float     entryDelay = 0.0f;
    Vector3   entryPull1{};
    Vector3   entryPull2{};

    Invader(Vector3 start, AlienType alienType);

    // Counts down the hit flash.
    void Update(float dt);

    // "time" (seconds) drives the animations (bobbing, claws, glowing core).
    // commanderModel is the loaded .glb model, or nullptr to use the built-in look.
    // level (0-based) picks the colours, see AlienColorsForLevel.
    void Draw(float time, const Model* commanderModel, int level) const;

    // Size of the collision sphere; commanders are bigger.
    float Radius() const;
};
