// Random.h - random numbers for effects.

#pragma once

#include "raylib.h"
#include "raymath.h"   // raylib's vector maths: Vector3Add, Vector3Scale, ...

// "inline" lets us define a function in a header without the linker complaining
// that it exists in several .cpp files.

// A random decimal number from min to max.
inline float RandomFloat(float min, float max) {
    const float t = static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f;   // 0.0 .. 1.0
    return min + (max - min) * t;
}

// A random DIRECTION in 3D: a vector of length 1 pointing anywhere.
//
// Naive idea: pick random x, y, z in -1..1 and normalise. But that favours the
// corners of the cube (the diagonals are longer), so some directions would be
// more likely than others. The fix is "rejection sampling": throw away every
// point outside the unit sphere, and only normalise the ones inside.
inline Vector3 RandomDirection() {
    for (;;) {
        const Vector3 v{RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f)};
        const float lengthSquared = Vector3DotProduct(v, v);   // x*x + y*y + z*z
        if (lengthSquared > 0.0001f && lengthSquared <= 1.0f) {
            return Vector3Normalize(v);
        }
    }
}
