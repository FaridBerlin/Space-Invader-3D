// Laser.cpp

#include "Laser.h"

#include <algorithm>   // std::max, std::min, std::remove_if
#include <cmath>       // std::sin, std::ceil, std::sqrt

#include "GlowRenderer.h"
#include "Random.h"
#include "raymath.h"   // Vector3Lerp, Vector3Distance

namespace {

// A bolt is a chain of straight pieces, one every kStep units along it. Each corner
// is pushed a little to the side at random: that is the zig-zag.
constexpr float kStep     = 1.1f;
constexpr int   kBranches = 4;   // small forks per bolt, re-drawn with every jitter

}  // namespace

float Laser::Thickness() const {
    return config::kLaserThickness[std::clamp(level_, 1, config::kWeaponLevels) - 1];
}

void Laser::Update(float dt, bool on, int level) {
    on_    = on;
    level_ = level;
    time_ += dt;

    // Chain arcs fade away by themselves (also after fire is let go).
    for (Arc& arc : arcs_) arc.life -= dt;
    arcs_.erase(std::remove_if(arcs_.begin(), arcs_.end(), [](const Arc& a) { return a.life <= 0.0f; }),
                arcs_.end());

    if (!on_) return;
    // Real lightning flickers: every few hundredths of a second a new zig-zag.
    jitterTimer_ -= dt;
    if (jitterTimer_ <= 0.0f) {
        Jitter();
        jitterTimer_ = config::kLaserJitterTime;
    }
}

void Laser::SetBolt(int i, bool active, Vector3 start, Vector3 end, bool hitting) {
    Bolt& bolt = bolts_[static_cast<std::size_t>(i)];
    bolt.active  = active;
    bolt.start   = start;
    bolt.hitting = hitting;
    // Always at least a short bolt, even right in front of a target.
    bolt.end = (start.z - end.z < 0.5f) ? Vector3{end.x, end.y, start.z - 0.5f} : end;
}

void Laser::AddArc(Vector3 from, Vector3 to) {
    Arc arc{from, to, config::kLaserArcTime, {}};
    for (float& offset : arc.offsets) offset = RandomFloat(-0.35f, 0.35f);
    arcs_.push_back(arc);
}

