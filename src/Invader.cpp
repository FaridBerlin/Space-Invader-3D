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
    0.94f,
};

// Used for the commander when its model file is missing: bigger, red-violet and gold.
constexpr ScarabStyle kCommanderFallbackStyle{
    {130,  45,  80, 255},
    { 50,  20,  35, 255},
    {110,  40,  70, 255},
    {255, 200,  60, 255},   // gold
    1.30f,
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

// A curved blade: a strip that follows a Bezier curve from p0 to p2 and gets
// thinner toward its tip. Used for the Scarab's claws and the Reaper's wings.
//   width, depth  half the width and half the thickness at the root
void DrawBlade(Vector2 p0, Vector2 p1, Vector2 p2, float width, float depth,
               Color front, Color back, Color line) {
    const int segments = 7;

    // Walk along the curve. At each step, build a cross-section: an inner and
    // an outer point (the blade's width), each at the front and the back (depth).
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

        const float halfWidth = width * (1.0f - t) + 0.015f;   // thinner toward the tip
        const float halfDepth = depth * (1.0f - t) + 0.01f;
        const Vector3 innerF{point.x - across.x * halfWidth, point.y - across.y * halfWidth,  halfDepth};
        const Vector3 outerF{point.x + across.x * halfWidth, point.y + across.y * halfWidth,  halfDepth};
        const Vector3 innerB{innerF.x, innerF.y, -halfDepth};
        const Vector3 outerB{outerF.x, outerF.y, -halfDepth};

        if (i > 0) {
            const Vector3 toFront{0.0f, 0.0f, 1.0f};
            const Vector3 toBack{0.0f, 0.0f, -1.0f};
            const Vector3 out{across.x, across.y, 0.0f};
            DrawQuadFace(prevInnerF, prevOuterF, outerF, innerF, front, toFront);
            DrawQuadFace(prevInnerB, prevOuterB, outerB, innerB, back, toBack);
            DrawQuadFace(prevOuterF, prevOuterB, outerB, outerF, front, out);
            DrawQuadFace(prevInnerF, prevInnerB, innerB, innerF, front, Vector3Negate(out));
            // A glowing line along the blade's front edge.
            DrawLine3D(Vector3{prevOuterF.x, prevOuterF.y, prevOuterF.z + 0.01f},
                       Vector3{outerF.x, outerF.y, outerF.z + 0.01f}, line);
        }
        prevInnerF = innerF; prevOuterF = outerF; prevInnerB = innerB; prevOuterB = outerB;
    }
}

// One crescent claw of the Scarab. side = -1 for the left claw, +1 for the right one.
void DrawClaw(float side, Color claw, Color back, Color lines) {
    DrawBlade(Vector2{side * 0.42f, -0.50f},   // root, under the shell
              Vector2{side * 0.70f, -1.05f},   // the curve bulges outward ...
              Vector2{side * 0.08f, -1.25f},   // ... and the tip points inward
              0.16f, 0.10f, claw, back, lines);
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
        DrawClaw(side, claw, back, lines);
        rlPopMatrix();
    }
}

// ---- The Reaper -------------------------------------------------------------------
// A dark armoured body pointing down at the player, red chevron stripes, two big
// horn-wings sweeping out and down, and glowing red pods along its sides.
//   flash  0..1 white after a hit, pulse 0..1 glow of the pods, wings = flap angle
void DrawReaper(float flash, float pulse, float wings) {
    const Color white{255, 255, 255, 255};
    const Color body  = Mix(Color{ 92,  92, 108, 255}, white, flash);
    const Color wing  = Mix(Color{ 76,  76,  90, 255}, white, flash);
    const Color back  = Mix(Color{ 25,  25,  30, 255}, white, flash);
    const Color red   = Mix(Color{230,  35,  45, 255}, white, flash);
    const Color glow  = Mix(Color{255,  70,  70, 255}, white, std::fmax(flash, 0.4f * pulse));

    // ---- Body: a diamond with a ridge down the middle -------------------------------
    //        top
    //   left  ridge  right      (ridge = the front point, bulging toward us)
    //        tip                (the tip points down, toward the player)
    const Vector3 top  { 0.00f,  0.55f, 0.00f};
    const Vector3 left {-0.34f,  0.10f, 0.00f};
    const Vector3 right{ 0.34f,  0.10f, 0.00f};
    const Vector3 tip  { 0.00f, -0.95f, 0.00f};
    const Vector3 ridge{ 0.00f,  0.00f, 0.32f};   // front
    const Vector3 keel { 0.00f,  0.00f, -0.25f};  // back
    for (const Vector3 centre : {ridge, keel}) {
        const Color c = (centre.z > 0.0f) ? body : back;
        DrawFace(centre, top, left, c, lowpoly::Mid(centre, top, left));
        DrawFace(centre, top, right, c, lowpoly::Mid(centre, top, right));
        DrawFace(centre, left, tip, c, lowpoly::Mid(centre, left, tip));
        DrawFace(centre, right, tip, c, lowpoly::Mid(centre, right, tip));
    }

    // ---- Red chevrons: "V" stripes across the front -----------------------------------
    // Each stripe runs from the body's edge to the ridge and out to the other edge.
    // Both ends lie on the same flat face, so the line sits exactly on the body;
    // "lift" moves it a hair toward us so it is not hidden inside the face.
    const Vector3 lift{0.0f, 0.0f, 0.02f};
    auto edgePoint = [&](float y, float side) {        // on the outline of the body
        const Vector3 corner = (side < 0.0f) ? left : right;
        if (y > corner.y) return Vector3Lerp(corner, top, (y - corner.y) / (top.y - corner.y));
        return Vector3Lerp(corner, tip, (corner.y - y) / (corner.y - tip.y));
    };
    auto ridgePoint = [&](float y) {                    // on the line down the middle
        if (y > ridge.y) return Vector3Lerp(ridge, top, (y - ridge.y) / (top.y - ridge.y));
        return Vector3Lerp(ridge, tip, (ridge.y - y) / (ridge.y - tip.y));
    };
    for (const float y : {0.40f, 0.26f, -0.15f, -0.35f, -0.55f, -0.72f}) {
        const Vector3 middle = Vector3Add(ridgePoint(y - 0.08f), lift);   // the V points down
        DrawLine3D(Vector3Add(edgePoint(y, -1.0f), lift), middle, red);
        DrawLine3D(middle, Vector3Add(edgePoint(y, 1.0f), lift), red);
    }

    // ---- Horn-wings, flapping slowly around their roots --------------------------------
    for (const float side : {-1.0f, 1.0f}) {
        const Vector3 root{side * 0.22f, 0.45f, 0.0f};
        rlPushMatrix();
        rlTranslatef(root.x, root.y, root.z);
        rlRotatef(side * wings, 0.0f, 0.0f, 1.0f);
        rlTranslatef(-root.x, -root.y, -root.z);
        DrawBlade(Vector2{side * 0.22f, 0.45f},    // root, at the shoulder
                  Vector2{side * 1.20f, 0.90f},    // sweeps up and out ...
                  Vector2{side * 0.92f, -0.40f},   // ... and down to a point
                  0.17f, 0.08f, wing, back, red);
        rlPopMatrix();

        // Three glowing pods down each side, like the bumps in the picture.
        for (const float y : {0.02f, -0.28f, -0.56f}) {
            const float x = edgePoint(y, side).x + side * 0.06f;   // sitting on the body's edge
            DrawSphereEx(Vector3{x, y, 0.0f}, 0.10f, 6, 6, back);
            DrawSphereEx(Vector3{x, y, 0.08f}, 0.05f, 6, 6, glow);
        }
    }
}

