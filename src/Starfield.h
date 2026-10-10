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

private:
    struct Star {
        Vector3 position;
        float   speedFactor;   // 0.5 .. 1.5, multiplies the base speed
        float   brightness;    // 0.3 .. 1.0
    };

    // Creates a star at depth z with a random X, height, speed and brightness.
    static Star MakeStar(float z);

    std::vector<Star> stars_;
};
