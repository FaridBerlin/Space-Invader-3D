// LowPoly.cpp

#include "LowPoly.h"

#include <algorithm>   // std::min, std::max, std::clamp
#include <array>
#include <cmath>       // std::sqrt, std::cos, std::sin, std::fabs

#include "raymath.h"   // Vector3 maths: cross product, normalize, ...
#include "rlgl.h"      // rlBegin / rlVertex3f: drawing our own triangles

namespace lowpoly {

namespace {
float gShadowLight = kDefaultShadowLight;   // used by DrawFace, see SetShadowLight
}  // namespace

void SetShadowLight(float light) { gShadowLight = light; }

ShadowLightScope::ShadowLightScope(float light) : previous_(gShadowLight) { gShadowLight = light; }
ShadowLightScope::~ShadowLightScope() { gShadowLight = previous_; }

void DrawFace(Vector3 a, Vector3 b, Vector3 c, Color base, Vector3 outward) {
    // The "normal" is the direction the face points to: the cross product of
    // two of its edges is perpendicular to the face.
    Vector3 normal = Vector3Normalize(
        Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)));
    if (Vector3DotProduct(normal, outward) < 0.0f) {
        normal = Vector3Negate(normal);   // it pointed inside: turn it around
    }

    // The lamp: above, a bit to the left, and behind the player (where our camera is).
    const Vector3 lamp = Vector3Normalize(Vector3{-0.35f, 0.85f, 0.45f});
    const float lit = std::max(0.0f, Vector3DotProduct(normal, lamp));   // 0 .. 1
    // From the shadow light (facing away) up to 1.10 (facing the lamp).
    const float k = gShadowLight + (1.10f - gShadowLight) * lit;

    auto shade = [k](unsigned char channel) {
        return static_cast<unsigned char>(std::min(255.0f, static_cast<float>(channel) * k));
    };
    const Color shaded{shade(base.r), shade(base.g), shade(base.b), 255};

    // raylib only draws a triangle from one side. We draw it from both, so we
    // never have to worry about the order of the corners.
    DrawTriangle3D(a, b, c, shaded);
    DrawTriangle3D(a, c, b, shaded);
}

void DrawQuadFace(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color base, Vector3 outward) {
    DrawFace(a, b, c, base, outward);
    DrawFace(a, c, d, base, outward);
}

Vector3 Mid(Vector3 a, Vector3 b, Vector3 c) {
    return Vector3Scale(Vector3Add(Vector3Add(a, b), c), 1.0f / 3.0f);
}

Color Mix(Color a, Color b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto mix = [t](unsigned char x, unsigned char y) {
        return static_cast<unsigned char>(static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t);
    };
    return Color{mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a)};
}


