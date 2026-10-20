// Laser.h - the lightning laser: a jagged electric beam from the ship's nose up the
// field, to the first alien, bomb or boss in its way.
//
// This class only LOOKS like lightning: it keeps the zig-zag shape and draws it.
// What the beam hits, and the damage, is decided by Game (see Game::UpdateLaser),
// which tells the laser every frame where the beam starts and where it ends.

#pragma once

#include <vector>

#include "raylib.h"

class GlowRenderer;   // only used by reference, so a forward declaration is enough

class Laser {
public:
    // Every frame. on = fire is held; start = the ship's nose; endZ = where the beam
    // stops (smaller z = further up the field); hitting = it ends on a target (then
    // a bright flash shows there); level = the weapon level, 1 .. 5 (thickness).
    void Update(float dt, bool on, Vector3 start, float endZ, bool hitting, int level);

    bool On() const { return on_; }

    void Draw() const;                                                   // the beam, in BeginMode3D
    void DrawGlow(const GlowRenderer& glow, const Camera3D& camera) const;   // between glow.Begin/End

private:
    // A small side branch of the lightning, growing out of zig-zag point "from".
    struct Branch {
        int   from;     // index of the zig-zag point it starts at
        float side;     // -1 = to the left, +1 = to the right
        float length;   // world units
    };

    void Jitter();                     // a new random zig-zag and new branches
    Vector3 Point(int index) const;    // zig-zag point number "index" in the world
    int PointCount() const;            // how many zig-zag points the beam has now

    bool    on_      = false;
    bool    hitting_ = false;
    Vector3 start_{};
    float   endZ_    = 0.0f;
    int     level_   = 1;
    float   time_    = 0.0f;
    float   jitterTimer_ = 0.0f;
    std::vector<float>  offsets_;   // sideways offset of each zig-zag point
    std::vector<Branch> branches_;
};
