// Rocket.cpp

#include "Rocket.h"

#include "Config.h"

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
    // The rocket is a red cone pointing forward (-Z), with a small flame behind it.
    const Vector3 tip {position.x, position.y, position.z - 0.45f};
    const Vector3 tail{position.x, position.y, position.z + 0.45f};
    const Vector3 flameEnd{position.x, position.y, position.z + 1.0f};

    // A cylinder whose start radius is 0 is a cone.
    DrawCylinderEx(tip, tail, 0.0f, 0.16f, 8, RED);
    DrawCylinderEx(tail, flameEnd, 0.12f, 0.0f, 8, ORANGE);
}
