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

#include <algorithm>          // std::clamp
#include <cmath>              // std::sin, std::cos
#include <cstddef>            // std::size_t
#include <initializer_list>   // lets us loop over a list like {-1.0f, 1.0f}
#include <iterator>           // std::size

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

// The Scarab as designed. In the game its colours come from the level (see
// kLevelAlienColors below); only its size is taken from here.
constexpr ScarabStyle kScarabStyle{
    {191, 132, 255, 255},   // violet
    { 96,  66, 128, 255},   // dark violet
    {162, 112, 217, 255},
    {160, 255,  60, 255},   // lime green
    0.94f,
};

// Used for the commander when its model file is missing: bigger, red-violet and gold.
constexpr ScarabStyle kCommanderFallbackStyle{
    {225,  80, 140, 255},   // bright red-violet (it was too dark on phones)
    {120,  45,  80, 255},
    {195,  70, 120, 255},
    {255, 200,  60, 255},   // gold
    1.30f,
};

// ---- Colours per level ----------------------------------------------------------------
// Body and glow colour of the wave in each level. Chosen so the glow never looks
// like that level's bomb (see kLevelBombColor in Config.h). The body colours are
// light on purpose: the background is dark, and the aliens must stand out from it.
constexpr LevelAlienColors kLevelAlienColors[] = {
    {{191, 132, 255, 255}, {160, 255,  60, 255}},   // 1 violet and lime
    {{ 89, 210, 235, 255}, {255, 205,  70, 255}},   // 2 teal and gold
    {{106, 132, 255, 255}, {255, 160,  60, 255}},   // 3 blue and orange
    {{ 89, 200, 132, 255}, {255, 110, 200, 255}},   // 4 green and pink
    {{217,  89, 149, 255}, { 80, 240, 220, 255}},   // 5 raspberry and aqua
    {{251, 166,  89, 255}, {255, 240, 170, 255}},   // 6 bronze and white-gold
    {{160, 125, 178, 255}, {255,  40,  70, 255}},   // 7 dusky violet and crimson
    {{180, 207, 230, 255}, { 90, 255, 150, 255}},   // 8 silver and green (the green crab)
    {{200, 135, 248, 255}, {255,  60, 110, 255}},   // 9 purple and crimson (the purple crab)
    {{255,  91, 108, 255}, {255, 190,  60, 255}},   // 10 red and gold (the red crab)
};
static_assert(std::size(kLevelAlienColors) == static_cast<std::size_t>(config::kLevelCount),
              "one alien colour scheme per level");

// A colour made darker: f = 0.5 gives half the brightness.
Color Darker(Color c, float f) {
    return Color{static_cast<unsigned char>(c.r * f), static_cast<unsigned char>(c.g * f),
                 static_cast<unsigned char>(c.b * f), c.a};
}

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
    DrawSphereEx(Vector3{0.0f, -0.05f, 0.42f}, 0.14f, 8, 8, Mix(lines, white, 0.3f + 0.5f * pulse));

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
//   metal = the armour colour of this level, stripes = its glow colour
void DrawReaper(float flash, float pulse, float wings, Color metal, Color stripes) {
    const Color white{255, 255, 255, 255};
    const Color body  = Mix(metal, white, flash);
    const Color wing  = Mix(Darker(metal, 0.82f), white, flash);
    const Color back  = Mix(Darker(metal, 0.28f), white, flash);
    const Color red   = Mix(stripes, white, flash);   // the stripes and wing edges
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
            DrawSphereEx(Vector3{x, y, 0.08f}, 0.07f, 6, 6, glow);
        }
    }
}

