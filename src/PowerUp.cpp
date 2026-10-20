// PowerUp.cpp

#include "PowerUp.h"

#include <algorithm>   // std::max, std::min
#include <cmath>       // std::sin, std::exp, std::fmod

#include "Config.h"
#include "LowPoly.h"   // lowpoly::Mix

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
        case PowerUpType::Weapon:    return Color{170,  80, 255, 255};   // purple
        case PowerUpType::Spread:    return config::kSpreadShotColor;    // green
        case PowerUpType::Laser:     return config::kLaserColor;         // orange
        case PowerUpType::Shield:    return Color{ 80, 220, 255, 255};   // cyan
        case PowerUpType::RapidFire: return Color{255, 225,  60, 255};   // yellow
        case PowerUpType::ExtraLife: return Color{255,  90, 150, 255};   // pink
    }
    return Color{255, 255, 255, 255};
}

PowerUpType PowerUp::RandomType() {
    // A number from 1 to 100. The chances in Config.h are slices of 1..100 laid
    // end to end: the first kWeaponOrbChance numbers are a weapon orb, the next
    // kShieldChance a shield, and so on.
    int roll = GetRandomValue(1, 100);
    if ((roll -= config::kWeaponOrbChance) <= 0) return PowerUpType::Weapon;
    if ((roll -= config::kSpreadOrbChance) <= 0) return PowerUpType::Spread;
    if ((roll -= config::kLaserOrbChance) <= 0)  return PowerUpType::Laser;
    if ((roll -= config::kShieldChance) <= 0)    return PowerUpType::Shield;
    if ((roll -= config::kRapidFireChance) <= 0) return PowerUpType::RapidFire;
    return PowerUpType::ExtraLife;
}

float PowerUp::Bob() const {
    return 0.15f * std::sin(age * 5.0f);
}

void PowerUp::Draw() const {
    // A see-through ball in the orb's colour. The glow around it is drawn by Game
    // (with the other glows), and the bright rim and symbol by DrawIcon.
    const Vector3 center{position.x, position.y + Bob(), position.z};
    DrawSphereEx(center, config::kPowerUpOrbSize, 10, 14, Fade(MainColor(), 0.30f));
}

namespace {

// One arrowhead pointing up ("^"), as two thick lines. (cx, cy) is its tip.
void DrawChevron(float cx, float cy, float width, float height, float thick, Color color) {
    DrawLineEx(Vector2{cx - width / 2.0f, cy + height}, Vector2{cx, cy}, thick, color);
    DrawLineEx(Vector2{cx, cy}, Vector2{cx + width / 2.0f, cy + height}, thick, color);
    // Round joints, so the tip and the ends do not look cut off.
    DrawCircleV(Vector2{cx, cy}, thick / 2.0f, color);
    DrawCircleV(Vector2{cx - width / 2.0f, cy + height}, thick / 2.0f, color);
    DrawCircleV(Vector2{cx + width / 2.0f, cy + height}, thick / 2.0f, color);
}

}  // namespace

