// Particles.h - explosions made of many tiny glowing cubes.
//
// A particle effect = lots of simple objects that each live a short time, fly
// outward, shrink and fade. Together they look like fire or sparks.

#pragma once

#include <vector>

#include "raylib.h"

struct Particle {
    Vector3 position;
    Vector3 velocity;   // units per second
    float   life;       // seconds left
    float   maxLife;    // starting life, so we know how far through its life it is
    float   size;       // edge length of the cube at birth
    Color   color;
    bool    alive = true;
};

// Describes what one explosion looks like.
struct ExplosionStyle {
    int   count;     // how many particles
    float speed;     // top outward speed, units per second
    float life;      // longest life of a particle, seconds
    float size;      // largest cube size
    Color colorA;    // each particle gets a random mix of A and B
    Color colorB;
};

class ParticleSystem {
public:
    void Explode(Vector3 center, const ExplosionStyle& style);
    void Update(float dt);
    void Draw() const;   // call between BeginMode3D and EndMode3D
    void Clear() { particles_.clear(); }

private:
    std::vector<Particle> particles_;
};