// ---- The Drone ----------------------------------------------------------------------
// A small cyan crystal (an "octahedron": two pyramids glued together) that spins,
// with a ring of light around it. Spinning makes each face catch the lamp in turn,
// so it sparkles.
//   flash 0..1, spin = rotation angle in degrees
//   color = the crystal's colour in this level (the ring always stays white-blue)
void DrawDrone(float flash, float spin, Color color) {
    const Color white{255, 255, 255, 255};
    const Color crystal = Mix(color, white, flash);
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
        // A crystal glows from inside, so it is NOT shaded by the lamp like armour.
        // Instead every face has its own fixed brightness: bright and normal
        // alternating on top, normal and darker underneath. As it spins, the
        // faces take turns facing us, and the crystal sparkles.
        const bool even = (i % 2 == 0);
        const Color upper = even ? Mix(crystal, white, 0.45f) : crystal;
        const Color lower = even ? crystal : Darker(crystal, 0.7f);
        DrawTriangle3D(top, a, b, upper);    DrawTriangle3D(top, b, a, upper);      // both sides
        DrawTriangle3D(bottom, a, b, lower); DrawTriangle3D(bottom, b, a, lower);
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

// ---- Helpers for the insects -----------------------------------------------------------

// A round body made of rings, like a vase turned on a lathe: "profile" lists
// (height y, radius) from top to bottom. Every band between two rings is made of
// flat quads, so it gets the faceted low-poly look. Bands take their colour from
// "colors" in turn: two colours give stripes.
struct Ring {
    float y;
    float radius;
};
template <std::size_t N>
void DrawLathe(const Ring (&profile)[N], int sides, const Color* colors, int colorCount) {
    for (std::size_t band = 0; band + 1 < N; ++band) {
        const Color color = colors[band % static_cast<std::size_t>(colorCount)];
        const Ring top = profile[band];
        const Ring bottom = profile[band + 1];
        for (int k = 0; k < sides; ++k) {
            const float a0 = 2.0f * PI * static_cast<float>(k) / static_cast<float>(sides);
            const float a1 = 2.0f * PI * static_cast<float>(k + 1) / static_cast<float>(sides);
            const Vector3 p0{top.radius * std::cos(a0), top.y, top.radius * std::sin(a0)};
            const Vector3 p1{top.radius * std::cos(a1), top.y, top.radius * std::sin(a1)};
            const Vector3 p2{bottom.radius * std::cos(a1), bottom.y, bottom.radius * std::sin(a1)};
            const Vector3 p3{bottom.radius * std::cos(a0), bottom.y, bottom.radius * std::sin(a0)};
            // Outward = away from the middle line of the body, at this face's height.
            const Vector3 mid = Vector3Scale(Vector3Add(Vector3Add(p0, p1), Vector3Add(p2, p3)), 0.25f);
            DrawQuadFace(p0, p1, p2, p3, color, Vector3{mid.x, 0.0f, mid.z});
        }
    }
}

// A flat wing: a "fan" of triangles from the first point to all the others, seen
// from both sides (front and back), with a glowing outline.
template <std::size_t N>
void DrawWing(const Vector3 (&points)[N], Color front, Color back, Color outline) {
    for (std::size_t i = 1; i + 1 < N; ++i) {
        DrawFace(points[0], points[i], points[i + 1], front, Vector3{0.0f, 0.0f, 1.0f});
        DrawFace(points[0], points[i], points[i + 1], back, Vector3{0.0f, 0.0f, -1.0f});
    }
    for (std::size_t i = 0; i < N; ++i) {
        DrawLine3D(points[i], points[(i + 1) % N], outline);
    }
}

// ---- The Wasp -----------------------------------------------------------------------
// A striped insect hanging head-up: a small head with glowing eyes, a thorax, and a
// long striped abdomen ending in a sting that points at the player. Two pairs of
// glassy wings buzz up and down.
//   flash 0..1, beat -1..1 (the wing position), body / glow = the level's colours
void DrawWasp(float flash, float beat, Color body, Color glow) {
    const Color white{255, 255, 255, 255};
    const Color dark   = Mix(Darker(body, 0.45f), white, flash);
    const Color shell  = Mix(body, white, flash);
    const Color stripe = Mix(glow, white, flash);

    // Head, thorax and abdomen, top to bottom. The abdomen bands take turns:
    // glow colour, dark, glow colour, ... and end in a point (the sting).
    const Ring head[] = {{0.62f, 0.0f}, {0.56f, 0.14f}, {0.44f, 0.17f}, {0.34f, 0.0f}};
    const Color headColors[] = {shell};
    DrawLathe(head, 6, headColors, 1);
    const Ring thorax[] = {{0.36f, 0.0f}, {0.30f, 0.20f}, {0.12f, 0.22f}, {0.02f, 0.0f}};
    const Color thoraxColors[] = {dark};
    DrawLathe(thorax, 6, thoraxColors, 1);
    const Ring abdomen[] = {{0.04f, 0.10f}, {-0.10f, 0.24f}, {-0.28f, 0.27f}, {-0.46f, 0.24f},
                            {-0.62f, 0.18f}, {-0.76f, 0.10f}, {-0.98f, 0.0f}};
    const Color abdomenColors[] = {stripe, dark};
    DrawLathe(abdomen, 6, abdomenColors, 2);

    // Two glowing eyes on the head, and two antennae.
    for (const float side : {-1.0f, 1.0f}) {
        DrawSphereEx(Vector3{side * 0.09f, 0.52f, 0.12f}, 0.075f, 6, 6, Mix(glow, white, 0.4f));
        DrawLine3D(Vector3{side * 0.05f, 0.60f, 0.08f}, Vector3{side * 0.16f, 0.80f, 0.10f}, stripe);
        DrawLine3D(Vector3{side * 0.16f, 0.80f, 0.10f}, Vector3{side * 0.30f, 0.86f, 0.10f}, stripe);
    }

    // The wings: see-through, rooted at the thorax, flicking up and down.
    const Color wingFront = Fade(Mix(Mix(glow, white, 0.55f), white, flash), 0.55f);
    const Color wingBack  = Fade(Mix(body, white, 0.4f), 0.45f);
    for (const float side : {-1.0f, 1.0f}) {
        rlPushMatrix();
        rlTranslatef(side * 0.12f, 0.24f, 0.0f);
        rlRotatef(side * (10.0f + 28.0f * beat), 0.0f, 0.0f, 1.0f);   // up and down
        // The big front wing, and a smaller one under it.
        const Vector3 big[] = {{0.0f, 0.0f, 0.0f}, {side * 0.35f, 0.24f, 0.0f}, {side * 0.88f, 0.22f, 0.0f},
                               {side * 1.02f, 0.05f, 0.0f}, {side * 0.80f, -0.08f, 0.0f}};
        DrawWing(big, wingFront, wingBack, stripe);
        const Vector3 small[] = {{0.0f, -0.04f, -0.02f}, {side * 0.62f, -0.12f, -0.02f},
                                 {side * 0.66f, -0.26f, -0.02f}, {side * 0.30f, -0.24f, -0.02f}};
        DrawWing(small, wingFront, wingBack, Fade(stripe, 0.6f));
        rlPopMatrix();
    }
}

// ---- The Moth -----------------------------------------------------------------------
// A big furry insect with wide wings. Each upper wing has a glowing "eye-spot" (a
// ring with a bright centre) to scare enemies, like real moths. Feathery antennae.
//   flash 0..1, beat -1..1 (slow wing beat), body / glow = the level's colours
void DrawMoth(float flash, float beat, Color body, Color glow) {
    const Color white{255, 255, 255, 255};
    const Color fur   = Mix(Mix(body, white, 0.25f), white, flash);
    const Color furB  = Mix(Darker(body, 0.7f), white, flash);
    const Color spot  = Mix(glow, white, flash);

    // The body: a fat furry spindle, its bands alternating light and dark.
    const Ring profile[] = {{0.50f, 0.0f}, {0.44f, 0.16f}, {0.28f, 0.24f}, {0.08f, 0.27f},
                            {-0.14f, 0.25f}, {-0.36f, 0.19f}, {-0.56f, 0.10f}, {-0.68f, 0.0f}};
    const Color bodyColors[] = {fur, furB};
    DrawLathe(profile, 7, bodyColors, 2);
    for (const float side : {-1.0f, 1.0f}) {
        DrawSphereEx(Vector3{side * 0.10f, 0.40f, 0.12f}, 0.08f, 6, 6, Mix(spot, white, 0.3f));   // eyes
        // Feathery antennae: a curved line with little side hairs.
        const Vector3 a{side * 0.06f, 0.48f, 0.05f};
        const Vector3 b{side * 0.30f, 0.82f, 0.05f};
        const Vector3 c{side * 0.52f, 0.92f, 0.05f};
        DrawLine3D(a, b, fur);
        DrawLine3D(b, c, fur);
        for (int i = 1; i <= 3; ++i) {
            const Vector3 p = Vector3Lerp(a, b, 0.3f * static_cast<float>(i));
            DrawLine3D(p, Vector3{p.x + side * 0.07f, p.y - 0.05f, p.z}, Fade(fur, 0.8f));
        }
    }

    // The wings, beating slowly around the shoulders.
    const Color wingFront = Mix(Mix(body, white, 0.30f), white, flash);
    const Color wingBack  = Mix(Darker(body, 0.5f), white, flash);
    for (const float side : {-1.0f, 1.0f}) {
        rlPushMatrix();
        rlTranslatef(side * 0.16f, 0.18f, 0.0f);
        rlRotatef(side * (8.0f + 16.0f * beat), 0.0f, 0.0f, 1.0f);
        // Upper wing: broad, reaching up and out; lower wing: rounder, below it.
        const Vector3 upper[] = {{0.0f, 0.0f, 0.0f}, {side * 0.30f, 0.52f, 0.0f}, {side * 0.92f, 0.70f, 0.0f},
                                 {side * 1.28f, 0.42f, 0.0f}, {side * 1.10f, 0.02f, 0.0f}, {side * 0.50f, -0.16f, 0.0f}};
        DrawWing(upper, wingFront, wingBack, Fade(spot, 0.7f));
        const Vector3 lower[] = {{0.0f, -0.12f, -0.02f}, {side * 0.58f, -0.20f, -0.02f},
                                 {side * 0.86f, -0.52f, -0.02f}, {side * 0.58f, -0.80f, -0.02f},
                                 {side * 0.18f, -0.60f, -0.02f}};
        DrawWing(lower, Mix(wingFront, wingBack, 0.35f), wingBack, Fade(spot, 0.5f));
        // The eye-spot on the upper wing: a glowing ring with a bright centre.
        const Vector3 eye{side * 0.78f, 0.36f, 0.03f};
        DrawCircle3D(eye, 0.17f, Vector3{0.0f, 1.0f, 0.0f}, 0.0f, spot);
        DrawCircle3D(eye, 0.12f, Vector3{0.0f, 1.0f, 0.0f}, 0.0f, Fade(spot, 0.6f));
        DrawSphereEx(eye, 0.08f, 6, 6, Mix(spot, white, 0.5f));
        rlPopMatrix();
    }
}

// ---- The Mite -------------------------------------------------------------------------
// A small armoured crab, the little cousin of the crab bosses: a faceted shell with a
// big glowing eye and two small ones, two legs on each side, and two tiny scissor
// claws that point down at the player and snap open and shut.
//   flash 0..1, open 0..1 (how far the scissors are open), body / glow = level colours
void DrawMite(float flash, float open, Color body, Color glow) {
    const Color white{255, 255, 255, 255};
    const Color shell = Mix(body, white, flash);
    const Color dark  = Mix(Darker(body, 0.45f), white, flash);
    const Color steel = Mix(Mix(body, Color{170, 180, 195, 255}, 0.55f), white, flash);
    const Color eye   = Mix(glow, white, 0.25f);

    // The shell: six points around, a front centre bulging toward the player and a
    // back centre, so every face gets its own shade.
    const Vector3 front{0.0f, 0.05f, 0.36f};
    const Vector3 back{0.0f, 0.05f, -0.25f};
    Vector3 rim[6];
    for (int i = 0; i < 6; ++i) {
        const float a = (90.0f + 60.0f * static_cast<float>(i)) * DEG2RAD;
        rim[i] = Vector3{0.58f * std::cos(a), 0.05f + 0.46f * std::sin(a), 0.0f};
    }
    for (int i = 0; i < 6; ++i) {
        const Vector3 a = rim[i], b = rim[(i + 1) % 6];
        DrawFace(front, a, b, (i % 2 == 0) ? shell : Mix(shell, dark, 0.3f), lowpoly::Mid(front, a, b));
        DrawFace(back, a, b, dark, lowpoly::Mid(back, a, b));
        DrawLine3D(a, b, Fade(glow, 0.5f));
    }

    // Legs: two on each side, bent at a knee.
    for (const float s : {-1.0f, 1.0f}) {
        for (const float y : {0.18f, -0.12f}) {
            const Vector3 root{s * 0.45f, y, 0.0f};
            const Vector3 knee{s * 0.88f, y + 0.22f, -0.05f};
            const Vector3 foot{s * 1.02f, y - 0.25f, -0.05f};
            DrawCylinderEx(root, knee, 0.07f, 0.06f, 5, dark);
            DrawCylinderEx(knee, foot, 0.06f, 0.0f, 5, dark);
        }
    }

    // The scissor claws: a short arm, then two thin blades that open and close.
    for (const float s : {-1.0f, 1.0f}) {
        const Vector3 shoulder{s * 0.32f, -0.36f, 0.05f};
        const Vector3 wrist{s * 0.42f, -0.68f, 0.1f};
        DrawCylinderEx(shoulder, wrist, 0.09f, 0.08f, 5, steel);
        const float spread = 0.08f + 0.18f * open;
        const Vector3 tipOut{s * (0.30f + spread), -1.22f, 0.1f};
        const Vector3 tipIn{s * (0.30f - spread), -1.22f, 0.1f};
        for (const Vector3& tip : {tipOut, tipIn}) {
            const Vector3 a{wrist.x - 0.06f, wrist.y, wrist.z};
            const Vector3 b{wrist.x + 0.06f, wrist.y, wrist.z};
            DrawTriangle3D(a, b, tip, steel);
            DrawTriangle3D(b, a, tip, dark);   // the back side
            DrawLine3D(wrist, tip, glow);      // a glowing cutting edge
        }
    }

    // The eyes: a big one in the middle, two small ones above it.
    DrawSphereEx(Vector3{0.0f, 0.02f, 0.40f}, 0.15f, 8, 8, Mix(eye, white, 0.3f));
    DrawSphereEx(Vector3{-0.22f, 0.26f, 0.30f}, 0.07f, 6, 6, eye);
    DrawSphereEx(Vector3{0.22f, 0.26f, 0.30f}, 0.07f, 6, 6, eye);
}

}  // namespace

