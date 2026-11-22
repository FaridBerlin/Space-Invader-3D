// Laser.cpp

#include "Laser.h"

#include <algorithm>   // std::min, std::max, std::clamp, std::remove_if
#include <cmath>       // std::sin, std::sqrt

#include "GlowRenderer.h"
#include "Random.h"
#include "raymath.h"   // Vector3Lerp, Vector3Distance, Vector3Subtract, ...
#include "rlgl.h"      // rlBegin / rlVertex3f: drawing our own triangles

namespace {

// How jagged a path is: how far a new middle point may be pushed to the side, as a
// share of the piece it splits. Bigger = wilder.
constexpr float kBoltRoughness   = 0.21f;   // the main bolt curves gently
constexpr float kStrandRoughness = 0.45f;   // the thin strands are sharp and wild
constexpr float kArcRoughness    = 0.30f;
// A long bolt does not swing further than this to the side (world units), or it
// would look like it misses what it hits.
constexpr float kMaxSway = 5.5f;

// Builds a new jagged path. It starts with a straight line (2 points) and splits
// every piece in the middle, again and again, until all kPathPoints corners are
// set ("midpoint displacement"). Each new middle point is pushed to the side by
// a random amount that shrinks with the size of the piece: big kinks first, then
// smaller and smaller jags. The middle point also slides a little along the line,
// so the pieces are not all the same length.
void MakePath(Laser::Path& path, float roughness) {
    constexpr int last = Laser::kPathPoints - 1;
    path.t[0] = 0.0f;
    path.side[0] = 0.0f;
    path.t[last] = 1.0f;
    path.side[last] = 0.0f;
    for (int step = last / 2; step >= 1; step /= 2) {
        for (int i = step; i < last; i += 2 * step) {
            const auto a = static_cast<std::size_t>(i - step);
            const auto b = static_cast<std::size_t>(i + step);
            const auto m = static_cast<std::size_t>(i);
            const float span = path.t[b] - path.t[a];
            path.t[m] = path.t[a] + span * (0.5f + RandomFloat(-0.18f, 0.18f));
            path.side[m] = 0.5f * (path.side[a] + path.side[b]) + RandomFloat(-1.0f, 1.0f) * roughness * span;
        }
    }
}

// The direction "to the side" of a line from a to b, flat on the field (y = 0).
Vector3 SideOf(Vector3 a, Vector3 b) {
    const float dx = b.x - a.x;
    const float dz = b.z - a.z;
    const float length = std::sqrt(dx * dx + dz * dz);
    if (length < 0.0001f) return Vector3{1.0f, 0.0f, 0.0f};
    return Vector3{-dz / length, 0.0f, dx / length};
}

// Turns a path into world points from "start" to "end".
int PathToPoints(const Laser::Path& path, Vector3 start, Vector3 end,
                 std::array<Vector3, Laser::kPathPoints>& points) {
    const Vector3 side = SideOf(start, end);
    const float sway = std::min(Vector3Distance(start, end), kMaxSway);
    for (int i = 0; i < Laser::kPathPoints; ++i) {
        const auto k = static_cast<std::size_t>(i);
        points[k] = Vector3Add(Vector3Lerp(start, end, path.t[k]), Vector3Scale(side, path.side[k] * sway));
    }
    return Laser::kPathPoints;
}

// Draws a flat band along the points that always faces the camera (a "ribbon").
// Neighbouring pieces share their corners, so the band has no gaps and no
// overlaps: no bright knots. widthStart / widthEnd: half the width at the first
// and at the last point (a fork gets thinner towards its tip).
// soft = true: the band is brightest along the middle and fades out to nothing at
// its edges (for the glow); false: one even colour (for the sharp core).
void DrawRibbon(const Vector3* points, int count, float widthStart, float widthEnd, Color color,
                const Camera3D& camera, bool soft = false) {
    if (count < 2) return;
    rlCheckRenderBatchLimit(count * 12);   // room for all triangles in the current batch
    // For each corner: the two edge points of the band, left and right of it.
    Vector3 left[Laser::kPathPoints];
    Vector3 right[Laser::kPathPoints];
    for (int i = 0; i < count; ++i) {
        // The band runs along the line (from the previous to the next corner) ...
        const Vector3 along = Vector3Subtract(points[std::min(i + 1, count - 1)], points[std::max(i - 1, 0)]);
        // ... and spreads out at right angles to it and to the view direction.
        const Vector3 toCamera = Vector3Subtract(camera.position, points[i]);
        const Vector3 spread = Vector3Normalize(Vector3CrossProduct(along, toCamera));
        const float t = static_cast<float>(i) / static_cast<float>(count - 1);
        const float width = widthStart + (widthEnd - widthStart) * t;
        left[i]  = Vector3Add(points[i], Vector3Scale(spread, width));
        right[i] = Vector3Subtract(points[i], Vector3Scale(spread, width));
    }
    // One corner of a triangle, with its own colour (the GPU blends the colours
    // in between, so a soft band fades smoothly from the middle to the edge).
    auto corner = [](const Vector3& p, Color c) {
        rlColor4ub(c.r, c.g, c.b, c.a);
        rlVertex3f(p.x, p.y, p.z);
    };
    const Color edge = soft ? Color{color.r, color.g, color.b, 0} : color;
    rlBegin(RL_TRIANGLES);
    for (int i = 0; i + 1 < count; ++i) {
        if (soft) {
            // Two strips per piece: left edge to the middle line, middle line to the right edge.
            corner(left[i], edge);      corner(points[i], color);       corner(points[i + 1], color);
            corner(left[i], edge);      corner(points[i + 1], color);   corner(left[i + 1], edge);
            corner(points[i], color);   corner(right[i], edge);         corner(right[i + 1], edge);
            corner(points[i], color);   corner(right[i + 1], edge);     corner(points[i + 1], color);
        } else {
            // One four-cornered strip, made of two triangles.
            corner(left[i], color);     corner(right[i], color);        corner(right[i + 1], color);
            corner(left[i], color);     corner(right[i + 1], color);    corner(left[i + 1], color);
        }
    }
    rlEnd();
}

// Every second point of a path: enough for the soft, wide layers (you cannot see
// small jags in a blur), and half the triangles.
int EveryOther(const Vector3* points, int count, std::array<Vector3, Laser::kPathPoints>& out) {
    int n = 0;
    for (int i = 0; i < count; i += 2) out[static_cast<std::size_t>(n++)] = points[i];
    return n;   // count is odd (65), so the last point is always included
}

// One bolt or arc in three layers: a wide soft haze, a brighter glow, a white core.
void DrawLightning(const Vector3* points, int count, float thickness, float brightness,
                   const config::LaserColors& colors, const Camera3D& camera) {
    std::array<Vector3, Laser::kPathPoints> coarse;
    const int n = EveryOther(points, count, coarse);
    DrawRibbon(coarse.data(), n, 0.45f * thickness, 0.45f * thickness, Fade(colors.glow, 0.35f * brightness), camera, true);
    DrawRibbon(coarse.data(), n, 0.14f * thickness, 0.14f * thickness, Fade(colors.glow, 0.80f * brightness), camera, true);
    const float core = std::max(0.04f, 0.05f * thickness);
    DrawRibbon(points, count, core, core, colors.core, camera);
}

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
    // Real lightning flickers: every few hundredths of a second a new shape.
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
    Arc arc{from, to, config::kLaserArcTime, {}, config::LaserColorsFor(level_).glow};
    MakePath(arc.path, kArcRoughness);
    arcs_.push_back(arc);
}

