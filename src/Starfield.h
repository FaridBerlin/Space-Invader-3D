// Starfield.h - stars that stream toward the camera, so it feels like flying.
//
// Each star is drawn as a short line along Z (a "streak"). Faster stars get
// longer streaks, like a long camera exposure. Stars have different speeds, so
// near ones seem to move faster than far ones (that is "parallax").

#pragma once

#include <vector>

#include "raylib.h"

class Starfield {
public:
    Starfield();

    void Update(float dt);
    void Draw() const;   // call between BeginMode3D and EndMode3D

    // The stars' colour (each level has its own mood). The change happens smoothly
    // over about two seconds, not in one jump.
    void SetColor(Color target) { target_ = target; }

private:
    struct Star {
        Vector3 position;
        float   speedFactor;   // 0.5 .. 1.5, multiplies the base speed
        float   brightness;    // 0.3 .. 1.0
    };

    // Creates a star at depth z with a random X, height, speed and brightness.
    static Star MakeStar(float z);

    std::vector<Star> stars_;
    Color color_{200, 220, 255, 255};    // what the stars look like right now
    Color target_{200, 220, 255, 255};   // what they are fading toward
};
