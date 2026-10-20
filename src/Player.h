// Player.h - the player's ship.
//
// This is our first C++ class. A class bundles DATA (the ship's position and
// speed) together with the FUNCTIONS that work on that data (Update, Draw).

#pragma once

#include "Input.h"
#include "raylib.h"

class Player {
public:
    Player();   // the "constructor": runs once when a Player is created

    // Moves the ship. dt = seconds since the last frame.
    // "const PlayerInput&" means: give me the input without copying it (&),
    // and I promise not to change it (const).
    void Update(float dt, const PlayerInput& input);

    // Draws the ship. The "const" at the end promises that drawing does not
    // change the ship. The compiler enforces this promise.
    void Draw() const;

    // Tiny "getters": let other code read x_ and z_ without being able to change them.
    float X() const { return x_; }
    float Z() const { return z_; }   // 0 = the start line at the bottom, negative = further up

    // How far up the screen the ship may fly (a negative Z). Game makes it smaller
    // during a boss fight. If the ship is beyond a new limit, it glides back.
    void SetForwardLimit(float minZ) { minZ_ = minZ; }
    float ForwardLimit() const { return minZ_; }

    // The centre of the ship in world space, used for collision tests.
    Vector3 Position() const;

    // Upgrades the ship's look: 0 = basic, 1 = second tier, 2 = third tier.
    // Higher tiers are bigger and have more parts (see Player.cpp).
    void SetTier(int tier);
    int  Tier() const { return tier_; }

    // Switches between the first ship design and the copper "Claw" (from level 5).
    void SetHero(bool hero) { hero_ = hero; }
    bool Hero() const { return hero_; }

    // Half the ship's width: it depends on the design and grows with the tier.
    // Used for the wall limit.
    float HalfWidth() const;

    // The radius of the ship's collision sphere: what can hit it. Bigger for the Claw,
    // so a bomb cannot fly through its claws without doing harm.
    float HitRadius() const;

private:
    // "private" = only Player's own functions can touch these. Everyone else
    // must go through the public functions, so the ship can never end up in an
    // invalid state. A trailing underscore marks member variables.
    float x_         = 0.0f;   // position along X (left/right)
    float z_         = 0.0f;   // position along Z (0 = bottom line, negative = up the screen)
    float velocityX_ = 0.0f;   // current sideways speed in units per second
    float velocityZ_ = 0.0f;   // current speed up / down the screen (negative = up)
    float minZ_      = -10.0f; // forward limit, see SetForwardLimit (Player() sets it from Config.h)
    float time_      = 0.0f;   // seconds since creation, drives the engine flicker
    int   tier_      = 0;      // upgrade level, 0..kTierCount-1
    bool  hero_      = false;  // true = draw the Claw instead of the first ship

    // Draws the Claw around the origin. Draw() has already moved, rolled and
    // scaled the coordinate system, so this only builds the shape.
    void DrawHero() const;
};
