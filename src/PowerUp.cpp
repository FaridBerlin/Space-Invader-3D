// PowerUp.cpp

#include "PowerUp.h"

#include <cmath>              // std::sin, std::cos, std::exp
#include <initializer_list>   // for loops over a list like {0.45f, 0.62f}

#include "Config.h"
#include "rlgl.h"

void PowerUp::Update(float dt) {
    age += dt;
    position.z += config::kPowerUpSpeed * dt;   // toward the player (+Z)

    // Sink smoothly from where the alien was to the ship's height, so the player
    // can fly into it. (Same exponential smoothing as the ship's movement.)
    position.y += (config::kPlayerHeight - position.y) * (1.0f - std::exp(-3.0f * dt));

    if (position.z > config::kPowerUpDespawnZ) {
        alive = false;   // flew past: missed it
    }
}

Color PowerUp::MainColor() const {
    switch (type) {
        case PowerUpType::Shield:    return Color{ 80, 220, 255, 255};   // cyan
        case PowerUpType::RapidFire: return Color{255, 150,  40, 255};   // orange
        case PowerUpType::ExtraLife: return Color{255,  90, 150, 255};   // pink
    }
    return Color{255, 255, 255, 255};
}

PowerUpType PowerUp::RandomType() {
    // A number from 1 to 100: the first kExtraLifeChance numbers mean "extra life",
    // the rest is split evenly between shield and rapid fire.
    const int roll = GetRandomValue(1, 100);
    if (roll <= config::kExtraLifeChance) return PowerUpType::ExtraLife;
    return (roll % 2 == 0) ? PowerUpType::Shield : PowerUpType::RapidFire;
}

void PowerUp::Draw() const {
    const Color main = MainColor();
    const Color white{255, 255, 255, 255};
    const float bob = 0.15f * std::sin(age * 5.0f);

    rlPushMatrix();
    rlTranslatef(position.x, position.y + bob, position.z);
    rlRotatef(age * 120.0f, 0.0f, 1.0f, 0.0f);   // spin around the vertical axis

    // The capsule: a pill shape lying sideways, with a white ring around its middle.
    DrawCapsule(Vector3{-0.38f, 0.0f, 0.0f}, Vector3{0.38f, 0.0f, 0.0f}, 0.26f, 8, 4, main);
    DrawCylinderEx(Vector3{-0.08f, 0.0f, 0.0f}, Vector3{0.08f, 0.0f, 0.0f}, 0.30f, 0.30f, 12, white);

    // A small symbol orbiting it: a ring for the shield, two arrows for rapid fire.
    if (type == PowerUpType::Shield) {
        DrawCircle3D(Vector3{0.0f, 0.0f, 0.0f}, 0.62f, Vector3{1.0f, 0.0f, 0.0f}, 90.0f, main);
        DrawCircle3D(Vector3{0.0f, 0.0f, 0.0f}, 0.66f, Vector3{1.0f, 0.0f, 0.0f}, 90.0f, white);
    } else if (type == PowerUpType::ExtraLife) {
        // A "+" sign above the capsule.
        DrawLine3D(Vector3{-0.2f, 0.55f, 0.0f}, Vector3{0.2f, 0.55f, 0.0f}, white);
        DrawLine3D(Vector3{0.0f, 0.35f, 0.0f}, Vector3{0.0f, 0.75f, 0.0f}, white);
    } else {
        for (const float y : {0.45f, 0.62f}) {   // two stacked arrowheads ("^ ^")
            DrawLine3D(Vector3{-0.22f, y - 0.15f, 0.0f}, Vector3{0.0f, y, 0.0f}, white);
            DrawLine3D(Vector3{0.0f, y, 0.0f}, Vector3{0.22f, y - 0.15f, 0.0f}, white);
        }
    }
    rlPopMatrix();
}