// ---- The Drone ----------------------------------------------------------------------
// A small cyan crystal (an "octahedron": two pyramids glued together) that spins,
// with a ring of light around it. Spinning makes each face catch the lamp in turn,
// so it sparkles.
//   flash 0..1, spin = rotation angle in degrees
void DrawDrone(float flash, float spin) {
    const Color white{255, 255, 255, 255};
    const Color crystal = Mix(Color{ 70, 195, 255, 255}, white, flash);
    const Color ring    = Mix(Color{150, 240, 255, 255}, white, flash);

    rlPushMatrix();
    rlRotatef(spin, 0.0f, 1.0f, 0.0f);   // spin around the vertical axis

    const Vector3 top   {0.0f,  0.50f, 0.0f};
    const Vector3 bottom{0.0f, -0.50f, 0.0f};
    const Vector3 around[4] = {{0.36f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.36f},
                               {-0.36f, 0.0f, 0.0f}, {0.0f, 0.0f, -0.36f}};
    for (int i = 0; i < 4; ++i) {
        const Vector3 a = around[i];
        const Vector3 b = around[(i + 1) % 4];   // % 4 wraps 4 back to 0
        DrawFace(top, a, b, crystal, lowpoly::Mid(top, a, b));
        DrawFace(bottom, a, b, crystal, lowpoly::Mid(bottom, a, b));
        DrawLine3D(top, a, ring);       // bright edges make the crystal sparkle
        DrawLine3D(bottom, a, ring);
    }
    rlPopMatrix();

    // The ring: a circle of short lines, tilted and turning the other way.
    rlPushMatrix();
    rlRotatef(-1.7f * spin, 0.0f, 1.0f, 0.0f);
    rlRotatef(25.0f, 1.0f, 0.0f, 0.0f);
    const int pieces = 20;
    for (int i = 0; i < pieces; i += 1) {
        const float a0 = 2.0f * PI * static_cast<float>(i) / pieces;
        const float a1 = 2.0f * PI * static_cast<float>(i + 1) / pieces;
        if (i % 5 == 4) continue;   // leave small gaps: it looks like a dashed energy ring
        DrawLine3D(Vector3{0.62f * std::cos(a0), 0.0f, 0.62f * std::sin(a0)},
                   Vector3{0.62f * std::cos(a1), 0.0f, 0.62f * std::sin(a1)}, ring);
    }
    rlPopMatrix();
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
    switch (type) {
        case AlienType::Commander: return config::kCommanderRadius;
        case AlienType::Drone:     return config::kDroneRadius;
        default:                   return config::kInvaderRadius;
    }
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
    } else if (type == AlienType::Reaper) {
        rlScalef(config::kReaperScale, config::kReaperScale, config::kReaperScale);
        const float pulse = 0.5f + 0.5f * std::sin(time * 5.0f + position.x);
        const float wings = 6.0f * std::sin(time * 2.5f + position.x * 0.5f);   // degrees
        DrawReaper(flashAmount, pulse, wings);
    } else if (type == AlienType::Drone) {
        rlScalef(config::kDroneScale, config::kDroneScale, config::kDroneScale);
        DrawDrone(flashAmount, time * 90.0f + position.x * 20.0f);   // 90 degrees per second
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
