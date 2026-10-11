// Starfield.cpp

#include "Starfield.h"

#include <algorithm>   // std::min

#include "Config.h"
#include "Random.h"

Starfield::Star Starfield::MakeStar(float z) {
    Star star;

    // Stars fill the whole space: above the floor AND below it. Only a thin
    // layer around the floor is left empty, so no star streaks through the
    // glass or the ship. Pick a height; if it lands in the gap, pick again.
    float y = 0.0f;
    do {
        y = RandomFloat(config::kStarMinY, config::kStarMaxY);
    } while (y > config::kStarGapMinY && y < config::kStarGapMaxY);

    star.position = Vector3{
        RandomFloat(-config::kStarHalfWidth, config::kStarHalfWidth),
        y,
        z};
    star.speedFactor = RandomFloat(0.5f, 1.5f);
    star.brightness  = RandomFloat(0.3f, 1.0f);
    return star;
}

Starfield::Starfield() {
    // Spread the first stars over the whole depth range, so the sky is already
    // full at the start and not only filling up from the far end.
    stars_.reserve(static_cast<std::size_t>(config::kStarCount));
    for (int i = 0; i < config::kStarCount; ++i) {
        stars_.push_back(MakeStar(RandomFloat(config::kStarFarZ, config::kStarNearZ)));
    }
}

void Starfield::Update(float dt) {
    for (auto& star : stars_) {
        star.position.z += config::kStarSpeed * star.speedFactor * dt;

        // Passed the camera? Send it back to the far end as a brand-new star.
        // The same few hundred stars are reused forever: no allocations.
        if (star.position.z > config::kStarNearZ) {
            star = MakeStar(config::kStarFarZ);
        }
    }
}

void Starfield::Draw() const {
    const Color base{200, 220, 255, 255};   // a slightly blue white

    for (const auto& star : stars_) {
        // Fade in over the first stretch, so stars do not "pop" into view at the far end.
        const float fadeIn = std::min(1.0f, (star.position.z - config::kStarFarZ) / 25.0f);

        const float length = config::kStarStreak * star.speedFactor;
        const Vector3 tail{star.position.x, star.position.y, star.position.z - length};

        DrawLine3D(star.position, tail, Fade(base, star.brightness * fadeIn));
    }
}
