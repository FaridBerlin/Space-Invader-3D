// Rocket.cpp

#include "Rocket.h"

#include <cmath>   // std::sin, std::cos, std::fabs

#include "Config.h"

Rocket::Rocket(Vector3 start, Color rocketColor, float angleDegrees)
    : position(start), color(rocketColor),
      // The speed is split into a part to the side (sin) and a part forward (cos).
      sideSpeed(config::kRocketSpeed * std::sin(angleDegrees * DEG2RAD)) {}

void Rocket::Update(float dt) {
    // Distance = speed * time. Flying "forward" means toward -Z. An angled shot
    // also moves sideways; together the two parts are kRocketSpeed.
    const float forward = std::sqrt(config::kRocketSpeed * config::kRocketSpeed - sideSpeed * sideSpeed);
    position.z -= forward * dt;
    position.x += sideSpeed * dt;

    // Past the far end of the field, or far out to the side: mark it for removal.
    // We do not delete it here. Game removes all dead rockets in one go,
    // after the loop that updates them.
    if (position.z < -config::kFieldLength || std::fabs(position.x) > config::kFieldHalfWidth + 4.0f) {
        alive = false;
    }
}

void Rocket::Draw() const {
    // The rocket is a cone pointing forward (-Z) in its colour, with a small
    // lighter trail behind it.
    // The direction it flies in, as a vector of length 1 (straight ahead: 0, 0, -1).
    const float sideShare = sideSpeed / config::kRocketSpeed;
    const Vector3 dir{sideShare, 0.0f, -std::sqrt(1.0f - sideShare * sideShare)};
    auto along = [&](float distance) {   // a point this far ahead of the centre (negative: behind)
        return Vector3{position.x + dir.x * distance, position.y, position.z + dir.z * distance};
    };
    const Vector3 tip = along(0.45f);
    const Vector3 tail = along(-0.45f);
    const Vector3 flameEnd = along(-1.0f);

    // Mix the colour half-way with white for the trail: ColorLerp-style, by hand.
    const Color light{static_cast<unsigned char>((color.r + 255) / 2),
                      static_cast<unsigned char>((color.g + 255) / 2),
                      static_cast<unsigned char>((color.b + 255) / 2), 255};

    // A cylinder whose start radius is 0 is a cone.
    DrawCylinderEx(tip, tail, 0.0f, 0.16f, 8, color);
    DrawCylinderEx(tail, flameEnd, 0.12f, 0.0f, 8, light);
}
