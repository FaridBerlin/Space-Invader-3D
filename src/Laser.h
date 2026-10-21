// Laser.h - the lightning laser: up to three jagged electric bolts from the ship's
// nose up the field, each to the first alien, bomb or boss in its way, and short
// "chain" arcs that jump from a hit alien to its neighbours.
//
// This class only LOOKS like lightning: it keeps the zig-zag shapes and draws
// them. What the bolts hit, and the damage, is decided by Game (see
// Game::UpdateLaser), which tells the laser every frame where each bolt starts
// and ends, and adds an arc for every chain jump.

#pragma once

#include <array>
#include <vector>

#include "Config.h"
#include "raylib.h"

class GlowRenderer;   // only used by reference, so a forward declaration is enough

class Laser {
public:
    // Every frame, once: on = fire is held; level = the weapon level, 1 .. 5
    // (how thick the bolts are). Also lets the chain arcs fade away.
    void Update(float dt, bool on, int level);

    // Every frame, for each bolt i (0 .. kLaserMaxBolts-1): is it there, where does
    // it start (the ship's nose) and end, and does it end on a target (then a bright
    // flash shows there)?
    void SetBolt(int i, bool active, Vector3 start, Vector3 end, bool hitting);

    // A chain jump: a short flash of lightning from one alien to another.
    void AddArc(Vector3 from, Vector3 to);

    bool On() const { return on_; }

    void Draw() const;                                                       // in BeginMode3D
    void DrawGlow(const GlowRenderer& glow, const Camera3D& camera) const;   // between glow.Begin/End

private:
    static constexpr int kMaxPoints = 60;   // zig-zag corners of one bolt (enough for the whole field)
    static constexpr int kStrands   = 2;    // thin side strands around each bolt

    // A small side fork of the lightning, growing out of zig-zag point "from".
    struct Branch {
        int   from;     // index of the zig-zag point it starts at
        float side;     // -1 = to the left, +1 = to the right
        float length;   // world units
    };

    struct Bolt {
        bool    active  = false;
        bool    hitting = false;
        Vector3 start{};
        Vector3 end{};
        std::array<float, kMaxPoints> offsets{};                  // the main zig-zag
        std::array<std::array<float, kMaxPoints>, kStrands> strands{};   // the thin strands
        std::vector<Branch> branches;
    };

    struct Arc {
        Vector3 from;
        Vector3 to;
        float   life;                  // seconds left
        std::array<float, 6> offsets;  // its zig-zag
    };

    void Jitter();                                   // new random zig-zags for every bolt
    int PointCount(const Bolt& bolt) const;          // how many zig-zag points a bolt has now
    Vector3 Point(const Bolt& bolt, int index, const std::array<float, kMaxPoints>& offsets) const;
    float Thickness() const;

    bool  on_    = false;
    int   level_ = 1;
    float time_  = 0.0f;
    float jitterTimer_ = 0.0f;
    std::array<Bolt, config::kLaserMaxBolts> bolts_{};
    std::vector<Arc> arcs_;
};
