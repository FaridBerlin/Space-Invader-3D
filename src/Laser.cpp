// Laser.cpp

#include "Laser.h"

#include <algorithm>   // std::max, std::min
#include <cmath>       // std::sin, std::ceil

#include "Config.h"
#include "GlowRenderer.h"
#include "Random.h"

namespace {

// The beam is a chain of straight pieces, one every kStep units up the field.
// Each corner is pushed a little to the side at random: that is the zig-zag.
constexpr float kStep       = 1.1f;
constexpr int   kMaxPoints  = 60;     // enough for the whole field (60 * 1.1 > 50)
constexpr int   kBranches   = 5;      // small side branches, re-drawn with every jitter

}  // namespace

void Laser::Update(float dt, bool on, Vector3 start, float endZ, bool hitting, int level) {
    on_      = on;
    start_   = start;
    endZ_    = std::min(endZ, start.z - 0.5f);   // always at least a short beam
    hitting_ = hitting;
    level_   = level;
    time_   += dt;
    if (!on_) return;

    // Real lightning flickers: every few hundredths of a second a new zig-zag.
    jitterTimer_ -= dt;
    if (jitterTimer_ <= 0.0f || offsets_.empty()) {
        Jitter();
        jitterTimer_ = config::kLaserJitterTime;
    }
}

void Laser::Jitter() {
    // The higher the level, the wilder the lightning.
    const float wild = 0.18f + 0.05f * static_cast<float>(level_);
    offsets_.assign(kMaxPoints, 0.0f);
    for (int i = 1; i < kMaxPoints; ++i) {
        offsets_[static_cast<std::size_t>(i)] = RandomFloat(-wild, wild);
    }
    branches_.clear();
    for (int i = 0; i < kBranches; ++i) {
        branches_.push_back(Branch{GetRandomValue(1, kMaxPoints - 2), RandomFloat(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f,
                                   RandomFloat(0.6f, 1.4f) + 0.15f * static_cast<float>(level_)});
    }
}

int Laser::PointCount() const {
    const float length = start_.z - endZ_;
    return std::min(kMaxPoints, static_cast<int>(std::ceil(length / kStep)) + 1);
}

Vector3 Laser::Point(int index) const {
    const int last = PointCount() - 1;
    // The first point sits on the nose and the last one exactly at the end, both
    // without a sideways offset, so the beam starts and ends in the right place.
    const float offset = (index == 0 || index == last) ? 0.0f : offsets_[static_cast<std::size_t>(index)];
    const float z = (index == last) ? endZ_ : start_.z - static_cast<float>(index) * kStep;
    return Vector3{start_.x + offset, start_.y, z};
}

void Laser::Draw() const {
    if (!on_ || offsets_.empty()) return;

    // Two layers along the zig-zag: a thicker see-through orange "sleeve", and a
    // thin white-hot core inside it. A cylinder can be drawn between any two
    // points, so each straight piece is one cylinder.
    const float lv = static_cast<float>(level_);
    const float outer = 0.10f + 0.04f * lv;
    const float core  = 0.04f + 0.015f * lv;
    const Color sleeve = Fade(config::kLaserColor, 0.55f);
    const int count = PointCount();
    for (int i = 0; i + 1 < count; ++i) {
        const Vector3 a = Point(i);
        const Vector3 b = Point(i + 1);
        DrawCylinderEx(a, b, outer, outer, 6, sleeve);
        DrawCylinderEx(a, b, core, core, 5, config::kLaserCoreColor);
    }

    // The branches: two short bent pieces sticking out to the side, like the
    // little forks of real lightning. Only on the part of the beam that exists.
    for (const Branch& branch : branches_) {
        if (branch.from >= count - 1) continue;
        const Vector3 a = Point(branch.from);
        const Vector3 b{a.x + branch.side * branch.length * 0.55f, a.y, a.z - branch.length * 0.45f};
        const Vector3 c{b.x + branch.side * branch.length * 0.35f, a.y, b.z - branch.length * 0.55f};
        DrawCylinderEx(a, b, core, core * 0.6f, 4, config::kLaserColor);
        DrawCylinderEx(b, c, core * 0.6f, 0.0f, 4, config::kLaserColor);
    }
}

void Laser::DrawGlow(const GlowRenderer& glow, const Camera3D& camera) const {
    if (!on_ || offsets_.empty()) return;
    const float lv = static_cast<float>(level_);
    // Soft light along the beam (at every corner), a flash at the nose, and a
    // bigger flickering flash where it hits something.
    const int count = PointCount();
    for (int i = 0; i < count; ++i) {
        glow.Draw(camera, Point(i), 1.4f + 0.3f * lv, config::kLaserColor);
    }
    glow.Draw(camera, start_, 1.8f + 0.2f * lv, config::kLaserCoreColor);
    if (hitting_) {
        const float flicker = 1.0f + 0.25f * std::sin(time_ * 47.0f);
        glow.Draw(camera, Point(count - 1), (2.6f + 0.3f * lv) * flicker, config::kLaserColor);
    }
}
