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

// ---- Temporary test object (removed in M2, when the player ship arrives) -----
constexpr float kTestCubeSpeed = 1.5f;   // how fast the test cube swings (radians per second)

}  // namespace config
