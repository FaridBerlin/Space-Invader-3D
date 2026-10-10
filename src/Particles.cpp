// Particles.cpp

#include "Particles.h"

#include <cmath>

#include "Config.h"
#include "Random.h"
#include "Util.h"
#include "raymath.h"
#include "rlgl.h"

namespace {

// Blend two colours: amount 0 gives a, amount 1 gives b, 0.5 the middle.
// This "linear interpolation" (lerp) is one of the most used ideas in graphics.
Color MixColor(Color a, Color b, float amount) {
    auto mix = [amount](unsigned char from, unsigned char to) {
        return static_cast<unsigned char>(static_cast<float>(from) +
                                          (static_cast<float>(to) - static_cast<float>(from)) * amount);
    };
    return Color{mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), 255};
}

}  // namespace

void ParticleSystem::Explode(Vector3 center, const ExplosionStyle& style) {
    for (int i = 0; i < style.count; ++i) {
        // A cap protects slower devices (phones!) from too many particles.
        if (static_cast<int>(particles_.size()) >= config::kMaxParticles) {
            return;
        }

        // Fly outward in a random direction at a random speed.
        const Vector3 direction = RandomDirection();
        const float speed = style.speed * RandomFloat(0.3f, 1.0f);
        const float life  = style.life * RandomFloat(0.6f, 1.0f);

        Particle p;
        p.position = center;
        p.velocity = Vector3Scale(direction, speed);   // direction * speed
        p.life     = life;
        p.maxLife  = life;
        p.size     = style.size * RandomFloat(0.6f, 1.0f);
        p.color    = MixColor(style.colorA, style.colorB, RandomFloat(0.0f, 1.0f));
        particles_.push_back(p);
    }
}

void ParticleSystem::Update(float dt) {
    for (auto& p : particles_) {
        p.life -= dt;
        if (p.life <= 0.0f) {
            p.alive = false;
            continue;   // skip the rest for this particle, go to the next one
        }

        // Gravity pulls down: it changes the velocity a little every second.
        p.velocity.y -= config::kParticleGravity * dt;

        // Drag (air resistance) slows particles down. "exp(-k * dt)" is the share
        // of speed that is kept this frame. It works the same at any frame rate.
        p.velocity = Vector3Scale(p.velocity, std::exp(-config::kParticleDrag * dt));

        // Move: position = position + velocity * time
        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
    }

    RemoveDead(particles_);
}

void ParticleSystem::Draw() const {
    // ---- Blending ----------------------------------------------------------------
    // Normally a new pixel REPLACES what is on screen. With ADDITIVE blending the
    // new colour is ADDED to the old one. Where particles overlap, the result gets
    // brighter and brighter, like light. That is why explosions look like they glow.
    BeginBlendMode(BLEND_ADDITIVE);

    // ---- The depth buffer ----------------------------------------------------------
    // The GPU keeps a "depth buffer": for every pixel, how far away the nearest
    // drawn thing is. Anything farther away is skipped. That works for solid
    // objects but not for see-through ones: a faint particle in front would
    // hide a bright particle behind it. So for transparent effects we still TEST
    // against the depth buffer (do not draw behind solid objects) but do not WRITE
    // to it (do not block other particles).
    rlDisableDepthMask();

    for (const auto& p : particles_) {
        const float t = p.life / p.maxLife;          // 1 when new, 0 when it dies
        const float size = p.size * (0.3f + 0.7f * t);   // shrink as it ages
        DrawCube(p.position, size, size, size, Fade(p.color, t));
    }

    rlEnableDepthMask();
    EndBlendMode();
}