// ---- DrawOrb --------------------------------------------------------------------
namespace {

// An icosahedron: the 20-sided "dice" shape, the roundest shape made of equal
// triangles. Its 12 corners sit on a ball of radius 1.
struct Triangle {
    Vector3 a, b, c;
};

std::array<Triangle, 20> Icosahedron() {
    const float g = (1.0f + std::sqrt(5.0f)) / 2.0f;   // the golden ratio
    std::array<Vector3, 12> v = {{{-1, g, 0}, {1, g, 0}, {-1, -g, 0}, {1, -g, 0},
                                  {0, -1, g}, {0, 1, g}, {0, -1, -g}, {0, 1, -g},
                                  {g, 0, -1}, {g, 0, 1}, {-g, 0, -1}, {-g, 0, 1}}};
    for (Vector3& p : v) p = Vector3Normalize(p);
    // Which three corners make each of the 20 faces.
    constexpr int faces[20][3] = {{0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11},
                                  {1, 5, 9},  {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
                                  {3, 9, 4},  {3, 4, 2},  {3, 2, 6},   {3, 6, 8},  {3, 8, 9},
                                  {4, 9, 5},  {2, 4, 11}, {6, 2, 10},  {8, 6, 7},  {9, 8, 1}};
    std::array<Triangle, 20> result{};
    for (int i = 0; i < 20; ++i) {
        result[static_cast<std::size_t>(i)] = {v[static_cast<std::size_t>(faces[i][0])],
                                               v[static_cast<std::size_t>(faces[i][1])],
                                               v[static_cast<std::size_t>(faces[i][2])]};
    }
    return result;
}

// The smoother ball: every triangle split into 4, and the new corners pushed out
// onto the ball again (80 triangles).
std::array<Triangle, 80> Subdivided(const std::array<Triangle, 20>& coarse) {
    std::array<Triangle, 80> result{};
    std::size_t n = 0;
    for (const Triangle& t : coarse) {
        const Vector3 ab = Vector3Normalize(Vector3Add(t.a, t.b));
        const Vector3 bc = Vector3Normalize(Vector3Add(t.b, t.c));
        const Vector3 ca = Vector3Normalize(Vector3Add(t.c, t.a));
        result[n++] = {t.a, ab, ca};
        result[n++] = {ab, t.b, bc};
        result[n++] = {ca, bc, t.c};
        result[n++] = {ab, bc, ca};
    }
    return result;
}

// Worked out once, the first time they are needed ("static" keeps them).
const std::array<Triangle, 20>& SmallOrb() {
    static const std::array<Triangle, 20> shape = Icosahedron();
    return shape;
}
const std::array<Triangle, 80>& SmoothOrb() {
    static const std::array<Triangle, 80> shape = Subdivided(SmallOrb());
    return shape;
}

template <std::size_t N>
void DrawTriangles(const std::array<Triangle, N>& shape, Vector3 center, float radius, Color color) {
    rlCheckRenderBatchLimit(static_cast<int>(N * 3));   // room for all corners in the current batch
    rlBegin(RL_TRIANGLES);
    rlColor4ub(color.r, color.g, color.b, color.a);
    for (const Triangle& t : shape) {
        for (const Vector3& p : {t.a, t.b, t.c}) {
            rlVertex3f(center.x + p.x * radius, center.y + p.y * radius, center.z + p.z * radius);
        }
    }
    rlEnd();
}

}  // namespace

void DrawOrb(Vector3 center, float radius, Color color, bool smooth) {
    if (smooth) {
        DrawTriangles(SmoothOrb(), center, radius, color);
    } else {
        DrawTriangles(SmallOrb(), center, radius, color);
    }
}


void DrawTube(Vector3 a, Vector3 b, float radiusA, float radiusB, int sides, Color color) {
    const Vector3 along = Vector3Subtract(b, a);
    if (Vector3Length(along) < 0.0001f || sides < 3) return;
    // Two directions at right angles to the tube (and to each other): the circle
    // around the tube is built from them.
    const Vector3 dir = Vector3Normalize(along);
    const Vector3 helper = std::fabs(dir.y) < 0.9f ? Vector3{0.0f, 1.0f, 0.0f} : Vector3{1.0f, 0.0f, 0.0f};
    const Vector3 u = Vector3Normalize(Vector3CrossProduct(dir, helper));
    const Vector3 v = Vector3CrossProduct(dir, u);
    auto ring = [&](Vector3 center, float radius, int i) {
        const float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(sides);
        return Vector3Add(center, Vector3Add(Vector3Scale(u, std::cos(angle) * radius),
                                             Vector3Scale(v, std::sin(angle) * radius)));
    };
    rlCheckRenderBatchLimit(sides * 6);
    rlBegin(RL_TRIANGLES);
    rlColor4ub(color.r, color.g, color.b, color.a);
    for (int i = 0; i < sides; ++i) {
        // One side of the tube: a long four-cornered strip, made of two triangles.
        const Vector3 a1 = ring(a, radiusA, i), a2 = ring(a, radiusA, i + 1);
        const Vector3 b1 = ring(b, radiusB, i), b2 = ring(b, radiusB, i + 1);
        for (const Vector3& p : {a1, b1, a2, a2, b1, b2}) rlVertex3f(p.x, p.y, p.z);
    }
    rlEnd();
}

}  // namespace lowpoly
