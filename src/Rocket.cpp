// Rocket.cpp

#include "Rocket.h"

#include "Config.h"

Rocket::Rocket(Vector3 start, Color rocketColor)
    : position(start), color(rocketColor) {}

void Rocket::Update(float dt) {
    // Distance = speed * time. Flying "forward" means toward -Z.
    position.z -= config::kRocketSpeed * dt;

    // Past the far end of the field: mark it for removal.
    // We do not delete it here. Game removes all dead rockets in one go,
    // after the loop that updates them.
    if (position.z < -config::kFieldLength) {
        alive = false;
    }
}

void Rocket::Draw() const {
    // The rocket is a cone pointing forward (-Z) in its colour, with a small
    // lighter trail behind it.
    const Vector3 tip {position.x, position.y, position.z - 0.45f};
    const Vector3 tail{position.x, position.y, position.z + 0.45f};
    const Vector3 flameEnd{position.x, position.y, position.z + 1.0f};

    // Mix the colour half-way with white for the trail: ColorLerp-style, by hand.
    const Color light{static_cast<unsigned char>((color.r + 255) / 2),
                      static_cast<unsigned char>((color.g + 255) / 2),
                      static_cast<unsigned char>((color.b + 255) / 2), 255};

    // A cylinder whose start radius is 0 is a cone.
    DrawCylinderEx(tip, tail, 0.0f, 0.16f, 8, color);
    DrawCylinderEx(tail, flameEnd, 0.12f, 0.0f, 8, light);
}
