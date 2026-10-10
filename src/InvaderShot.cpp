// InvaderShot.cpp

#include "InvaderShot.h"

#include "Config.h"

void InvaderShot::Update(float dt) {
    // Toward the player = toward +Z (a rocket flies toward -Z).
    position.z += speed * dt;

    // Flew past the ship without hitting it: mark for removal.
    if (position.z > config::kInvaderShotDespawnZ) {
        alive = false;
    }
}

void InvaderShot::Draw() const {
    // A yellow cone pointing at the player, like the yellow triangle of the 2D game.
    const Vector3 base{position.x, position.y, position.z - 0.4f};
    const Vector3 tip {position.x, position.y, position.z + 0.4f};
    DrawCylinderEx(base, tip, 0.2f, 0.0f, 8, YELLOW);
}
