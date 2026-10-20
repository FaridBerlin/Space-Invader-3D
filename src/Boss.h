// Boss.h - a huge alien that ends levels 3, 5 and 7.
//
// A boss flies in from far away, then slides from side to side and attacks in a
// fixed order of patterns (a fan of shots, aimed bursts, bombs, a rain of shots,
// a spiral). Its core opens now and then: a hit there does triple damage. Below
// half health it gets angry and attacks faster. When it runs out of hit points
// it shakes, explodes in a chain of blasts, and is gone.
//
// The boss does not know about Game. It gets the lists of shots and bombs and
// adds its attacks to them; what else happens (sounds, explosions, points) it
// reports back in a BossEvents, and Game reacts. That keeps Boss small and
// keeps all the rules about score and sound in one place (Game).

#pragma once

#include <optional>
#include <vector>

#include "Bomb.h"
#include "InvaderShot.h"
#include "raylib.h"

class GlowRenderer;   // a "forward declaration": we only use a reference to it here,
                      // so the compiler does not need the whole class yet.

// What happened when a rocket hit the boss.
struct BossHit {
    Vector3 position;    // where the rocket hit
    bool    core;        // true = it hit the open core (triple damage)
    bool    destroyed;   // true = that was the last hit point
};

// Things Game should react to after one Boss::Update.
struct BossEvents {
    bool shot = false;               // shots were fired (play the sound)
    std::vector<Vector3> blasts;     // explosions to show (during the death sequence)
    bool finished = false;           // the death sequence is over: remove the boss
};

class Boss {
public:
    // level = the 0-based level this boss ends; it picks the boss (see kLevelBoss)
    // and the colours (the level's alien colours).
    explicit Boss(int level);

    // player = where the ship is (the aimed bursts fly there).
    BossEvents Update(float dt, Vector3 player, std::vector<InvaderShot>& shots, std::vector<Bomb>& bombs);

    // A rocket (a sphere) against the boss. Returns what happened, or nothing
    // when it missed (or the boss cannot be hurt right now).
    std::optional<BossHit> TryHit(Vector3 center, float radius);

    void Draw() const;
    void DrawGlow(const GlowRenderer& glow, const Camera3D& camera) const;   // between glow.Begin/End

    const char* Name() const;
    int   Points() const;
    float Health() const;          // 0 .. 1, for the health bar
    bool  Dying() const { return state_ == State::Dying; }
    bool  Angry() const;           // below half health
    bool  CoreOpen() const;
    Color GlowColor() const { return glow_; }
    Vector3 Position() const { return position_; }

private:
    enum class State { Entering, Fighting, Dying };
    enum class Attack { Fan, Aimed, Bombs, Rain, Spiral };

    // A shot that waits a moment before it is fired: bursts and rains are
    // a queue of these, so one attack can spread its shots over time.
    struct PendingShot {
        float   delay;       // seconds until it fires
        Vector3 offset;      // where it leaves the boss: a point of the model
        float   sideSpeed;   // < 0 left, > 0 right
        float   speed;       // toward the player
    };

    void StartAttack(Vector3 player, std::vector<Bomb>& bombs);
    Vector3 Mouth() const;   // where the shots leave the boss (a point of the model)
    Vector3 ToWorld(Vector3 local) const;   // a point of the model -> a point in the world
    Vector3 ToLocal(Vector3 world) const;   // and back

    int   level_;
    int   index_;            // which boss: 0, 1 or 2
    float scale_;
    int   hitPoints_;
    int   maxHitPoints_;
    Color body_;
    Color glow_;

    State state_ = State::Entering;
    float stateTime_ = 0.0f;   // seconds in the current state
    float time_      = 0.0f;   // seconds since it appeared (animations, sway)
    float flash_     = 0.0f;   // > 0 right after a hit: it lights up
    float attackTimer_ = 2.0f;
    int   attackStep_  = 0;    // where we are in this boss's attack pattern
    float blastTimer_  = 0.0f; // death sequence: next blast
    Vector3 position_{};
    float roll_ = 0.0f;        // tilt in degrees while sliding
    float sway_ = 0.0f;        // the side-to-side swing, an angle for sin()
    std::vector<PendingShot> pending_;
};