void PowerUp::DrawIcon(const Camera3D& camera) const {
    // Where is the orb on the screen, and how big? We project its centre, and a
    // point one orb-radius to the side: the distance between them is the radius
    // in pixels. So the icon shrinks with distance like a 3D object would.
    const Vector3 center{position.x, position.y + Bob(), position.z};
    const Vector2 c = GetWorldToScreen(center, camera);
    const Vector2 side = GetWorldToScreen(Vector3{center.x + config::kPowerUpOrbSize, center.y, center.z}, camera);
    const float pulse = 1.0f + 0.06f * std::sin(age * 8.0f);
    const float r = (side.x - c.x) * pulse;
    if (r < 2.0f) return;   // far away (or behind the camera): too small to draw

    const Color main = MainColor();
    const Color white{255, 255, 255, 255};
    // The ball: lighter in the middle, darker at the edge; then a bright rim.
    DrawCircleGradient(static_cast<int>(c.x), static_cast<int>(c.y), r,
                       Fade(lowpoly::Mix(main, white, 0.35f), 0.55f), Fade(main, 0.15f));
    DrawRing(c, r * 0.86f, r, 0.0f, 360.0f, 32, Fade(lowpoly::Mix(main, white, 0.5f), 0.95f));

    const float thick = std::max(2.0f, r * 0.16f);
    switch (type) {
        case PowerUpType::Weapon: {
            // Three arrowheads stacked on top of each other, gliding upward and
            // fading in at the bottom and out at the top: "more power".
            const float step = r * 0.36f;
            const float shift = std::fmod(age * 1.2f, 1.0f) * step;   // 0 .. step, then again
            for (int i = 0; i < 3; ++i) {
                const float tipY = c.y + r * 0.10f + step - static_cast<float>(i) * step - shift;
                const float t = (static_cast<float>(i) * step + shift) / (3.0f * step);   // 0 bottom .. 1 top
                const float alpha = std::min(1.0f, std::min(t * 4.0f, (1.0f - t) * 4.0f) + 0.25f);
                DrawChevron(c.x, tipY - r * 0.28f, r * 0.95f, r * 0.30f, thick, Fade(white, alpha));
            }
            break;
        }
        case PowerUpType::Spread: {
            // A fan: five rays from one point at the bottom, each with a dot at its end.
            const Vector2 root{c.x, c.y + r * 0.55f};
            for (int i = 0; i < 5; ++i) {
                const float angle = (-50.0f + 25.0f * static_cast<float>(i)) * DEG2RAD;
                const Vector2 tip{root.x + std::sin(angle) * r * 0.95f, root.y - std::cos(angle) * r * 0.95f};
                DrawLineEx(root, tip, thick * 0.7f, white);
                DrawCircleV(tip, thick * 0.75f, white);
            }
            break;
        }
        case PowerUpType::RapidFire: {
            // "x2": twice as fast.
            const int size = static_cast<int>(r * 0.95f);
            const char* text = "x2";
            DrawText(text, static_cast<int>(c.x) - MeasureText(text, size) / 2, static_cast<int>(c.y) - size / 2,
                     size, white);
            break;
        }
        case PowerUpType::Laser: {
            // A lightning bolt: two triangles (corners counter-clockwise on screen).
            const float s = r * 0.62f;
            DrawTriangle(Vector2{c.x + s * 0.25f, c.y - s}, Vector2{c.x - s * 0.55f, c.y + s * 0.15f},
                         Vector2{c.x + s * 0.10f, c.y + s * 0.15f}, white);
            DrawTriangle(Vector2{c.x - s * 0.10f, c.y - s * 0.15f}, Vector2{c.x - s * 0.25f, c.y + s},
                         Vector2{c.x + s * 0.55f, c.y - s * 0.15f}, white);
            break;
        }
        case PowerUpType::Shield: {
            // A shield: flat at the top, coming to a point at the bottom.
            const float s = r * 0.58f;
            const Vector2 tl{c.x - s * 0.80f, c.y - s * 0.75f};
            const Vector2 tr{c.x + s * 0.80f, c.y - s * 0.75f};
            const Vector2 ml{c.x - s * 0.80f, c.y + s * 0.05f};
            const Vector2 mr{c.x + s * 0.80f, c.y + s * 0.05f};
            const Vector2 tip{c.x, c.y + s};
            DrawTriangle(tl, ml, mr, white);
            DrawTriangle(tl, mr, tr, white);
            DrawTriangle(ml, tip, mr, white);
            DrawLineEx(Vector2{c.x, c.y - s * 0.55f}, Vector2{c.x, c.y + s * 0.65f}, thick * 0.7f, main);
            break;
        }
        case PowerUpType::ExtraLife: {
            const int size = static_cast<int>(r * 0.95f);
            const char* text = "+1";
            DrawText(text, static_cast<int>(c.x) - MeasureText(text, size) / 2, static_cast<int>(c.y) - size / 2,
                     size, white);
            break;
        }
    }
}