void Laser::Jitter() {
    // The main zig-zag stays close to the line; the thin strands wander further,
    // so they wrap around it like in a real electric discharge.
    const float wild = 0.16f + 0.04f * Thickness();
    for (Bolt& bolt : bolts_) {
        bolt.offsets[0] = 0.0f;
        for (int i = 1; i < kMaxPoints; ++i) {
            bolt.offsets[static_cast<std::size_t>(i)] = RandomFloat(-wild, wild);
        }
        for (auto& strand : bolt.strands) {
            strand[0] = 0.0f;
            for (int i = 1; i < kMaxPoints; ++i) {
                strand[static_cast<std::size_t>(i)] = RandomFloat(-2.4f * wild, 2.4f * wild);
            }
        }
        bolt.branches.clear();
        for (int i = 0; i < kBranches; ++i) {
            bolt.branches.push_back(Branch{GetRandomValue(1, kMaxPoints - 2),
                                           RandomFloat(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f,
                                           RandomFloat(0.5f, 1.2f) + 0.2f * Thickness()});
        }
    }
}

int Laser::PointCount(const Bolt& bolt) const {
    const float length = Vector3Distance(bolt.start, bolt.end);
    return std::min(kMaxPoints, static_cast<int>(std::ceil(length / kStep)) + 1);
}

Vector3 Laser::Point(const Bolt& bolt, int index, const std::array<float, kMaxPoints>& offsets) const {
    const int last = PointCount(bolt) - 1;
    // Evenly along the straight line from start to end, then pushed sideways.
    // The first point sits on the nose and the last one exactly at the end, both
    // without a sideways offset, so the bolt starts and ends in the right place.
    const float t = last > 0 ? static_cast<float>(index) / static_cast<float>(last) : 1.0f;
    Vector3 p = Vector3Lerp(bolt.start, bolt.end, t);
    if (index != 0 && index != last) p.x += offsets[static_cast<std::size_t>(index)];
    return p;
}

void Laser::Draw() const {
    if (on_) {
        const float thick = Thickness();
        const float sleeve = 0.055f * thick;   // the see-through blue around the core
        const float core   = 0.022f * thick;   // the white-hot middle
        const float strand = 0.010f * thick;   // the thin side strands
        for (const Bolt& bolt : bolts_) {
            if (!bolt.active) continue;
            const int count = PointCount(bolt);
            // A cylinder can be drawn between any two points, so each straight piece
            // of the zig-zag is one cylinder: first the blue sleeve, then the core.
            for (int i = 0; i + 1 < count; ++i) {
                const Vector3 a = Point(bolt, i, bolt.offsets);
                const Vector3 b = Point(bolt, i + 1, bolt.offsets);
                DrawCylinderEx(a, b, sleeve, sleeve, 6, Fade(config::kLaserColor, 0.6f));
                DrawCylinderEx(a, b, core, core, 5, config::kLaserCoreColor);
            }
            // The thin violet strands that wind around the bolt.
            for (const auto& offsets : bolt.strands) {
                for (int i = 0; i + 1 < count; ++i) {
                    DrawCylinderEx(Point(bolt, i, offsets), Point(bolt, i + 1, offsets), strand, strand, 4,
                                   Fade(config::kLaserStrandColor, 0.75f));
                }
            }
            // The forks: two short bent pieces sticking out to the side.
            for (const Branch& branch : bolt.branches) {
                if (branch.from >= count - 1) continue;
                const Vector3 a = Point(bolt, branch.from, bolt.offsets);
                const Vector3 b{a.x + branch.side * branch.length * 0.55f, a.y, a.z - branch.length * 0.45f};
                const Vector3 c{b.x + branch.side * branch.length * 0.35f, a.y, b.z - branch.length * 0.55f};
                DrawCylinderEx(a, b, strand * 1.4f, strand, 4, config::kLaserStrandColor);
                DrawCylinderEx(b, c, strand, 0.0f, 4, config::kLaserStrandColor);
            }
        }
    }

    // The chain jumps: a short zig-zag from one alien to the next.
    for (const Arc& arc : arcs_) {
        Vector3 previous = arc.from;
        const int pieces = static_cast<int>(arc.offsets.size()) + 1;
        for (int i = 1; i <= pieces; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(pieces);
            Vector3 p = Vector3Lerp(arc.from, arc.to, t);
            if (i < pieces) {
                p.x += arc.offsets[static_cast<std::size_t>(i - 1)];
                p.z += arc.offsets[static_cast<std::size_t>(pieces - 1 - i)] * 0.6f;
            }
            DrawCylinderEx(previous, p, 0.07f, 0.07f, 5, Fade(config::kLaserColor, 0.6f));
            DrawCylinderEx(previous, p, 0.03f, 0.03f, 4, config::kLaserCoreColor);
            previous = p;
        }
    }
}

void Laser::DrawGlow(const GlowRenderer& glow, const Camera3D& camera) const {
    if (on_) {
        const float thick = Thickness();
        // Soft blue light along each bolt (between the corners, faint, so it melts
        // into one even haze), a flash at the nose, and a bigger flickering flash
        // where it hits something.
        for (const Bolt& bolt : bolts_) {
            if (!bolt.active) continue;
            const int count = PointCount(bolt);
            for (int i = 0; i + 1 < count; ++i) {
                const Vector3 middle = Vector3Lerp(Point(bolt, i, bolt.offsets), Point(bolt, i + 1, bolt.offsets), 0.5f);
                glow.Draw(camera, middle, 1.3f + 0.4f * thick, Fade(config::kLaserColor, 0.35f));
            }
            glow.Draw(camera, bolt.start, 1.2f + 0.3f * thick, config::kLaserCoreColor);
            if (bolt.hitting) {
                const float flicker = 1.0f + 0.25f * std::sin(time_ * 47.0f + bolt.start.x);
                glow.Draw(camera, bolt.end, (2.0f + 0.4f * thick) * flicker, config::kLaserColor);
            }
        }
    }
    for (const Arc& arc : arcs_) {
        glow.Draw(camera, arc.to, 2.0f, config::kLaserColor);
        glow.Draw(camera, Vector3Lerp(arc.from, arc.to, 0.5f), 1.6f, config::kLaserStrandColor);
    }
}
