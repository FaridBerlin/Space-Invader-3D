// Player.cpp - how the ship moves and how it looks.

#include "Player.h"

#include <algorithm>   // std::min, std::max
#include <cmath>       // std::exp, std::sin

#include "Config.h"
#include "raylib.h"
#include "raymath.h"   // Vector3 maths: cross product, normalize, ...
#include "rlgl.h"  // raylib's low-level matrix functions (rlPushMatrix, ...)

// ---- Private helpers, visible only inside this file --------------------------
// An anonymous namespace makes everything in it local to this .cpp file.
namespace {

// The ship has no real lighting, so we fake it: every flat face is darkened or
// brightened depending on how much it points toward an imaginary lamp. That one
// trick turns plain triangles into a shape with visible volume ("flat shading").
//
//   a, b, c  the three corners of the triangle
//   base     the colour of the face when it is lit "normally"
//   outward  a direction that points AWAY from the ship's inside. A triangle has
//            two sides; this tells us which one is the outside (see below).
void DrawFace(Vector3 a, Vector3 b, Vector3 c, Color base, Vector3 outward) {
    // The "normal" is the direction the face points to: the cross product of
    // two of its edges is perpendicular to the face.
    Vector3 normal = Vector3Normalize(
        Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)));
    if (Vector3DotProduct(normal, outward) < 0.0f) {
        normal = Vector3Negate(normal);   // it pointed inside: turn it around
    }

    // The lamp: above, a bit to the left, and behind the ship (where our camera is).
    const Vector3 lamp = Vector3Normalize(Vector3{-0.35f, 0.85f, 0.45f});
    const float lit = std::max(0.0f, Vector3DotProduct(normal, lamp));   // 0 .. 1
    const float k = 0.35f + 0.75f * lit;   // 0.35 (in shadow) .. 1.10 (facing the lamp)

    auto shade = [k](unsigned char channel) {
        return static_cast<unsigned char>(std::min(255.0f, channel * k));
    };
    const Color shaded{shade(base.r), shade(base.g), shade(base.b), 255};

    // raylib only draws a triangle from one side. We draw it from both, so we
    // never have to worry about the order of the corners.
    DrawTriangle3D(a, b, c, shaded);
    DrawTriangle3D(a, c, b, shaded);
}

// A four-cornered face, as two triangles. Corners must go around the edge in order.
void DrawQuadFace(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color base, Vector3 outward) {
    DrawFace(a, b, c, base, outward);
    DrawFace(a, c, d, base, outward);
}

// The middle of a triangle or quad, used as the "outward" hint for the hull,
// because the hull is built around the ship's centre (the origin).
Vector3 Mid(Vector3 a, Vector3 b, Vector3 c) {
    return Vector3Scale(Vector3Add(Vector3Add(a, b), c), 1.0f / 3.0f);
}

}  // namespace

Player::Player() = default;   // all members already have starting values in Player.h

void Player::Update(float dt, const PlayerInput& input) {
    time_ += dt;

    // 1. What speed do we WANT? input.move is -1..+1, so this is -speed..+speed.
    const float targetVelocity = input.move * config::kPlayerSpeed;

    // 2. Move the real velocity toward the wanted one, smoothly, so the ship
    //    accelerates and brakes instead of jerking.
    //    "1 - exp(-k * dt)" is the share of the remaining gap we close this frame.
    //    Unlike a fixed factor such as 0.2, it gives the SAME feel at 30, 60 or
    //    144 FPS: another place where delta time keeps the game fair.
    const float blend = 1.0f - std::exp(-config::kPlayerResponsiveness * dt);
    velocityX_ += (targetVelocity - velocityX_) * blend;

    // 3. Move: distance = speed * time.
    x_ += velocityX_ * dt;

    // 4. Stay inside the play field. If we hit the wall, stop dead there.
    const float limit = config::kFieldHalfWidth - config::kPlayerHalfWidth;
    if (x_ > limit) {
        x_ = limit;
        velocityX_ = 0.0f;
    } else if (x_ < -limit) {
        x_ = -limit;
        velocityX_ = 0.0f;
    }
}

Vector3 Player::Position() const {
    return Vector3{x_, config::kPlayerHeight, 0.0f};
}

