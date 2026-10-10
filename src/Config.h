// Config.h - every tuning number of the game in one place.
//
// A ".h" file is a "header": it is copied into every .cpp file that says
// #include "Config.h". Headers are how C++ files share declarations.

#pragma once   // "include me only once", even if several files ask for me

#include "raylib.h"

// A namespace is a named box. Writing config::kFieldHalfWidth makes it obvious
// where a value comes from and avoids name clashes with other code.
namespace config {

// ---- Window ---------------------------------------------------------------
constexpr int kScreenWidth  = 1280;
constexpr int kScreenHeight = 720;

// Longest time step we allow, in seconds. If the game stalls (window dragged,
// slow computer), dt can become huge and fast objects would jump straight
// through their targets. Capping dt keeps the simulation stable.
constexpr float kMaxFrameTime = 0.05f;

// ---- Play field -------------------------------------------------------------
// Coordinates (raylib is right-handed, Y is up):
//   X: left (-) / right (+)       the player moves along X
//   Y: down (-) / up (+)
//   Z: toward the camera (+) / away into the screen (-)   invaders come from -Z
// One unit is roughly "one metre". The player stands at Z = 0.
constexpr float kFieldHalfWidth = 12.0f;   // field spans X from -12 to +12
constexpr float kFieldLength    = 50.0f;   // field spans Z from 0 down to -50

// ---- Camera -------------------------------------------------------------------
// The camera sits behind and above the player, looking down the field.
constexpr Vector3 kCameraPosition{0.0f, 8.0f, 14.0f};
constexpr Vector3 kCameraTarget{0.0f, 0.0f, -12.0f};
constexpr float   kCameraFovDegrees = 60.0f;   // vertical field of view

// ---- Player ship --------------------------------------------------------------
constexpr float kPlayerSpeed          = 14.0f;  // top sideways speed, units per second
constexpr float kPlayerResponsiveness = 12.0f;  // higher = reaches top speed / stops faster
constexpr float kPlayerMaxBankDegrees = 35.0f;  // roll angle at top speed
constexpr float kPlayerHalfWidth      = 1.5f;   // wing tip to centre, used for the wall limit
constexpr float kPlayerHeight         = 0.6f;   // ship centre above the ground
constexpr float kPlayerHitRadius      = 1.1f;   // collision sphere of the ship (a bit smaller than the wings)

// ---- Rockets ------------------------------------------------------------------
constexpr float kRocketSpeed        = 35.0f;   // units per second, flying toward -Z
constexpr float kRocketFireInterval = 0.2f;    // seconds between shots (same as the 2D game)
constexpr float kRocketRadius       = 0.3f;    // size of the rocket's collision sphere
constexpr float kRocketSpawnZ       = -2.7f;   // just in front of the ship's nose

// ---- Invaders -----------------------------------------------------------------
constexpr int   kInvaderMinColumns = 5;        // a wave gets a random size, like the 2D game
constexpr int   kInvaderMaxColumns = 10;
constexpr int   kInvaderMinRows    = 2;
constexpr int   kInvaderMaxRows    = 5;
constexpr float kInvaderSpacingX   = 2.2f;     // distance between neighbours sideways
constexpr float kInvaderSpacingZ   = 2.4f;     // distance between rows in depth
constexpr float kInvaderHalfWidth  = 0.7f;     // used to stop at the field edge
constexpr float kInvaderRadius     = 0.8f;     // size of the collision sphere
constexpr float kInvaderHeight     = 0.9f;     // centre above the ground
constexpr float kInvaderSpeed      = 5.0f;     // sideways speed of the whole wave
constexpr float kInvaderStepZ      = 1.0f;     // how far the wave steps toward you at an edge
constexpr float kInvaderStartZ     = -30.0f;   // Z of the front row when a wave spawns
constexpr float kInvaderReachZ     = -2.5f;    // an invader this close to the ship = game over

// ---- Invader shots --------------------------------------------------------------
// The 2D game fired one shot every ~8 seconds. In 3D you can see shots coming and
// dodge them more easily, so we fire more often. Tune to taste!
constexpr float kInvaderShotInterval = 2.0f;   // seconds between shots from the wave
constexpr float kInvaderShotSpeed    = 14.0f;  // units per second, flying toward +Z (the player)
constexpr float kInvaderShotRadius   = 0.25f;  // collision sphere of a shot
constexpr float kInvaderShotDespawnZ = 6.0f;   // shots that fly past the ship are removed here

// ---- Starfield ------------------------------------------------------------------
constexpr int   kStarCount     = 260;
constexpr float kStarHalfWidth = 70.0f;    // stars spread from X = -70 to +70
constexpr float kStarMinY      = 1.5f;     // keep stars above the ground and out of the play area
constexpr float kStarMaxY      = 45.0f;
constexpr float kStarFarZ      = -130.0f;  // stars are born here ...
constexpr float kStarNearZ     = 13.0f;    // ... and die here, just in front of the camera
constexpr float kStarSpeed     = 28.0f;    // base speed toward the camera, units per second
constexpr float kStarStreak    = 0.9f;     // length of a star's streak

// ---- Particles and explosions ------------------------------------------------------
constexpr int   kMaxParticles  = 1500;     // safety cap (phones will thank us later)
constexpr float kParticleGravity = 5.0f;   // downward pull, units per second squared
constexpr float kParticleDrag    = 1.6f;   // air resistance, higher = slows faster

constexpr int   kInvaderExplosionCount = 40;
constexpr float kInvaderExplosionSpeed = 9.0f;
constexpr float kInvaderExplosionLife  = 0.9f;
constexpr float kInvaderExplosionSize  = 0.5f;

constexpr int   kPlayerExplosionCount  = 110;
constexpr float kPlayerExplosionSpeed  = 12.0f;
constexpr float kPlayerExplosionLife   = 1.6f;
constexpr float kPlayerExplosionSize   = 0.26f;

// ---- Score and waves ------------------------------------------------------------
constexpr int   kScorePerInvader   = 100;      // same as the 2D game
constexpr float kWaveRespawnDelay  = 1.5f;     // pause before a new wave appears
constexpr float kGameOverDelay     = 1.6f;     // time to watch the explosion before the Game Over screen

}  // namespace config
