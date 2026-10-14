// Bomb.cpp

#include "Bomb.h"

#include <algorithm>   // std::clamp
#include <cmath>       // std::sin, std::fabs, std::fmod, std::floor, std::sqrt

#include "Config.h"
#include "raymath.h"   // Vector3 maths
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

namespace {

// A "random" number from 0 to 1 that is always the same for the same n. Effects
// use it so every spark and every lightning bolt has its own fixed direction,
// without storing anything. (The classic shader trick: the fractional part of a
// large, fast-changing sine.)
float Hash(float n) {
    const float x = std::sin(n * 12.9898f) * 43758.5453f;
    return x - std::floor(x);
}

// A direction (length 1) made from two hash values: an angle around, and a height.
Vector3 HashDirection(float n) {
    const float angle = Hash(n) * 2.0f * PI;
    const float y     = Hash(n + 7.31f) * 2.0f - 1.0f;   // -1 .. 1
    const float r     = std::sqrt(1.0f - y * y);
    return Vector3{r * std::cos(angle), y, r * std::sin(angle)};
}

Color Brighter(Color c, float t) {   // t = 0: c, t = 1: white
    auto mix = [t](unsigned char v) { return static_cast<unsigned char>(v + (255 - v) * t); };
    return Color{mix(c.r), mix(c.g), mix(c.b), 255};
}

}  // namespace

void Bomb::Draw(float time) const {
    // Each level adds an effect on top of the previous ones ("level >= n").
    const bool pulses    = level >= 1;   // level 2
    const bool hasRing   = level >= 2;   // level 3
    const bool hasSpikes = level >= 3;   // level 4
    const bool hasSparks = level >= 4;   // level 5
    const bool flickers  = level >= 5;   // level 6
    const bool cracked   = level >= 6;   // level 7

    const float pulse   = pulses ? 1.0f + 0.08f * std::sin(time * 5.0f) : 1.0f;
    // Flicker: a fast, uneven brightness change, like a faulty light.
    const float flicker = flickers ? 0.6f + 0.4f * std::fabs(std::sin(time * 23.0f + position.x * 3.0f)) : 1.0f;
    const Color shell   = Fade(color, flicker);

    // ---- The core: a dark ball in the bomb's colour (level 7: almost black) ------------
    const Color core = cracked ? Color{30, 6, 12, 255}
                               : Color{static_cast<unsigned char>(color.r / 3),
                                       static_cast<unsigned char>(color.g / 3),
                                       static_cast<unsigned char>(color.b / 3), 255};
    DrawSphereEx(position, radius * 0.8f, 12, 12, core);

    rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);
        rlRotatef(time * 120.0f, 0.0f, 1.0f, 0.0f);   // everything below spins with the bomb

        // The spinning wire cage, breathing in and out from level 2.
        DrawSphereWires(Vector3{0, 0, 0}, radius * pulse, 8, 12, shell);

        // Spikes: 8 cones pointing out to the corners of a cube. Longer from level 6.
        if (hasSpikes) {
            const float length = flickers ? 1.8f : 1.45f;
            for (int i = 0; i < 8; ++i) {
                const Vector3 dir = Vector3Normalize(Vector3{(i & 1) ? 1.0f : -1.0f,
                                                             (i & 2) ? 1.0f : -1.0f,
                                                             (i & 4) ? 1.0f : -1.0f});
                DrawCylinderEx(Vector3Scale(dir, radius * 0.75f), Vector3Scale(dir, radius * length),
                               radius * 0.14f, 0.0f, 6, shell);
            }
        }

        // Level 7: glowing red cracks running over the dark core.
        if (cracked) {
            const Color crack = Fade(Color{255, 60, 60, 255}, flicker);
            for (int c = 0; c < 6; ++c) {
                Vector3 previous = Vector3Scale(HashDirection(static_cast<float>(c) + 0.5f), radius * 0.82f);
                for (int step = 1; step <= 4; ++step) {
                    // Each step wanders a little further over the surface.
                    const Vector3 wander = Vector3Scale(HashDirection(c * 10.0f + step), 0.45f);
                    const Vector3 next = Vector3Scale(Vector3Normalize(Vector3Add(previous, wander)), radius * 0.82f);
                    DrawLine3D(previous, next, crack);
                    previous = next;
                }
            }
        }
    rlPopMatrix();

    // ---- Level 3+: an energy ring, tilted and turning the other way -----------------
    if (hasRing) {
        rlPushMatrix();
            rlTranslatef(position.x, position.y, position.z);
            rlRotatef(-time * 200.0f, 0.0f, 1.0f, 0.0f);
            rlRotatef(70.0f, 1.0f, 0.0f, 0.0f);
            DrawCircle3D(Vector3{0, 0, 0}, radius * 1.5f, Vector3{1, 0, 0}, 90.0f, Brighter(color, 0.4f));
            DrawCircle3D(Vector3{0, 0, 0}, radius * 1.56f, Vector3{1, 0, 0}, 90.0f, Fade(color, 0.6f));
        rlPopMatrix();
    }

    // ---- Level 5+: sparks streaming off the bomb and falling behind it ----------------
    // No particle system needed: every spark has a fixed direction (Hash) and a
    // "phase" from 0 to 1 that keeps cycling with time. At phase 0 it sits on the
    // surface; then it flies out, trails behind (-Z, the bomb flies toward +Z) and shrinks.
    if (hasSparks) {
        const int sparks = flickers ? 16 : 9;
        for (int i = 0; i < sparks; ++i) {
            const float phase = std::fmod(time * 1.4f + Hash(static_cast<float>(i)) , 1.0f);
            const Vector3 dir = HashDirection(static_cast<float>(i) + 3.7f);
            const Vector3 p = Vector3Add(position,
                Vector3Add(Vector3Scale(dir, radius * (1.0f + phase * 0.9f)),
                           Vector3{0.0f, 0.0f, -phase * radius * 2.5f}));
            const float size = radius * 0.13f * (1.0f - phase);
            DrawCube(p, size, size, size, Brighter(color, 0.6f));
        }
    }

    // ---- Level 7: small lightning bolts that jump to new places 12 times a second -----
    if (cracked) {
        const float seed = std::floor(time * 12.0f);   // the same for 1/12 s, then new
        for (int b = 0; b < 3; ++b) {
            const Vector3 dir = HashDirection(seed * 3.1f + static_cast<float>(b));
            Vector3 previous = Vector3Add(position, Vector3Scale(dir, radius));
            for (int step = 1; step <= 4; ++step) {
                // Zig-zag: move outward, plus a random sideways jump.
                const Vector3 jitter = Vector3Scale(HashDirection(seed + b * 5.0f + step), radius * 0.25f);
                const Vector3 next = Vector3Add(
                    Vector3Add(position, Vector3Scale(dir, radius * (1.0f + 0.3f * static_cast<float>(step)))), jitter);
                DrawLine3D(previous, next, Color{255, 230, 230, 255});
                previous = next;
            }
        }
    }
}

float Bomb::GlowSize(float time) const {
    float size = radius * config::kBombGlowFactor;
    if (level >= 1) size *= 1.0f + 0.06f * std::sin(time * 5.0f);   // breathes with the pulse
    if (level >= 6) size *= 1.3f + 0.25f * std::sin(time * 7.0f);   // level 7: big, throbbing
    return size;
}
