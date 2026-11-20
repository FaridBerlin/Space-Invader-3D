// Laser.h - the lightning laser: up to three jagged electric bolts from the ship's
// nose up the field, each to the first alien, bomb or boss in its way, and short
// "chain" arcs that jump from a hit alien to its neighbours.
//
// This class only LOOKS like lightning: it keeps the jagged shapes and draws them.
// What the bolts hit, and the damage, is decided by Game (see Game::UpdateLaser),
// which tells the laser every frame where each bolt starts and ends, and adds an
// arc for every chain jump.
//
// How it is drawn: each bolt is one jagged line (a "path"), made by splitting a
// straight line again and again and pushing every new middle point a little to
// the side, less for the small pieces than for the big ones. That gives big kinks
// with small jags inside them, like real lightning. The path is drawn as flat
// bands that always face the camera ("ribbons"): a wide soft haze, a brighter glow
// and a thin white-hot core. The bands are connected, so there are no bright knots
// where two pieces meet.

#pragma once

#include <array>
#include <vector>

#include "Config.h"
#include "raylib.h"

class GlowRenderer;   // only used by reference, so a forward declaration is enough

class Laser {
public:
    // Every frame, once: on = fire is held; level = the weapon level, 1 .. 5
    // (how thick the bolts are, and their colour). Also lets the chain arcs fade away.
    void Update(float dt, bool on, int level);

    // Every frame, for each bolt i (0 .. kLaserMaxBolts-1): is it there, where does
    // it start (the ship's nose) and end, and does it end on a target (then a bright
    // flash shows there)?
    void SetBolt(int i, bool active, Vector3 start, Vector3 end, bool hitting);

    // A chain jump: a short flash of lightning from one alien to another.
    void AddArc(Vector3 from, Vector3 to);

    bool On() const { return on_; }

    void Draw(const Camera3D& camera) const;                                 // in BeginMode3D
    void DrawGlow(const GlowRenderer& glow, const Camera3D& camera) const;   // between glow.Begin/End

    // How many corners a path has: 2^6 pieces = 65 points.
    static constexpr int kPathPoints = 65;
    // One jagged line from "start" (t = 0) to "end" (t = 1): for every corner, how far
    // along the line it is (t) and how far it is pushed to the side, as a share of
    // the line's length (side).
    struct Path {
        std::array<float, kPathPoints> t{};
        std::array<float, kPathPoints> side{};
    };

private:
    static constexpr int kForks     = 2;   // small side forks per bolt
    static constexpr int kForkPoints = 9;  // corners of one fork

    // A small fork, growing out of corner "from" of its bolt.
    struct Fork {
        int   from = 0;
        float direction = 1.0f;   // -1 = to the left, +1 = to the right
        float length = 1.0f;      // world units
        std::array<float, kForkPoints> side{};
    };

    struct Bolt {
        bool    active  = false;
        bool    hitting = false;
        Vector3 start{};
        Vector3 end{};
        Path    path;       // the main bolt: thick, white, gently curving
        std::array<Path, 3> strands{};   // thin, sharp discharges that wind around it
        std::array<Fork, kForks> forks{};
        float   brightness = 1.0f;   // flickers a little with every new shape
    };

    struct Arc {
        Vector3 from;
        Vector3 to;
        float   life;   // seconds left
        Path    path;
        Color   color;  // the glow colour when it was made
    };

    void Jitter();     // new random shapes for every bolt
    float Thickness() const;

    bool  on_    = false;
    int   level_ = 1;
    float time_  = 0.0f;
    float jitterTimer_ = 0.0f;
    std::array<Bolt, config::kLaserMaxBolts> bolts_{};
    std::vector<Arc> arcs_;
};
