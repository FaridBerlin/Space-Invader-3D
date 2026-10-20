// Boss.cpp - how a boss moves, attacks, takes damage and looks.

#include "Boss.h"

#include <algorithm>   // std::min, std::max, std::clamp, std::remove_if
#include <cmath>       // std::sin, std::cos, std::fmod

#include "Config.h"
#include "GlowRenderer.h"
#include "Invader.h"   // AlienColorsForLevel
#include "LowPoly.h"
#include "Random.h"
#include "raymath.h"
#include "rlgl.h"

namespace {

using lowpoly::DrawFace;
using lowpoly::DrawQuadFace;
using lowpoly::Mid;
using lowpoly::Mix;

constexpr Color kWhite{255, 255, 255, 255};
constexpr Color kRed{255, 40, 40, 255};

Color Darker(Color c, float f) {
    return Color{static_cast<unsigned char>(static_cast<float>(c.r) * f),
                 static_cast<unsigned char>(static_cast<float>(c.g) * f),
                 static_cast<unsigned char>(static_cast<float>(c.b) * f), c.a};
}

float Smooth(float t) {   // 0..1 -> 0..1, slow at both ends
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// The local points the model and the collision use. The boss faces +Z (toward
// the player), Y is up. Everything is multiplied by the boss's scale.
constexpr Vector3 kCoreCenter{0.0f, 0.15f, 2.5f};
constexpr float   kCoreRadius = 0.85f;
constexpr Vector3 kEyes[2]    = {{-0.95f, 1.0f, 1.9f}, {0.95f, 1.0f, 1.9f}};
constexpr Vector3 kTurrets[2] = {{-4.6f, -0.25f, 1.7f}, {4.6f, -0.25f, 1.7f}};   // muzzle tips

// Each boss attacks in its own order, round and round.
// (Attack is private to Boss, so the table stores plain numbers and Boss
// converts them: 0 Fan, 1 Aimed, 2 Bombs, 3 Rain, 4 Spiral.)
constexpr int kPatternLength = 6;
constexpr int kPatterns[config::kBossCount][kPatternLength] = {
    {0, 1, 0, 2, 0, 1},   // Hive Queen: fans and aimed bursts, now and then a bomb
    {0, 1, 2, 3, 0, 1},   // War Titan: adds a rain of shots from its wing guns
    {0, 4, 1, 2, 3, 4},   // Void Emperor: adds a sweeping spiral
};
constexpr int kFanShots[config::kBossCount]   = {5, 7, 9};
constexpr int kAimedShots[config::kBossCount] = {3, 3, 5};
constexpr int kBombCount[config::kBossCount]  = {1, 2, 3};

}  // namespace

Boss::Boss(int level)
    : level_(level),
      index_(std::clamp(config::kLevelBoss[level], 0, config::kBossCount - 1)),
      scale_(config::kBossScale[index_]),
      hitPoints_(config::kBossHitPoints[index_]),
      maxHitPoints_(config::kBossHitPoints[index_]) {
    const LevelAlienColors palette = AlienColorsForLevel(level);
    body_ = palette.body;
    glow_ = palette.glow;
    position_ = Vector3{0.0f, config::kBossY, config::kBossEnterZ};
}

const char* Boss::Name() const { return config::kBossName[index_]; }
int   Boss::Points() const     { return config::kBossScore[index_]; }
float Boss::Health() const     { return static_cast<float>(std::max(hitPoints_, 0)) / static_cast<float>(maxHitPoints_); }
bool  Boss::Angry() const      { return hitPoints_ * 2 < maxHitPoints_; }

bool Boss::CoreOpen() const {
    if (state_ != State::Fighting) return false;
    // fmod = the remainder of a division: it counts 0 .. cycle, again and again.
    return std::fmod(stateTime_, config::kBossCoreCycle) > config::kBossCoreCycle - config::kBossCoreOpen;
}

Vector3 Boss::Mouth() const {
    return Vector3{0.0f, kCoreCenter.y, kCoreCenter.z + 1.2f};   // just in front of the core
}

BossEvents Boss::Update(float dt, Vector3 player, std::vector<InvaderShot>& shots, std::vector<Bomb>& bombs) {
    BossEvents events;
    time_      += dt;
    stateTime_ += dt;
    flash_ = std::max(0.0f, flash_ - dt);

    switch (state_) {
        case State::Entering: {
            // Glide in from far away and slow down at the end.
            const float t = Smooth(stateTime_ / config::kBossEnterTime);
            position_.z = config::kBossEnterZ + (config::kBossZ - config::kBossEnterZ) * t;
            position_.y = config::kBossY + 0.2f * std::sin(time_ * 1.5f);
            if (stateTime_ >= config::kBossEnterTime) {
                state_ = State::Fighting;
                stateTime_ = 0.0f;
                attackTimer_ = 1.0f;
            }
            break;
        }
        case State::Fighting: {
            // Slide from side to side. sin() swings between -1 and +1. We keep a
            // running "phase" instead of using the time directly, so it can speed
            // up when angry without jumping to a different spot.
            const float speed = config::kBossSwaySpeed[index_] * (Angry() ? 1.5f : 1.0f);
            sway_ += speed * dt;
            position_.x = config::kBossSway[index_] * std::sin(sway_);
            position_.y = config::kBossY + 0.2f * std::sin(time_ * 1.5f);
            roll_ = -10.0f * std::cos(sway_);   // lean into the turn

            // Next attack, when the last one has fired all its shots.
            attackTimer_ -= dt;
            if (attackTimer_ <= 0.0f && pending_.empty()) {
                StartAttack(player, bombs);
                attackTimer_ = config::kBossAttackPause[index_] * (Angry() ? config::kBossRageFactor : 1.0f);
            }

            // Fire the queued shots whose time has come.
            for (auto& p : pending_) {
                p.delay -= dt;
                if (p.delay <= 0.0f) {
                    // Shots fly at the height of the aliens, so they can hit the ship.
                    const Vector3 from = ToWorld(p.offset);
                    shots.emplace_back(Vector3{from.x, config::kInvaderHeight, from.z}, p.speed, p.sideSpeed);
                    events.shot = true;
                }
            }
            // Remove the fired ones (the erase-remove idiom, see Util.h).
            pending_.erase(std::remove_if(pending_.begin(), pending_.end(),
                                          [](const PendingShot& p) { return p.delay <= 0.0f; }),
                           pending_.end());
            break;
        }
        case State::Dying: {
            // Shake, tilt and sink, while blasts go off all over the body.
            const float t = stateTime_ / config::kBossDeathTime;
            roll_ = 25.0f * t * std::sin(time_ * 9.0f);
            position_.y = config::kBossY - 1.5f * t * t;
            position_.x += RandomFloat(-1.0f, 1.0f) * 6.0f * dt;
            blastTimer_ -= dt;
            if (blastTimer_ <= 0.0f) {
                events.blasts.push_back(Vector3{position_.x + RandomFloat(-5.0f, 5.0f) * scale_,
                                                position_.y + RandomFloat(-0.5f, 1.5f) * scale_,
                                                position_.z + RandomFloat(-2.0f, 2.5f) * scale_});
                blastTimer_ = 0.12f;
            }
            if (stateTime_ >= config::kBossDeathTime) {
                events.finished = true;
            }
            break;
        }
    }
    return events;
}

void Boss::StartAttack(Vector3 player, std::vector<Bomb>& bombs) {
    const auto attack = static_cast<Attack>(kPatterns[index_][attackStep_ % kPatternLength]);
    ++attackStep_;
    const int   extra = Angry() ? 2 : 0;
    const float speed = config::kBossShotSpeed[index_];
    const Vector3 mouth = Mouth();
    constexpr float kDeg = PI / 180.0f;

    switch (attack) {
        case Attack::Fan: {
            // n shots at once, spread over 70 degrees like the fingers of a hand.
            const int n = kFanShots[index_] + extra;
            for (int i = 0; i < n; ++i) {
                const float angle = (-35.0f + 70.0f * static_cast<float>(i) / static_cast<float>(n - 1)) * kDeg;
                pending_.push_back({0.0f, mouth, speed * std::sin(angle), speed * std::cos(angle)});
            }
            break;
        }
        case Attack::Aimed: {
            // A quick burst at the ship. Sideways speed = sideways distance / flight time.
            // (The ship can fly up and down, so the distance is measured to where it is now.)
            const int n = kAimedShots[index_] + extra;
            const float flightTime = std::max(0.3f, (player.z - ToWorld(mouth).z) / speed);
            const float side = (player.x - position_.x) / flightTime;
            for (int i = 0; i < n; ++i) {
                pending_.push_back({0.16f * static_cast<float>(i), mouth, side, speed});
            }
            break;
        }
        case Attack::Bombs: {
            // Bombs drop out of the boss, spread out side by side.
            const int n = kBombCount[index_] + (Angry() ? 1 : 0);
            for (int i = 0; i < n; ++i) {
                const float offset = (static_cast<float>(i) - 0.5f * static_cast<float>(n - 1)) * 3.2f;
                const float x = std::clamp(position_.x + offset, -config::kFieldHalfWidth + 1.0f,
                                           config::kFieldHalfWidth - 1.0f);
                bombs.emplace_back(Vector3{x, position_.y + 1.0f, position_.z + 2.0f * scale_},
                                   config::kLevelBombSpeed[level_], config::kLevelBombRadius[level_],
                                   config::kLevelBombColor[level_], level_);
            }
            break;
        }
        case Attack::Rain: {
            // The two wing guns fire straight ahead, taking turns.
            const int n = 10 + 2 * extra;
            for (int i = 0; i < n; ++i) {
                pending_.push_back({0.12f * static_cast<float>(i), kTurrets[i % 2], 0.0f, speed});
            }
            break;
        }
        case Attack::Spiral: {
            // A stream of shots that sweeps across the field, left to right or back.
            const int n = 18 + 3 * extra;
            const float direction = (attackStep_ % 2 == 0) ? 1.0f : -1.0f;
            for (int i = 0; i < n; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(n - 1);
                const float angle = direction * (-40.0f + 80.0f * t) * kDeg;
                pending_.push_back({0.05f * static_cast<float>(i), mouth,
                                    speed * std::sin(angle), speed * std::cos(angle)});
            }
            break;
        }
    }
}

// A point of the model -> the world: the same steps as in Draw (scale, lean
// forward, roll, move), done by hand. Vector3RotateByAxisAngle turns a point
// around an axis (raymath).
Vector3 Boss::ToWorld(Vector3 local) const {
    Vector3 p = Vector3Scale(local, scale_);
    p = Vector3RotateByAxisAngle(p, Vector3{1.0f, 0.0f, 0.0f}, config::kBossPitch * DEG2RAD);
    p = Vector3RotateByAxisAngle(p, Vector3{0.0f, 0.0f, 1.0f}, roll_ * DEG2RAD);
    return Vector3Add(p, position_);
}

// ... and back: the same steps in the opposite order, each one undone.
Vector3 Boss::ToLocal(Vector3 world) const {
    Vector3 p = Vector3Subtract(world, position_);
    p = Vector3RotateByAxisAngle(p, Vector3{0.0f, 0.0f, 1.0f}, -roll_ * DEG2RAD);
    p = Vector3RotateByAxisAngle(p, Vector3{1.0f, 0.0f, 0.0f}, -config::kBossPitch * DEG2RAD);
    return Vector3Scale(p, 1.0f / scale_);
}

bool Boss::Touches(Vector3 center, float radius) const {
    if (state_ != State::Fighting) return false;   // flying in or dying: cannot be hurt
    // Work in the boss's own space: move the point so the boss centre is the
    // origin, and undo the scale. Then the numbers match the model below.
    const Vector3 local = ToLocal(center);
    const float r = radius / scale_;
    return (CoreOpen() && Vector3Distance(local, kCoreCenter) < kCoreRadius + 0.3f + r) ||
           Vector3Distance(local, Vector3{0.0f, 0.4f, 0.0f}) < 3.3f + r ||      // the body
           Vector3Distance(local, Vector3{-4.3f, 0.3f, -0.2f}) < 2.0f + r ||    // left wing
           Vector3Distance(local, Vector3{4.3f, 0.3f, -0.2f}) < 2.0f + r ||     // right wing
           Vector3Distance(local, Vector3{0.0f, 0.0f, 3.6f}) < 1.4f + r;        // mandibles
}

std::optional<BossHit> Boss::TryHit(Vector3 center, float radius) {
    if (!Touches(center, radius)) return std::nullopt;   // missed (or it cannot be hurt now)
    const Vector3 local = ToLocal(center);
    const bool core = CoreOpen() && Vector3Distance(local, kCoreCenter) < kCoreRadius + 0.3f + radius / scale_;

    hitPoints_ -= core ? config::kBossCoreDamage : 1;
    flash_ = config::kBossHitFlashTime;
    const bool destroyed = hitPoints_ <= 0;
    if (destroyed) {
        state_ = State::Dying;
        stateTime_ = 0.0f;
        pending_.clear();
    }
    return BossHit{center, core, destroyed};
}

// ---- Drawing -------------------------------------------------------------------------

void Boss::Draw() const {
    // Colours of this boss, made from the level's alien colours. A hit flashes
    // everything toward white; when angry the glow flickers toward red.
    // Only a light tint (at most 35% white): a boss is hit many times a second, and
    // a full white flash would hide its colours for the whole fight.
    const float flash = 0.35f * flash_ / config::kBossHitFlashTime;
    const float rage  = Angry() ? 0.5f + 0.5f * std::sin(time_ * 12.0f) : 0.0f;
    const Color glow   = Mix(Mix(glow_, kRed, 0.6f * rage), kWhite, flash);
    const Color shell  = Mix(Mix(body_, kWhite, 0.15f), kWhite, flash);
    const Color shellB = Mix(shell, Darker(body_, 0.6f), 0.35f);           // the second facet colour
    const Color armour = Mix(Mix(body_, Color{92, 92, 108, 255}, 0.55f), kWhite, flash);
    const Color dark   = Darker(body_, 0.45f);

    rlPushMatrix();
    rlTranslatef(position_.x, position_.y, position_.z);
    rlRotatef(roll_, 0.0f, 0.0f, 1.0f);
    rlRotatef(config::kBossPitch, 1.0f, 0.0f, 0.0f);   // lean back: the face turns up toward the camera
    rlScalef(scale_, scale_, scale_);

    // ---- The dome: two rings of 12 points and a top, like a faceted shell ----------
    constexpr int kN = 12;
    Vector3 outer[kN], inner[kN];
    for (int i = 0; i < kN; ++i) {
        const float a = 2.0f * PI * static_cast<float>(i) / static_cast<float>(kN);
        const float depth = (std::sin(a) > 0.0f) ? 2.6f : 3.0f;   // longer at the back
        outer[i] = Vector3{3.6f * std::cos(a), 0.2f, depth * std::sin(a)};
        inner[i] = Vector3{0.55f * outer[i].x, 1.35f, 0.55f * outer[i].z - 0.15f};
    }
    const Vector3 apex{0.0f, 1.95f, -0.3f};
    const Vector3 bottom{0.0f, -1.2f, 0.2f};
    const Vector3 origin{0.0f, 0.0f, 0.0f};
    for (int i = 0; i < kN; ++i) {
        const int j = (i + 1) % kN;
        const Color c = (i % 2 == 0) ? shell : shellB;
        DrawQuadFace(outer[i], outer[j], inner[j], inner[i], c, Mid(outer[i], outer[j], inner[j]));
        DrawFace(inner[i], inner[j], apex, c, Mid(inner[i], inner[j], apex));
        DrawFace(outer[i], bottom, outer[j], dark, Mid(outer[i], bottom, outer[j]));
    }
    // Glowing ribs over the dome.
    for (int i = 0; i < kN; i += 3) {
        DrawLine3D(Vector3Add(outer[i], Vector3{0, 0.02f, 0}), Vector3Add(inner[i], Vector3{0, 0.02f, 0}), glow);
        DrawLine3D(Vector3Add(inner[i], Vector3{0, 0.02f, 0}), apex, glow);
    }

    // ---- Wings, guns, mandibles and eyes (mirrored) --------------------------------
    for (int side = 0; side < 2; ++side) {
        const float s = side == 0 ? -1.0f : 1.0f;
        auto P = [s](float x, float y, float z) { return Vector3{s * x, y, z}; };

        // A flat armoured wing with a dark underside and glowing edges.
        const Vector3 a = P(2.9f, 0.25f, -1.4f), b = P(5.6f, 0.7f, -2.6f), c = P(6.2f, 0.35f, 0.2f);
        const Vector3 d = P(4.8f, 0.15f, 1.7f), e = P(3.2f, 0.15f, 1.4f);
        const Vector3 up{0.0f, 1.0f, 0.0f}, down{0.0f, -1.0f, 0.0f}, low{0.0f, -0.3f, 0.0f};
        DrawFace(a, b, c, armour, up);
        DrawFace(a, c, d, armour, up);
        DrawFace(a, d, e, armour, up);
        DrawFace(Vector3Add(a, low), Vector3Add(b, low), Vector3Add(c, low), dark, down);
        DrawFace(Vector3Add(a, low), Vector3Add(c, low), Vector3Add(d, low), dark, down);
        DrawLine3D(b, c, glow);
        DrawLine3D(c, d, glow);

        // A gun under the wing; its muzzle glows (see DrawGlow).
        const Vector3 muzzle = kTurrets[side];
        DrawCylinderEx(Vector3{muzzle.x, muzzle.y, muzzle.z - 1.4f}, muzzle, 0.32f, 0.24f, 8, dark);
        DrawSphere(muzzle, 0.2f, glow);

        // A big mandible in front, pointing at the player.
        const Vector3 root = P(1.0f, -0.1f, 2.0f), out = P(2.3f, -0.2f, 2.6f);
        const Vector3 tip = P(0.55f, -0.25f, 4.8f), ridge = P(1.35f, 0.35f, 3.0f);
        const Vector3 inside = P(1.3f, -0.1f, 3.0f);
        auto face = [&](Vector3 p, Vector3 q, Vector3 r, Color col) {
            DrawFace(p, q, r, col, Vector3Subtract(Mid(p, q, r), inside));
        };
        face(root, out, ridge, armour);
        face(out, tip, ridge, armour);
        face(tip, root, ridge, shellB);
        face(root, tip, out, dark);
        DrawLine3D(ridge, tip, glow);

        // Eyes.
        DrawSphere(kEyes[side], 0.26f, Angry() ? kRed : glow);

        // ---- Extra parts of the bigger bosses ----
        if (index_ >= 1) {
            // Two horns.
            const Vector3 h0 = P(1.2f, 1.2f, -0.1f), h1 = P(2.0f, 1.0f, -0.2f), h2 = P(1.6f, 1.1f, -0.9f);
            const Vector3 hornTip = P(2.9f, 4.0f, -2.0f);
            const Vector3 hornIn  = Mid(h0, h1, h2);
            const Color horn = Mix(armour, glow, 0.2f);
            DrawFace(h0, h1, hornTip, horn, Vector3Subtract(Mid(h0, h1, hornTip), hornIn));
            DrawFace(h1, h2, hornTip, horn, Vector3Subtract(Mid(h1, h2, hornTip), hornIn));
            DrawFace(h2, h0, hornTip, horn, Vector3Subtract(Mid(h2, h0, hornTip), hornIn));
            // A second, raised wing.
            DrawFace(P(2.6f, 0.9f, -0.6f), P(4.8f, 2.1f, -2.2f), P(5.2f, 1.2f, 0.4f), shell, up);
            DrawLine3D(P(4.8f, 2.1f, -2.2f), P(5.2f, 1.2f, 0.4f), glow);
        }
    }

    if (index_ >= 2) {
        // A crown of spikes on the dome ...
        for (int i = 1; i < kN; i += 2) {
            const Vector3 base = inner[i];
            const Vector3 tipUp{base.x * 1.5f, base.y + 1.9f, base.z * 1.5f};
            const Vector3 left  = Vector3Lerp(inner[i], inner[(i + kN - 1) % kN], 0.35f);
            const Vector3 right = Vector3Lerp(inner[i], inner[(i + 1) % kN], 0.35f);
            DrawFace(left, right, tipUp, armour, Vector3Subtract(Mid(left, right, tipUp), origin));
            DrawLine3D(base, tipUp, glow);
        }
        // ... and a ring of blades that circles around the body.
        for (int i = 0; i < 10; ++i) {
            const float a = time_ * 0.8f + 2.0f * PI * static_cast<float>(i) / 10.0f;
            const Vector3 dir{std::cos(a), 0.0f, std::sin(a)};
            const Vector3 baseA = Vector3Add(Vector3Scale(dir, 4.4f), Vector3{-dir.z * 0.4f, 0.0f, dir.x * 0.4f});
            const Vector3 baseB = Vector3Add(Vector3Scale(dir, 4.4f), Vector3{dir.z * 0.4f, 0.0f, -dir.x * 0.4f});
            const Vector3 blade = Vector3Add(Vector3Scale(dir, 5.4f), Vector3{0.0f, 0.1f, 0.0f});
            DrawFace(baseA, baseB, blade, glow, Vector3{0.0f, 1.0f, 0.0f});
        }
        DrawCircle3D(Vector3{0.0f, 0.0f, 0.0f}, 4.4f, Vector3{1.0f, 0.0f, 0.0f}, 90.0f, Fade(glow, 0.7f));
    }

    // ---- The core and its four armour petals -----------------------------------------
    // The petals swing open while the core is vulnerable. "open" eases from 0 to 1.
    float open = 0.0f;
    if (state_ == State::Fighting) {
        const float into = std::fmod(stateTime_, config::kBossCoreCycle) -
                           (config::kBossCoreCycle - config::kBossCoreOpen);   // < 0 while closed
        open = Smooth(std::min(into / 0.3f, (config::kBossCoreOpen - into) / 0.3f));
    } else if (state_ == State::Dying) {
        open = 1.0f;
    }
    DrawSphere(kCoreCenter, kCoreRadius, Mix(Darker(glow, 0.35f), Mix(glow, kWhite, 0.4f), open));
    for (int i = 0; i < 4; ++i) {
        const float a  = (45.0f + 90.0f * static_cast<float>(i)) * DEG2RAD;
        const float a1 = a - 45.0f * DEG2RAD, a2 = a + 45.0f * DEG2RAD;
        const Vector3 b1{1.15f * std::cos(a1), kCoreCenter.y + 1.15f * std::sin(a1), 2.2f};
        const Vector3 b2{1.15f * std::cos(a2), kCoreCenter.y + 1.15f * std::sin(a2), 2.2f};
        const Vector3 closedTip{0.0f, kCoreCenter.y, 3.65f};
        const Vector3 openTip{1.9f * std::cos(a), kCoreCenter.y + 1.9f * std::sin(a), 2.9f};
        const Vector3 tip = Vector3Lerp(closedTip, openTip, open);
        DrawFace(b1, b2, tip, armour, Vector3Subtract(Mid(b1, b2, tip), kCoreCenter));
        DrawLine3D(b1, tip, glow);
    }

    rlPopMatrix();
}

void Boss::DrawGlow(const GlowRenderer& glow, const Camera3D& camera) const {
    auto world = [this](Vector3 p) { return ToWorld(p); };
    const bool open = CoreOpen() || state_ == State::Dying;
    const float pulse = 0.85f + 0.15f * std::sin(time_ * 8.0f);
    const Color eyes = Angry() ? kRed : glow_;
    glow.Draw(camera, world(kCoreCenter), (open ? 6.0f * pulse : 2.0f) * scale_,
              open ? Mix(glow_, kWhite, 0.3f) : glow_);
    for (const Vector3& eye : kEyes) glow.Draw(camera, world(eye), 1.4f * scale_, eyes);
    for (const Vector3& gun : kTurrets) glow.Draw(camera, world(gun), 1.2f * scale_, glow_);
}