LevelAlienColors AlienColorsForLevel(int level) {
    return kLevelAlienColors[std::clamp(level, 0, config::kLevelCount - 1)];
}

Invader::Invader(Vector3 start, AlienType alienType)
    : position(start),
      type(alienType),
      hitPoints(alienType == AlienType::Commander ? config::kCommanderHitPoints
                : alienType == AlienType::Moth   ? config::kMothHitPoints
                                                 : config::kAlienHitPoints) {}

void Invader::Update(float dt) {
    if (flash > 0.0f) {
        flash -= dt;
    }
}

float Invader::Radius() const {
    switch (type) {
        case AlienType::Commander: return config::kCommanderRadius * config::kAlienScale;
        case AlienType::Drone:     return config::kDroneRadius * config::kAlienScale;
        case AlienType::Wasp:      return config::kWaspRadius * config::kAlienScale;
        case AlienType::Moth:      return config::kMothRadius * config::kAlienScale;
        case AlienType::Mite:      return config::kMiteRadius * config::kAlienScale;
        default:                   return config::kInvaderRadius * config::kAlienScale;
    }
}

void Invader::Draw(float time, const Model* commanderModel, int level) const {
    const LevelAlienColors palette = AlienColorsForLevel(level);
    // Bob up and down. Adding position.x to the angle makes neighbours move
    // out of step, so the wave looks like it is wobbling, not like a block.
    const float bob = 0.12f * std::sin(time * 4.0f + position.x);
    const float flashAmount = flash > 0.0f ? flash / config::kHitFlashTime : 0.0f;   // 1 .. 0

    // Commanders hover a little higher than the rest, so you can see them over
    // the rows in front. (Only the picture moves; the hit area stays where it is.)
    const float hover = (type == AlienType::Commander) ? config::kCommanderHover : 0.0f;

    // Aliens keep more light on their shadow side, so they stand out from the dark
    // background (back to normal when this function ends, see ShadowLightScope).
    const lowpoly::ShadowLightScope brightShadows(config::kAlienShadowLight);

    rlPushMatrix();
    rlTranslatef(position.x, position.y + bob + hover, position.z);
    rlRotatef(roll, 0.0f, 0.0f, 1.0f);   // divers tilt into their curve
    // Turn the face up toward the camera: a turn around the X axis (left-right).
    // A negative angle lifts the front (+Z) side up.
    rlRotatef(-config::kAlienFaceTilt, 1.0f, 0.0f, 0.0f);
    rlScalef(config::kAlienScale, config::kAlienScale, config::kAlienScale);

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
        // The armour: the level's body colour mixed with grey metal, so it still looks armoured.
        DrawReaper(flashAmount, pulse, wings, Mix(palette.body, Color{150, 150, 168, 255}, 0.45f), palette.glow);
    } else if (type == AlienType::Wasp) {
        // Wings beat fast: 22 times a second, a little out of step between wasps.
        rlScalef(config::kWaspScale, config::kWaspScale, config::kWaspScale);
        DrawWasp(flashAmount, std::sin(time * 44.0f + position.x * 1.7f), palette.body, palette.glow);
    } else if (type == AlienType::Moth) {
        // Big wings beat slowly.
        rlScalef(config::kMothScale, config::kMothScale, config::kMothScale);
        DrawMoth(flashAmount, std::sin(time * 4.0f + position.x * 0.6f), palette.body, palette.glow);
    } else if (type == AlienType::Mite) {
        // Its little scissors snap open and shut.
        DrawMite(flashAmount, 0.5f + 0.5f * std::sin(time * 6.0f + position.x), palette.body, palette.glow);
    } else if (type == AlienType::Drone) {
        rlScalef(config::kDroneScale, config::kDroneScale, config::kDroneScale);
        DrawDrone(flashAmount, time * 90.0f + position.x * 20.0f,   // 90 degrees per second
                  palette.glow);   // the crystal glows in the level's bright colour
    } else {
        // The Scarab takes the level's colours; the commander keeps its gold.
        const ScarabStyle levelScarab{palette.body, Darker(palette.body, 0.5f),
                                      Darker(palette.body, 0.85f), palette.glow, kScarabStyle.scale};
        const ScarabStyle& style =
            (type == AlienType::Commander) ? kCommanderFallbackStyle : levelScarab;
        rlScalef(style.scale, style.scale, style.scale);

        const float pulse = 0.5f + 0.5f * std::sin(time * 6.0f + position.x * 0.7f);
        const float claws = 8.0f + 10.0f * std::sin(time * 3.0f + position.x);   // degrees
        DrawScarab(style, flashAmount, pulse, claws);
    }

    rlPopMatrix();
}
