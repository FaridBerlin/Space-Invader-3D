// InvaderShot.cpp

#include "InvaderShot.h"

#include <cmath>   // std::fabs, std::sqrt

#include "Config.h"

void InvaderShot::Update(float dt) {
    // Toward the player = toward +Z (a rocket flies toward -Z).
    position.z += speed * dt;
    position.x += sideSpeed * dt;

    // Flew past the ship (or far out to the side) without hitting it: mark for removal.
    if (position.z > config::kInvaderShotDespawnZ ||
        std::fabs(position.x) > config::kFieldHalfWidth + 6.0f) {
        alive = false;
    }
}

void InvaderShot::Draw() const {
    // A yellow cone pointing at the player, like the yellow triangle of the 2D game.
    // The cone points the way the shot flies: (sideSpeed, 0, speed), made 0.4 long.
    const float length = std::sqrt(sideSpeed * sideSpeed + speed * speed);
    const float dx = 0.4f * sideSpeed / length;
    const float dz = 0.4f * speed / length;
    const Vector3 base{position.x - dx, position.y, position.z - dz};
    const Vector3 tip {position.x + dx, position.y, position.z + dz};
    DrawCylinderEx(base, tip, 0.2f, 0.0f, 8, YELLOW);
}
