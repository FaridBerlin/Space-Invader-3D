// Bomb.cpp

#include "Bomb.h"

#include <algorithm>   // std::clamp
#include <cmath>       // sinf

#include "Config.h"
#include "rlgl.h"

void Bomb::Update(float dt) {
    position.z += speed * dt;

    // Height depends only on how far along the path we are. "t" runs from 0
    // (spawn point) to 1 (the point where the bomb has levelled out) and stays
    // at 1 afterwards. Because the height is computed from the position instead
    // of being added up frame by frame, there is nothing to drift or get wrong
    // when the frame rate changes.
    const float t = (position.z - config::kBombSpawnZ) /
                    (config::kBombLevelOutZ - config::kBombSpawnZ);
    position.y = config::kBombSpawnY +
                 (config::kBombCruiseY - config::kBombSpawnY) * std::clamp(t, 0.0f, 1.0f);

    // Flew past the ship: mark for removal (Game deletes dead objects later).
    if (position.z > config::kBombDespawnZ) {
        alive = false;
    }
}

void Bomb::Draw(float time) const {
    // A dark core in the bomb's colour, plus two wireframe rings that spin
    // around it, so it reads as "danger" even from far away.
    const Color dark{static_cast<unsigned char>(color.r / 3),
                     static_cast<unsigned char>(color.g / 3),
                     static_cast<unsigned char>(color.b / 3), 255};
    DrawSphereEx(position, radius * 0.8f, 12, 12, dark);

    rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);
        rlRotatef(time * 120.0f, 0.0f, 1.0f, 0.0f);       // spin around the vertical axis
        DrawSphereWires(Vector3{0, 0, 0}, radius, 8, 12, color);
    rlPopMatrix();
}
