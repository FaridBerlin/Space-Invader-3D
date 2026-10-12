// Invader.cpp - how the aliens look.
//
// The Scarab is built from flat triangles (see LowPoly.h), like the player's ship.
// It is drawn "standing up" and facing the player (+Z), like a picture in a
// frame, with some depth so it still looks 3D:
//
//            ___---___          a round shell, open at the bottom,
//          /    |     \         with glowing lime "circuit" lines
//         |  -- O --   |        and a glowing core (O) in the middle
//          \__ /   \ __/
//            (       )          two crescent claws that slowly open and close
//             `.   .'

#include "Invader.h"

#include <cmath>              // std::sin, std::cos
#include <initializer_list>   // lets us loop over a list like {-1.0f, 1.0f}

#include "Config.h"
#include "LowPoly.h"
#include "raymath.h"
#include "rlgl.h"

namespace {

using lowpoly::DrawFace;
using lowpoly::DrawQuadFace;
using lowpoly::Mix;

// The colours of one alien design.
struct ScarabStyle {
    Color shell;   // the front of the shell
    Color back;    // the back and the inside (mostly hidden)
    Color claw;
    Color lines;   // the glowing circuit lines and the core
    float scale;   // 1.0 = normal size
};

constexpr ScarabStyle kScarabStyle{
    { 95,  60, 150, 255},   // violet
    { 40,  25,  70, 255},   // very dark violet
    { 80,  48, 125, 255},
    {160, 255,  60, 255},   // lime green
    0.78f,
};

// Used for the commander when its model file is missing: bigger, red-violet and gold.
constexpr ScarabStyle kCommanderFallbackStyle{
    {130,  45,  80, 255},
    { 50,  20,  35, 255},
    {110,  40,  70, 255},
    {255, 200,  60, 255},   // gold
    1.15f,
};

// A point on the shell's rim. Angle in degrees, measured like on a clock face
// turned sideways: 0 = right, 90 = top, 180 = left, 270 = bottom.
Vector3 RimPoint(float degrees, float radius) {
    const float a = degrees * DEG2RAD;
    return Vector3{radius * std::cos(a), radius * std::sin(a), 0.0f};
}

// A point on a "quadratic Bezier curve": the smooth curve that starts at p0,
// ends at p2 and is pulled toward p1 on the way. t goes from 0 to 1.
Vector2 Bezier(Vector2 p0, Vector2 p1, Vector2 p2, float t) {
    const float u = 1.0f - t;
    return Vector2{u * u * p0.x + 2.0f * u * t * p1.x + t * t * p2.x,
                   u * u * p0.y + 2.0f * u * t * p1.y + t * t * p2.y};
}

// One crescent claw. side = -1 for the left claw, +1 for the right one.
// The claw is a curved strip that gets thinner toward its tip.
void DrawClaw(float side, const ScarabStyle& style, Color claw, Color lines) {
    const Vector2 p0{side * 0.42f, -0.50f};   // root, under the shell
    const Vector2 p1{side * 0.70f, -1.05f};   // the curve bulges outward ...
    const Vector2 p2{side * 0.08f, -1.25f};   // ... and the tip points inward
    const int segments = 7;

    // Walk along the curve. At each step, build a cross-section: an inner and
    // an outer point (the claw's width), each at the front and the back (depth).
    Vector3 prevInnerF{}, prevOuterF{}, prevInnerB{}, prevOuterB{};
    for (int i = 0; i <= segments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const Vector2 point = Bezier(p0, p1, p2, t);

        // Direction of the curve here, and the direction across it (turned 90 degrees).
        const Vector2 ahead = Bezier(p0, p1, p2, std::fmin(1.0f, t + 0.01f));
        const Vector2 behind = Bezier(p0, p1, p2, std::fmax(0.0f, t - 0.01f));
        Vector2 across{-(ahead.y - behind.y), ahead.x - behind.x};
        const float length = std::sqrt(across.x * across.x + across.y * across.y);
        across = Vector2{across.x / length, across.y / length};

        const float halfWidth = 0.16f * (1.0f - t) + 0.015f;   // thinner toward the tip
        const float depth     = 0.10f * (1.0f - t) + 0.01f;
        const Vector3 innerF{point.x - across.x * halfWidth, point.y - across.y * halfWidth,  depth};
        const Vector3 outerF{point.x + across.x * halfWidth, point.y + across.y * halfWidth,  depth};
        const Vector3 innerB{innerF.x, innerF.y, -depth};
        const Vector3 outerB{outerF.x, outerF.y, -depth};

        if (i > 0) {
            const Vector3 front{0.0f, 0.0f, 1.0f};
            const Vector3 back{0.0f, 0.0f, -1.0f};
            const Vector3 out{across.x, across.y, 0.0f};
            DrawQuadFace(prevInnerF, prevOuterF, outerF, innerF, claw, front);
            DrawQuadFace(prevInnerB, prevOuterB, outerB, innerB, style.back, back);
            DrawQuadFace(prevOuterF, prevOuterB, outerB, outerF, claw, out);
            DrawQuadFace(prevInnerF, prevInnerB, innerB, innerF, claw, Vector3Negate(out));
            // A glowing line along the claw's front edge.
            DrawLine3D(Vector3{prevOuterF.x, prevOuterF.y, prevOuterF.z + 0.01f},
                       Vector3{outerF.x, outerF.y, outerF.z + 0.01f}, lines);
        }
        prevInnerF = innerF; prevOuterF = outerF; prevInnerB = innerB; prevOuterB = outerB;
    }
}

// The whole Scarab, drawn around the origin (the caller moves it into place).
//   flash  0..1, how much the alien lights up white after a hit
//   pulse  0..1, how bright the core glows right now
//   claws  the claws' opening angle in degrees
void DrawScarab(const ScarabStyle& style, float flash, float pulse, float claws) {
    const Color white{255, 255, 255, 255};
    const Color shell = Mix(style.shell, white, flash);
    const Color back  = Mix(style.back,  white, flash);
    const Color claw  = Mix(style.claw,  white, flash);
    const Color lines = Mix(style.lines, white, flash);

    // ---- The shell: two halves, each a fan of triangles from a centre to the rim ----
    // The rim is a circle with a gap at the bottom (from 230 to 310 degrees), where
    // the claws come out. The centre bulges toward the player, so every triangle
    // tilts a little differently and gets its own shade. The two halves are pushed
    // apart a little, leaving a dark seam down the middle.
    const float radius = 0.72f;
    const float seam   = 0.035f;   // half the gap between the two halves
    const int segmentsPerHalf = 8;

    auto drawHalf = [&](float fromAngle, float toAngle, float shift) {
        const Vector3 offset{shift, 0.0f, 0.0f};
        const Vector3 frontCentre = Vector3Add(Vector3{0.0f, 0.05f, 0.42f}, offset);
        const Vector3 backCentre  = Vector3Add(Vector3{0.0f, 0.05f, -0.35f}, offset);
        Vector3 previous = Vector3Add(RimPoint(fromAngle, radius), offset);
        for (int i = 1; i <= segmentsPerHalf; ++i) {
            const float angle = fromAngle + (toAngle - fromAngle) * static_cast<float>(i) / segmentsPerHalf;
            const Vector3 rim = Vector3Add(RimPoint(angle, radius), offset);
            DrawFace(frontCentre, previous, rim, shell, lowpoly::Mid(frontCentre, previous, rim));
            DrawFace(backCentre, previous, rim, back, lowpoly::Mid(backCentre, previous, rim));
            // A faint lime outline along the rim, so the shape reads from far away.
            DrawLine3D(previous, rim, Fade(lines, 0.55f));
            previous = rim;
        }
        // Close the open sides of this half (at the seam and at the bottom gap).
        const Vector3 first = Vector3Add(RimPoint(fromAngle, radius), offset);
        const Vector3 last  = Vector3Add(RimPoint(toAngle, radius), offset);
        DrawFace(frontCentre, backCentre, first, back, Vector3{-shift, -1.0f, 0.0f});
        DrawFace(frontCentre, backCentre, last,  back, Vector3{-shift, -1.0f, 0.0f});
    };
    drawHalf(-50.0f, 90.0f,  seam);    // right half: from the bottom gap up to the top
    drawHalf(90.0f, 230.0f, -seam);    // left half: from the top down to the bottom gap

    // ---- Glowing circuit lines on the shell -------------------------------------------
    // Each line runs part of the way from the centre toward the rim. "lift" moves
    // the lines a hair toward the player, so they sit ON the shell, not inside it.
    const Vector3 frontCentre{0.0f, 0.05f, 0.42f};
    const Vector3 lift{0.0f, 0.0f, 0.03f};
    auto onShell = [&](float degrees, float t) {   // t = 0 at the centre, 1 at the rim
        const float shift = (degrees > 90.0f && degrees < 270.0f) ? -seam : seam;
        const Vector3 p = Vector3Lerp(frontCentre, RimPoint(degrees, radius), t);
        return Vector3Add(p, Vector3{shift, 0.0f, lift.z + 0.03f * (1.0f - t)});
    };
    for (const float degrees : {20.0f, 55.0f, 125.0f, 160.0f, 200.0f, -20.0f}) {
        DrawLine3D(onShell(degrees, 0.35f), onShell(degrees, 0.85f), lines);
        // A short side branch at the end, like a line on a circuit board.
        const float branch = (degrees > 90.0f && degrees < 270.0f) ? -12.0f : 12.0f;
        DrawLine3D(onShell(degrees, 0.85f), onShell(degrees + branch, 0.95f), lines);
    }
    // Two glowing horseshoe marks, one on each half: small "U" shapes made of lines.
    for (const float side : {-1.0f, 1.0f}) {
        const Vector3 centre = Vector3Add(onShell(side > 0.0f ? 62.0f : 118.0f, 0.55f),
                                          Vector3{0.0f, 0.0f, 0.02f});
        Vector3 previousPoint{};
        for (int i = 0; i <= 8; ++i) {
            const float a = (200.0f + 140.0f * static_cast<float>(i) / 8.0f) * DEG2RAD;   // lower half circle
            const Vector3 point{centre.x + 0.09f * std::cos(a), centre.y + 0.09f * std::sin(a), centre.z};
            if (i > 0) DrawLine3D(previousPoint, point, lines);
            previousPoint = point;
        }
    }

    // ---- The glowing core, pulsing between lime and white ---------------------------
    DrawSphereEx(Vector3{0.0f, -0.05f, 0.42f}, 0.10f, 8, 8, Mix(lines, white, 0.5f * pulse));

    // ---- The claws, each turning around its root to open and close ------------------
    for (const float side : {-1.0f, 1.0f}) {
        const Vector3 root{side * 0.42f, -0.50f, 0.0f};
        rlPushMatrix();
        rlTranslatef(root.x, root.y, root.z);
        rlRotatef(side * claws, 0.0f, 0.0f, 1.0f);   // mirror the angle for the left claw
        rlTranslatef(-root.x, -root.y, -root.z);
        DrawClaw(side, style, claw, lines);
        rlPopMatrix();
    }
}

}  // namespace