void Player::Draw() const {
    // Banking: the faster we slide sideways, the more the ship rolls.
    // The ship flies toward -Z, so "rolling" means rotating around the Z axis.
    // Positive angles tip the right wing UP when seen from behind, so we use the
    // negative sign to dip the wing on the side we are turning toward.
    const float bankDegrees =
        -(velocityX_ / config::kPlayerSpeed) * config::kPlayerMaxBankDegrees;

    // ---- Transform stack ---------------------------------------------------------
    // Instead of calculating the position of every part by hand, we MOVE and
    // ROTATE the whole coordinate system, then draw the ship around the origin.
    //   rlPushMatrix  = remember the current coordinate system
    //   rlTranslatef  = move the origin to the ship's position
    //   rlRotatef     = roll the whole system around the Z axis
    //   rlPopMatrix   = go back, so other objects are not affected
    // Every Draw call in between happens in the ship's own "local" space.
    rlPushMatrix();
    rlTranslatef(x_, config::kPlayerHeight, 0.0f);
    rlRotatef(bankDegrees, 0.0f, 0.0f, 1.0f);

    // Palette: a metallic blue-grey body, darker wings, lime-green trim, glowing cyan.
    const Color hullColor{190, 205, 230, 255};
    const Color wingColor{125, 155, 225, 255};
    const Color trimColor{170, 255,  70, 255};   // lime green: eye-catching neon edges
    const Color darkColor{ 48,  56,  80, 255};

    // Local space: the nose points to -Z, the tail to +Z, Y is up, X is left/right.

    // ---- Hull: three rings of points (nose, middle, tail) joined by triangles ------
    const Vector3 nose{0.0f, 0.0f, -2.7f};
    const Vector3 midL{-0.55f, 0.0f, -0.2f}, midR{0.55f, 0.0f, -0.2f};
    const Vector3 midT{0.0f, 0.50f, -0.4f},  midB{0.0f, -0.35f, -0.2f};
    const Vector3 tailL{-0.6f, 0.0f, 1.1f},  tailR{0.6f, 0.0f, 1.1f};
    const Vector3 tailT{0.0f, 0.45f, 1.1f},  tailB{0.0f, -0.35f, 1.1f};

    auto hullFace = [&](Vector3 a, Vector3 b, Vector3 c) {
        DrawFace(a, b, c, hullColor, Mid(a, b, c));
    };
    hullFace(nose, midT, midL);   hullFace(nose, midR, midT);    // nose, upper half
    hullFace(nose, midL, midB);   hullFace(nose, midB, midR);    // nose, lower half
    DrawQuadFace(midT, midL, tailL, tailT, hullColor, Mid(midT, midL, tailL));   // body top-left
    DrawQuadFace(midT, midR, tailR, tailT, hullColor, Mid(midT, midR, tailR));   // body top-right
    DrawQuadFace(midB, midL, tailL, tailB, hullColor, Mid(midB, midL, tailL));   // body bottom-left
    DrawQuadFace(midB, midR, tailR, tailB, hullColor, Mid(midB, midR, tailR));   // body bottom-right
    DrawQuadFace(tailT, tailL, tailB, tailR, darkColor, Vector3{0.0f, 0.0f, 1.0f});   // rear plate

    // Glowing lime lines along the top ridge and the sides: the "neon outline".
    DrawLine3D(nose, midT, trimColor);
    DrawLine3D(midT, tailT, trimColor);
    DrawLine3D(nose, midL, trimColor);   DrawLine3D(midL, tailL, trimColor);
    DrawLine3D(nose, midR, trimColor);   DrawLine3D(midR, tailR, trimColor);

    // ---- Wings (mirrored: sign = -1 is the left wing, +1 the right) ----------------
    for (const float sign : {-1.0f, 1.0f}) {
        const Vector3 rootFront{sign * 0.55f, 0.02f, -0.9f};
        const Vector3 tip      {sign * 1.45f, 0.14f,  0.85f};
        const Vector3 tipRear  {sign * 1.15f, 0.12f,  1.3f};
        const Vector3 rootRear {sign * 0.60f, 0.00f,  1.1f};
        const Vector3 down{0.0f, -0.08f, 0.0f};   // the underside sits a little lower

        DrawQuadFace(rootFront, tip, tipRear, rootRear, wingColor, Vector3{0.0f, 1.0f, 0.0f});
        DrawQuadFace(Vector3Add(rootFront, down), Vector3Add(tip, down),
                     Vector3Add(tipRear, down),   Vector3Add(rootRear, down),
                     darkColor, Vector3{0.0f, -1.0f, 0.0f});

        // Neon trim on the front edge and the wing tip.
        const Vector3 lift{0.0f, 0.02f, 0.0f};
        DrawLine3D(Vector3Add(rootFront, lift), Vector3Add(tip, lift), trimColor);
        DrawLine3D(Vector3Add(tip, lift), Vector3Add(tipRear, lift), trimColor);

        // A small upright fin on the wing tip.
        DrawFace(tip, Vector3{sign * 1.45f, 0.75f, 1.3f}, Vector3{sign * 1.3f, 0.12f, 1.3f},
                 trimColor, Vector3{-1.0f, 0.0f, 0.0f});

        // Cannon on the wing: a dark barrel with a glowing cyan muzzle.
        DrawCylinderEx(Vector3{sign * 0.95f, 0.18f, -1.0f}, Vector3{sign * 0.95f, 0.18f, 0.6f},
                       0.1f, 0.1f, 8, darkColor);
        DrawSphereEx(Vector3{sign * 0.95f, 0.18f, -1.0f}, 0.1f, 8, 8, Color{120, 230, 255, 255});
    }

    // ---- Cockpit: a squashed glass sphere with a small shine on it -----------------
    rlPushMatrix();
    rlTranslatef(0.0f, 0.38f, -0.35f);
    rlScalef(0.45f, 0.4f, 1.0f);   // squash the ball into an egg shape (x, y, z factors)
    DrawSphereEx(Vector3{0.0f, 0.0f, 0.0f}, 0.5f, 14, 14, Color{60, 190, 255, 255});
    rlPopMatrix();
    DrawSphereEx(Vector3{-0.07f, 0.56f, -0.62f}, 0.07f, 6, 6, Color{235, 250, 255, 255});

    // ---- Two engines with a flickering blue-white flame ---------------------------
    for (const float sign : {-1.0f, 1.0f}) {
        const float x = sign * 0.32f;
        DrawCylinderEx(Vector3{x, -0.05f, 0.85f}, Vector3{x, -0.05f, 1.35f}, 0.22f, 0.26f, 10, darkColor);

        // A different phase per engine, so the two flames do not flicker in step.
        const float flameLength = 0.9f + 0.25f * std::sin(time_ * 40.0f + sign * 1.7f);
        const Vector3 start{x, -0.05f, 1.35f};
        DrawCylinderEx(start, Vector3{x, -0.05f, 1.35f + flameLength}, 0.2f, 0.0f, 10,
                       Color{60, 160, 255, 255});                                  // outer: blue
        DrawCylinderEx(start, Vector3{x, -0.05f, 1.35f + flameLength * 0.6f}, 0.11f, 0.0f, 10,
                       Color{220, 250, 255, 255});                                 // core: white
    }

    rlPopMatrix();
}