void Laser::Jitter() {
    for (Bolt& bolt : bolts_) {
        MakePath(bolt.path, kBoltRoughness);
        for (Path& strand : bolt.strands) MakePath(strand, kStrandRoughness);
        for (Fork& fork : bolt.forks) {
            fork.from      = GetRandomValue(6, kPathPoints - 10);
            fork.direction = RandomFloat(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;
            fork.length    = RandomFloat(0.6f, 1.6f) + 0.25f * Thickness();
            for (float& side : fork.side) side = RandomFloat(-0.18f, 0.18f);
            fork.side.front() = 0.0f;
        }
        bolt.brightness = RandomFloat(0.75f, 1.0f);
    }
}

void Laser::Draw(const Camera3D& camera) const {
    if (!on_ && arcs_.empty()) return;

    // Lightning is light: it ADDS its colour to what is behind it (additive blending),
    // and it does not hide what is drawn after it (no depth writes). Both sides of
    // the bands are drawn (no back-face culling).
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();
    rlDisableBackfaceCulling();

    const config::LaserColors colors = config::LaserColorsFor(level_);
    const float thick = Thickness();
    std::array<Vector3, kPathPoints> points;

    if (on_) {
        for (const Bolt& bolt : bolts_) {
            if (!bolt.active) continue;
            // The thin strands first, then the main bolt over them. Each strand: a
            // faint glow and a thin bright line.
            for (const Path& strand : bolt.strands) {
                PathToPoints(strand, bolt.start, bolt.end, points);
                std::array<Vector3, kPathPoints> coarse;   // the thin strands: every second point is plenty
                const int n = EveryOther(points.data(), kPathPoints, coarse);
                DrawRibbon(coarse.data(), n, 0.08f * thick, 0.08f * thick,
                           Fade(colors.glow, 0.25f * bolt.brightness), camera, true);
                DrawRibbon(coarse.data(), n, 0.014f * thick, 0.014f * thick,
                           Fade(colors.strand, 0.75f * bolt.brightness), camera);
            }

            PathToPoints(bolt.path, bolt.start, bolt.end, points);
            DrawLightning(points.data(), kPathPoints, thick, bolt.brightness, colors, camera);

            // The forks: short jagged twigs that branch off to the side and up,
            // getting thinner towards their tips.
            const Vector3 side = SideOf(bolt.start, bolt.end);
            const Vector3 ahead = Vector3Normalize(Vector3Subtract(bolt.end, bolt.start));
            for (const Fork& fork : bolt.forks) {
                const Vector3 root = points[static_cast<std::size_t>(fork.from)];
                const Vector3 direction =
                    Vector3Normalize(Vector3Add(Vector3Scale(side, 0.8f * fork.direction), Vector3Scale(ahead, 0.6f)));
                const Vector3 across = SideOf(root, Vector3Add(root, direction));
                std::array<Vector3, kForkPoints> twig;
                for (int k = 0; k < kForkPoints; ++k) {
                    const float share = static_cast<float>(k) / static_cast<float>(kForkPoints - 1);
                    twig[static_cast<std::size_t>(k)] = Vector3Add(
                        Vector3Add(root, Vector3Scale(direction, share * fork.length)),
                        Vector3Scale(across, fork.side[static_cast<std::size_t>(k)] * fork.length));
                }
                DrawRibbon(twig.data(), kForkPoints, 0.09f * thick, 0.01f, Fade(colors.glow, 0.5f * bolt.brightness),
                           camera, true);
                DrawRibbon(twig.data(), kForkPoints, 0.022f * thick, 0.004f, Fade(colors.core, 0.8f * bolt.brightness),
                           camera);
            }
        }
    }

    // A star burst where the bolts leave the ship: a few short rays that turn slowly.
    if (on_) {
        for (const Bolt& bolt : bolts_) {
            if (!bolt.active) continue;
            constexpr int kRays = 6;
            for (int r = 0; r < kRays; ++r) {
                const float angle = time_ * 1.5f + static_cast<float>(r) * (2.0f * PI / kRays) + bolt.start.x;
                const float length = (0.5f + 0.25f * std::sin(time_ * 9.0f + static_cast<float>(r))) * (0.8f + 0.2f * thick);
                const Vector3 ray[2] = {bolt.start, Vector3Add(bolt.start, Vector3{std::cos(angle) * length, 0.0f,
                                                                                    std::sin(angle) * length})};
                DrawRibbon(ray, 2, 0.035f * thick, 0.0f, Fade(colors.core, 0.8f), camera);
            }
        }
    }

    // The chain jumps: a short bolt from one alien to the next that fades out.
    for (const Arc& arc : arcs_) {
        const float fade = std::clamp(arc.life / config::kLaserArcTime, 0.0f, 1.0f);
        config::LaserColors arcColors = colors;
        arcColors.glow = arc.color;
        arcColors.core = Fade(colors.core, fade);
        PathToPoints(arc.path, arc.from, arc.to, points);
        DrawLightning(points.data(), kPathPoints, 1.0f, fade, arcColors, camera);
    }

    // raylib collects triangles and draws them later in one go ("batch"). Draw them
    // NOW, while the settings above are still on, then switch the settings back.
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
    EndBlendMode();
}

void Laser::DrawGlow(const GlowRenderer& glow, const Camera3D& camera) const {
    const config::LaserColors colors = config::LaserColorsFor(level_);
    if (on_) {
        const float thick = Thickness();
        // A flash at the nose, and a bigger flickering flash where a bolt hits
        // something. (The light along the bolts is part of the bolts themselves.)
        for (const Bolt& bolt : bolts_) {
            if (!bolt.active) continue;
            glow.Draw(camera, bolt.start, 1.0f + 0.3f * thick, colors.core);
            if (bolt.hitting) {
                const float flicker = 1.0f + 0.25f * std::sin(time_ * 47.0f + bolt.start.x);
                glow.Draw(camera, bolt.end, (1.8f + 0.4f * thick) * flicker, colors.glow);
            }
        }
    }
    for (const Arc& arc : arcs_) {
        glow.Draw(camera, arc.to, 1.6f, Fade(arc.color, std::clamp(arc.life / config::kLaserArcTime, 0.0f, 1.0f)));
    }
}