Invader::Invader(Vector3 start, AlienType alienType)
    : position(start),
      type(alienType),
      hitPoints(alienType == AlienType::Commander ? config::kCommanderHitPoints : 1) {}

void Invader::Update(float dt) {
    if (flash > 0.0f) {
        flash -= dt;
    }
}

float Invader::Radius() const {
    return type == AlienType::Commander ? config::kCommanderRadius : config::kInvaderRadius;
}

void Invader::Draw(float time, const Model* commanderModel) const {
    // Bob up and down. Adding position.x to the angle makes neighbours move
    // out of step, so the wave looks like it is wobbling, not like a block.
    const float bob = 0.12f * std::sin(time * 4.0f + position.x);
    const float flashAmount = flash > 0.0f ? flash / config::kHitFlashTime : 0.0f;   // 1 .. 0

    // Commanders hover a little higher than the rest, so you can see them over
    // the rows in front. (Only the picture moves; the hit area stays where it is.)
    const float hover = (type == AlienType::Commander) ? config::kCommanderHover : 0.0f;

    rlPushMatrix();
    rlTranslatef(position.x, position.y + bob + hover, position.z);

    if (type == AlienType::Commander && commanderModel != nullptr) {
        // The model's detailed side faces -Z and its nose points up (+Y). Turning it
        // 180 degrees around X fixes both: the detail faces us and the nose points
        // down toward the player. Then we tilt it 17 degrees toward the camera.
        rlRotatef(163.0f, 1.0f, 0.0f, 0.0f);
        // A hit tints the model red for a moment ("tint" multiplies every colour).
        const Color tint = Mix(Color{255, 255, 255, 255}, Color{255, 90, 90, 255}, flashAmount);
        DrawModel(*commanderModel, Vector3{0.0f, 0.0f, 0.0f}, config::kCommanderModelScale, tint);
    } else {
        const ScarabStyle& style =
            (type == AlienType::Commander) ? kCommanderFallbackStyle : kScarabStyle;
        rlScalef(style.scale, style.scale, style.scale);

        const float pulse = 0.5f + 0.5f * std::sin(time * 6.0f + position.x * 0.7f);
        const float claws = 8.0f + 10.0f * std::sin(time * 3.0f + position.x);   // degrees
        DrawScarab(style, flashAmount, pulse, claws);
    }

    rlPopMatrix();
}
