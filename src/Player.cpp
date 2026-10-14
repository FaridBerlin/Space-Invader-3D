// Player.cpp - how the ship moves and how it looks.

#include "Player.h"

#include <algorithm>   // std::min, std::max, std::clamp
#include <cmath>       // std::exp, std::sin
#include <cstddef>     // std::size_t
#include <iterator>    // std::size

#include "Config.h"
#include "LowPoly.h"
#include "raylib.h"
#include "raymath.h"   // Vector3 maths: cross product, normalize, ...
#include "rlgl.h"  // raylib's low-level matrix functions (rlPushMatrix, ...)

// ---- Private helpers, visible only inside this file --------------------------
// An anonymous namespace makes everything in it local to this .cpp file.
namespace {

// The flat-shading helpers live in LowPoly.h; "using" lets us write DrawFace
// instead of lowpoly::DrawFace.
using lowpoly::DrawFace;
using lowpoly::DrawQuadFace;
using lowpoly::Mid;

// How one upgrade tier of the ship looks. Colours are {red, green, blue, alpha}.
struct ShipStyle {
    Color hull;
    Color wing;
    Color trim;          // the bright neon lines and fins
    Color canopy;        // the cockpit glass
    Color flameOuter;    // engine flame, outer part
    Color flameCore;     // engine flame, bright centre
    float flameLength;   // 1.0 = normal; bigger tiers have longer flames
};

// One entry per tier = per level. The ship starts blue-grey with lime trim and
// ends crimson and gold. The engine flames are orange, then red-orange, then red.
constexpr ShipStyle kStyles[] = {
    // hull               wing                trim                canopy              flame outer         flame core          length
    {{190, 205, 230, 255}, {125, 155, 225, 255}, {170, 255,  70, 255}, { 60, 190, 255, 255}, {255, 120,  20, 255}, {255, 230, 120, 255}, 1.00f},
    {{225, 230, 242, 255}, {150, 105, 225, 255}, { 90, 230, 255, 255}, {110, 240, 255, 255}, {255,  70,  20, 255}, {255, 200,  80, 255}, 1.15f},
    {{150, 152, 178, 255}, {215,  60,  85, 255}, {255, 210,  70, 255}, {255, 200,  80, 255}, {255,  40,  40, 255}, {255, 255, 200, 255}, 1.35f},
};
static_assert(std::size(kStyles) == static_cast<std::size_t>(config::kLevelCount),
              "one ship style per level");

}  // namespace

Player::Player() = default;   // all members already have starting values in Player.h

void Player::Update(float dt, const PlayerInput& input) {
    time_ += dt;

    // 1. What speed do we WANT? input.move is -1..+1, so this is -speed..+speed.
    //    With touch steering we want to reach input.targetX instead: the further
    //    away it is, the faster we fly there (a bit faster than with the keys,
    //    so the ship keeps up with the finger).
    float targetVelocity = input.move * config::kPlayerSpeed;
    if (input.targetX) {
        const float maxSpeed = config::kPlayerSpeed * config::kTouchSpeedFactor;
        targetVelocity = std::clamp((*input.targetX - x_) * config::kTouchFollowRate, -maxSpeed, maxSpeed);
    }

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
    const float limit = config::kFieldHalfWidth - HalfWidth();
    if (x_ > limit) {
        x_ = limit;
        velocityX_ = 0.0f;
    } else if (x_ < -limit) {
        x_ = -limit;
        velocityX_ = 0.0f;
    }
}

void Player::SetTier(int tier) {
    // Keep the tier inside the table, whatever the caller passes.
    tier_ = std::clamp(tier, 0, config::kLevelCount - 1);
}

float Player::HalfWidth() const {
    return config::kPlayerHalfWidth * config::kTierScale[tier_];
}

Vector3 Player::Position() const {
    return Vector3{x_, config::kPlayerHeight, 0.0f};
}

