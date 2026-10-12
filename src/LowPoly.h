// LowPoly.h - helpers to build "low-poly" 3D shapes out of flat triangles.
//
// The game has no real lighting. Instead, every flat face is darkened or
// brightened depending on how much it points toward an imaginary lamp. That one
// trick ("flat shading") turns plain triangles into shapes with visible volume.
// The player's ship and the aliens are both built with these functions.

#pragma once

#include "raylib.h"

namespace lowpoly {

// Draws one triangle, shaded by the direction it faces.
//   a, b, c  the three corners
//   base     the colour of the face when it is lit "normally"
//   outward  a direction pointing AWAY from the inside of the shape. A triangle
//            has two sides; this tells us which one is the outside.
void DrawFace(Vector3 a, Vector3 b, Vector3 c, Color base, Vector3 outward);

// A four-cornered face, as two triangles. Corners must go around the edge in order.
void DrawQuadFace(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color base, Vector3 outward);

// The middle of a triangle. Handy as the "outward" hint for a shape built around
// the origin: the middle of a face points away from the centre.
Vector3 Mid(Vector3 a, Vector3 b, Vector3 c);

// Blends colour a toward colour b. t = 0 gives a, t = 1 gives b.
Color Mix(Color a, Color b, float t);

}  // namespace lowpoly
