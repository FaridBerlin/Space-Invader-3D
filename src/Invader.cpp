// Invader.cpp - how an alien looks.

#include "Invader.h"

#include <cmath>

namespace {

void DrawBox(Vector3 center, Vector3 size, Color fill, Color edge) {
    DrawCube(center, size.x, size.y, size.z, fill);
    DrawCubeWires(center, size.x, size.y, size.z, edge);
}

}  // namespace

void Invader::Draw(float time) const {
    // Bob up and down. Adding position.x to the angle makes neighbours move
    // out of step, so the wave looks like it is wobbling, not like a block.
    const float bob = 0.12f * std::sin(time * 4.0f + position.x);

    // A small helper (a "lambda"): turns an offset from the alien's centre into
    // a world position. "[&]" lets it read the variables around it.
    auto at = [&](float dx, float dy, float dz) {
        return Vector3{position.x + dx, position.y + bob + dy, position.z + dz};
    };

    const Color body {150, 90, 230, 255};   // purple, like the 2D sprite
    const Color dark {70, 30, 130, 255};
    const Color white{245, 245, 255, 255};

    // The alien faces the player, which is toward +Z.
    DrawBox(at(0.0f, 0.0f, 0.0f),    {1.3f, 0.8f, 0.9f}, body, dark);   // body
    DrawBox(at(0.0f, 0.55f, 0.0f),   {0.8f, 0.35f, 0.7f}, body, dark);  // head bump
    DrawBox(at(-0.28f, 0.1f, 0.46f), {0.22f, 0.22f, 0.06f}, white, dark);  // left eye
    DrawBox(at( 0.28f, 0.1f, 0.46f), {0.22f, 0.22f, 0.06f}, white, dark);  // right eye
    DrawBox(at(-0.3f, 0.95f, 0.0f),  {0.1f, 0.4f, 0.1f}, body, dark);   // left antenna
    DrawBox(at( 0.3f, 0.95f, 0.0f),  {0.1f, 0.4f, 0.1f}, body, dark);   // right antenna
    DrawBox(at(-0.45f, -0.55f, 0.0f), {0.25f, 0.35f, 0.5f}, dark, dark); // left leg
    DrawBox(at( 0.45f, -0.55f, 0.0f), {0.25f, 0.35f, 0.5f}, dark, dark); // right leg
}