void Player::Draw() const {
    // Banking: the faster we slide sideways, the more the ship rolls.
    // The ship flies toward -Z, so "rolling" means rotating around the Z axis.
    // Positive angles tip the right wing UP when seen from behind, so we use the
    // negative sign to dip the wing on the side we are turning toward.
    // (std::clamp: with touch the ship can fly faster than kPlayerSpeed, but the
    // tilt should not go beyond the maximum.)
    const float bankDegrees =
        -std::clamp(velocityX_ / config::kPlayerSpeed, -1.0f, 1.0f) * config::kPlayerMaxBankDegrees;

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
    const float scale = config::kTierScale[tier_];
    rlScalef(scale, scale, scale);   // bigger ship at higher tiers (x, y, z factors)

    // The colours of the current tier (see the table at the top of this file).
    const ShipStyle& style = kStyles[tier_];
    const Color hullColor = style.hull;
    const Color wingColor = style.wing;
    const Color trimColor = style.trim;
    const Color darkColor{48, 56, 80, 255};

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

    // ---- Extra parts that come with the upgrades -----------------------------------
    if (tier_ >= 1) {
        // Small "canard" wings next to the nose.
        for (const float sign : {-1.0f, 1.0f}) {
            const Vector3 front{sign * 0.45f, 0.10f, -1.35f};
            const Vector3 out  {sign * 0.95f, 0.10f, -0.65f};
            const Vector3 back {sign * 0.50f, 0.10f, -0.55f};
            DrawFace(front, out, back, trimColor, Vector3{0.0f, 1.0f, 0.0f});
            DrawFace(Vector3Add(front, Vector3{0.0f, -0.06f, 0.0f}),
                     Vector3Add(out,   Vector3{0.0f, -0.06f, 0.0f}),
                     Vector3Add(back,  Vector3{0.0f, -0.06f, 0.0f}),
                     darkColor, Vector3{0.0f, -1.0f, 0.0f});
        }
    }
    if (tier_ >= 2) {
        // A tall tail fin on the back of the ship.
        DrawFace(Vector3{0.0f, 0.45f, 0.1f}, Vector3{0.0f, 1.15f, 1.15f}, Vector3{0.0f, 0.45f, 1.1f},
                 trimColor, Vector3{-1.0f, 0.0f, 0.0f});
        DrawLine3D(Vector3{0.0f, 0.45f, 0.1f}, Vector3{0.0f, 1.15f, 1.15f}, hullColor);
    }

    // ---- Cockpit: a squashed glass sphere with a small shine on it -----------------
    rlPushMatrix();
    rlTranslatef(0.0f, 0.38f, -0.35f);
    rlScalef(0.45f, 0.4f, 1.0f);   // squash the ball into an egg shape (x, y, z factors)
    DrawSphereEx(Vector3{0.0f, 0.0f, 0.0f}, 0.5f, 14, 14, style.canopy);
    rlPopMatrix();
    DrawSphereEx(Vector3{-0.07f, 0.56f, -0.62f}, 0.07f, 6, 6, Color{235, 250, 255, 255});

    // ---- Engines with a flickering flame (the top tier has a third, central one) --------
    const int   engineCount = (tier_ >= 2) ? 3 : 2;
    const float engineSpacing = (tier_ >= 2) ? 0.40f : 0.32f;
    for (int i = 0; i < engineCount; ++i) {
        // Two engines sit at -spacing and +spacing; three at -spacing, 0, +spacing.
        const float x = (engineCount == 2) ? (i == 0 ? -engineSpacing : engineSpacing)
                                           : (static_cast<float>(i) - 1.0f) * engineSpacing;
        DrawCylinderEx(Vector3{x, -0.05f, 0.85f}, Vector3{x, -0.05f, 1.35f}, 0.20f, 0.24f, 10, darkColor);

        // A different phase per engine, so the flames do not flicker in step.
        const float flameLength =
            (0.9f + 0.25f * std::sin(time_ * 40.0f + static_cast<float>(i) * 1.7f)) * style.flameLength;
        const Vector3 start{x, -0.05f, 1.35f};
        DrawCylinderEx(start, Vector3{x, -0.05f, 1.35f + flameLength}, 0.19f, 0.0f, 10,
                       style.flameOuter);                                  // outer flame
        DrawCylinderEx(start, Vector3{x, -0.05f, 1.35f + flameLength * 0.6f}, 0.10f, 0.0f, 10,
                       style.flameCore);                                   // bright core
    }

    rlPopMatrix();
}
