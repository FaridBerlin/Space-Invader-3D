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
using lowpoly::Mix;

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

// One entry per tier (which level gets which tier: kLevelShipTier in Config.h). The ship starts blue-grey with lime trim and
// ends crimson and gold. The engine flames are orange, then red-orange, then red.
constexpr ShipStyle kStyles[] = {
    // hull               wing                trim                canopy              flame outer         flame core          length
    {{190, 205, 230, 255}, {125, 155, 225, 255}, {170, 255,  70, 255}, { 60, 190, 255, 255}, {255, 120,  20, 255}, {255, 230, 120, 255}, 1.00f},
    {{225, 230, 242, 255}, {150, 105, 225, 255}, { 90, 230, 255, 255}, {110, 240, 255, 255}, {255,  70,  20, 255}, {255, 200,  80, 255}, 1.15f},
    {{150, 152, 178, 255}, {215,  60,  85, 255}, {255, 210,  70, 255}, {255, 200,  80, 255}, {255,  40,  40, 255}, {255, 255, 200, 255}, 1.35f},
};
static_assert(std::size(kStyles) == static_cast<std::size_t>(config::kTierCount),
              "one ship style per tier");

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
    tier_ = std::clamp(tier, 0, config::kTierCount - 1);
}

float Player::HalfWidth() const {
    const float base = hero_ ? config::kHeroHalfWidth : config::kPlayerHalfWidth;
    return base * config::kTierScale[tier_];
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

    if (hero_) {          // the Claw has its own function (below)
        DrawHero();
        rlPopMatrix();
        return;
    }

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

// ---- The Claw ----------------------------------------------------------------------
// A copper ship with a silver-edged hull, a gold crystal cockpit and two pairs of
// claw-shaped wings. Every part is a few flat triangles; the lamp shading in
// DrawFace gives them their light and dark sides.
void Player::DrawHero() const {
    const Color copper    {205, 100,  45, 255};
    const Color copperDark{125,  55,  30, 255};
    const Color silver    {205, 210, 220, 255};
    const Color navy      { 40,  45,  95, 255};
    const Color gold      {255, 195,  40, 255};
    const Color goldLight {255, 240, 150, 255};
    const Color underside { 60,  35,  30, 255};

    // The numbers below were measured on the concept picture. It is wider than it is
    // long, so we shrink it a little to keep it close to the first ship's size.
    rlPushMatrix();
    rlScalef(0.8f, 0.8f, 0.8f);

    // The outward hint for a face: from a point inside the part to the face's middle.
    auto face = [](Vector3 a, Vector3 b, Vector3 c, Color color, Vector3 inside) {
        DrawFace(a, b, c, color, Vector3Subtract(Mid(a, b, c), inside));
    };

    // ---- Hull ------------------------------------------------------------------------
    // The hull is a tube with a six-cornered cross-section, measured at three
    // "stations" along Z, closed by a pointed nose and a short tail spike.
    //       T            T = top ridge, U = upper edges (silver stripes between U and the
    //    U     U             sides L/R), B = bottom
    //   L       R
    //       B
    struct Ring { Vector3 L, UL, T, UR, R, B; };
    auto ring = [](float z, float w, float h, float d) {
        return Ring{{-w, 0.0f, z}, {-w * 0.55f, h * 0.72f, z}, {0.0f, h, z},
                    { w * 0.55f, h * 0.72f, z}, {w, 0.0f, z}, {0.0f, -d, z}};
    };
    const Ring rings[] = {ring(-1.7f, 0.45f, 0.45f, 0.25f),
                          ring(-0.6f, 0.62f, 0.60f, 0.32f),
                          ring( 0.6f, 0.42f, 0.45f, 0.28f)};
    const Vector3 nose{0.0f, 0.05f, -2.6f};
    const Vector3 tail{0.0f, -0.02f, 1.45f};
    const Vector3 hullCentre{0.0f, 0.1f, -0.6f};

    // One strip of the hull = 6 faces around the tube, each with its own colour.
    auto strip = [&](const Ring& a, const Ring& b) {
        DrawQuadFace(a.L,  a.UL, b.UL, b.L,  silver,     Vector3Subtract(Mid(a.L, a.UL, b.UL), hullCentre));
        DrawQuadFace(a.UL, a.T,  b.T,  b.UL, copper,     Vector3Subtract(Mid(a.UL, a.T, b.T), hullCentre));
        DrawQuadFace(a.T,  a.UR, b.UR, b.T,  copper,     Vector3Subtract(Mid(a.T, a.UR, b.UR), hullCentre));
        DrawQuadFace(a.UR, a.R,  b.R,  b.UR, silver,     Vector3Subtract(Mid(a.UR, a.R, b.R), hullCentre));
        DrawQuadFace(a.R,  a.B,  b.B,  b.R,  copperDark, Vector3Subtract(Mid(a.R, a.B, b.B), hullCentre));
        DrawQuadFace(a.B,  a.L,  b.L,  b.B,  copperDark, Vector3Subtract(Mid(a.B, a.L, b.L), hullCentre));
    };
    strip(rings[0], rings[1]);
    strip(rings[1], rings[2]);

    // A cone of triangles from a single point (nose or tail) to a ring.
    auto cap = [&](Vector3 tip, const Ring& r) {
        face(tip, r.L,  r.UL, silver,     hullCentre);
        face(tip, r.UL, r.T,  copper,     hullCentre);
        face(tip, r.T,  r.UR, copper,     hullCentre);
        face(tip, r.UR, r.R,  silver,     hullCentre);
        face(tip, r.R,  r.B,  copperDark, hullCentre);
        face(tip, r.B,  r.L,  copperDark, hullCentre);
    };
    cap(nose, rings[0]);
    cap(tail, rings[2]);

    // ---- Gold crystal cockpit on the front of the hull ---------------------------------
    {
        const Vector3 front{0.0f, 0.38f, -2.1f}, back{0.0f, 0.62f, -0.7f};
        const Vector3 left{-0.27f, 0.52f, -1.4f}, right{0.27f, 0.52f, -1.4f};
        const Vector3 top{0.0f, 0.88f, -1.35f};
        const Vector3 inside{0.0f, 0.45f, -1.35f};
        face(front, left, top, goldLight, inside);
        face(front, top, right, goldLight, inside);
        face(left, back, top, gold, inside);
        face(top, back, right, gold, inside);
    }

    // ---- Claws (mirrored: side = -1 left, +1 right) --------------------------------
    // Each claw is a folded plate: a raised crease runs from its gold front tip to
    // its sharp back point, so one half faces inward and the other outward.
    struct Claw {
        Vector3 root;       // front corner where it meets the body
        Vector3 tip;        // gold front tip
        Vector3 outer;      // outer shoulder
        Vector3 back;       // sharp back point
        Vector3 rootBack;   // back corner where it meets the body
        Vector3 crease;     // the raised fold
    };
    auto drawClaw = [&](const Claw& c) {
        const Vector3 inside = Mid(c.root, c.outer, c.rootBack);
        const Vector3 below{inside.x, inside.y - 1.0f, inside.z};
        face(c.root, c.tip, c.crease, copper, below);          // inner half
        face(c.root, c.crease, c.rootBack, copper, below);
        face(c.rootBack, c.crease, c.back, copper, below);
        const Color outerHalf = Mix(copper, copperDark, 0.45f);   // a darker, browner side
        face(c.tip, c.outer, c.crease, outerHalf, below);      // outer half
        face(c.crease, c.outer, c.back, outerHalf, below);
        // Flat dark underside.
        const Vector3 down{0.0f, -0.06f, 0.0f};
        const Vector3 uRoot = Vector3Add(c.root, down), uTip = Vector3Add(c.tip, down);
        const Vector3 uOuter = Vector3Add(c.outer, down), uBack = Vector3Add(c.back, down);
        const Vector3 uRootBack = Vector3Add(c.rootBack, down);
        const Vector3 under{0.0f, -1.0f, 0.0f};
        DrawFace(uRoot, uTip, uOuter, underside, under);
        DrawFace(uRoot, uOuter, uBack, underside, under);
        DrawFace(uRoot, uBack, uRootBack, underside, under);
        // The gold tip: the front 35% of the claw, painted on top of both halves.
        const Vector3 lift{0.0f, 0.012f, 0.0f};
        const Vector3 t  = Vector3Add(c.tip, lift);
        const Vector3 t1 = Vector3Add(Vector3Lerp(c.tip, c.root, 0.35f), lift);
        const Vector3 t2 = Vector3Add(Vector3Lerp(c.tip, c.crease, 0.35f), lift);
        const Vector3 t3 = Vector3Add(Vector3Lerp(c.tip, c.outer, 0.35f), lift);
        face(t, t1, t2, gold, below);
        face(t, t2, t3, gold, below);
        // From tier 2 on, the crease glows gold.
        if (tier_ >= 2) {
            DrawLine3D(Vector3Add(c.tip, lift), Vector3Add(c.crease, lift), gold);
            DrawLine3D(Vector3Add(c.crease, lift), Vector3Add(c.back, lift), gold);
        }
    };

    for (const float side : {-1.0f, 1.0f}) {
        auto P = [side](float x, float y, float z) { return Vector3{side * x, y, z}; };

        // Navy body between the hull and the claws (most of it hides under them).
        DrawQuadFace(P(0.35f, 0.10f, -1.1f), P(1.40f, 0.10f, -0.6f), P(1.40f, 0.10f, 0.6f),
                     P(0.35f, 0.10f, 0.8f), navy, Vector3{0.0f, 1.0f, 0.0f});

        // Inner claw: reaching forward beside the cockpit.
        drawClaw(Claw{P(0.50f, 0.15f, -1.0f), P(0.94f, 0.50f, -1.7f), P(1.45f, 0.10f, -0.95f),
                      P(0.72f, 0.05f, 0.80f), P(0.55f, 0.10f, 0.3f), P(1.00f, 0.60f, -0.6f)});
        // Outer claw: further out, sweeping back to a long sharp point.
        drawClaw(Claw{P(1.20f, 0.12f, -0.2f), P(1.68f, 0.42f, -0.8f), P(2.15f, 0.06f, -0.15f),
                      P(2.55f, 0.00f, 1.05f), P(1.35f, 0.06f, 0.75f), P(1.85f, 0.50f, 0.1f)});

        // Silver armour plate on the outside of the outer claw.
        DrawQuadFace(P(1.95f, 0.34f, -0.30f), P(2.38f, 0.20f, -0.25f), P(2.40f, 0.18f, 0.15f),
                     P(1.98f, 0.32f, 0.22f), silver, Vector3{side * 0.5f, 1.0f, 0.0f});
    }
    if (tier_ >= 2) {
        DrawLine3D(Vector3{0.0f, 0.46f, -1.7f}, Vector3{0.0f, 0.06f, -2.6f}, gold);   // the nose ridge
    }

    // ---- Two engines beside the tail spike, with copper-orange flames ----------------
    const Color flameOuter{255, 110, 30, 255};
    const Color flameCore {255, 230, 150, 255};
    const float flameScale = (tier_ >= 2) ? 1.35f : 1.15f;
    for (int i = 0; i < 2; ++i) {
        const float x = (i == 0) ? -0.25f : 0.25f;
        DrawCylinderEx(Vector3{x, 0.02f, 0.45f}, Vector3{x, 0.02f, 0.95f}, 0.17f, 0.21f, 10,
                       Mix(navy, underside, 0.5f));
        const float length = (0.9f + 0.25f * std::sin(time_ * 40.0f + static_cast<float>(i) * 1.7f)) * flameScale;
        const Vector3 start{x, 0.02f, 0.95f};
        DrawCylinderEx(start, Vector3{x, 0.02f, 0.95f + length}, 0.17f, 0.0f, 10, flameOuter);
        DrawCylinderEx(start, Vector3{x, 0.02f, 0.95f + length * 0.6f}, 0.09f, 0.0f, 10, flameCore);
    }

    rlPopMatrix();
}
