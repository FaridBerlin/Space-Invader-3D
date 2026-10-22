// LowPoly.cpp

#include "LowPoly.h"

#include <algorithm>   // std::min, std::max, std::clamp

#include "raymath.h"   // Vector3 maths: cross product, normalize, ...

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

}  // namespace lowpoly
