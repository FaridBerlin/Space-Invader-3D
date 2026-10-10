// Player.cpp - how the ship moves and how it looks.

#include "Player.h"

#include <cmath>   // std::exp, std::sin

#include "Config.h"
#include "raylib.h"
#include "rlgl.h"  // raylib's low-level matrix functions (rlPushMatrix, ...)

// ---- A private helper, visible only inside this file --------------------------
// An anonymous namespace makes everything in it local to this .cpp file.
namespace {

// Draws a solid box with a darker wireframe outline, so the flat colours still
// show clear edges (we have no lighting yet).
void DrawBox(Vector3 center, Vector3 size, Color fill, Color edge) {
    DrawCube(center, size.x, size.y, size.z, fill);
    DrawCubeWires(center, size.x, size.y, size.z, edge);
}

}  // namespace

Player::Player() = default;   // all members already have starting values in Player.h

void Player::Update(float dt, const PlayerInput& input) {
    time_ += dt;

    // 1. What speed do we WANT? input.move is -1..+1, so this is -speed..+speed.
    const float targetVelocity = input.move * config::kPlayerSpeed;

    // 2. Move the real velocity toward the wanted one, smoothly, so the ship
    //    accelerates and brakes instead of jerking.
    //    "1 - exp(-k * dt)" is the share of the remaining gap we close this frame.
    //    Unlike a fixed factor such as 0.2, it gives the SAME feel at 30, 60 or
    //    144 FPS: another place where delta time keeps the game fair.
    const float blend = 1.0f - std::exp(-config::kPlayerResponsiveness * dt);
    velocityX_ += (targetVelocity - velocityX_) * blend;

    // 3. Move: distance = speed * time.
    x_ += velocityX_ * dt;

    // 4. Stay inside the play field. If we hit the wall, stop dead there.
    const float limit = config::kFieldHalfWidth - config::kPlayerHalfWidth;
    if (x_ > limit) {
        x_ = limit;
        velocityX_ = 0.0f;
    } else if (x_ < -limit) {
        x_ = -limit;
        velocityX_ = 0.0f;
    }
}

Vector3 Player::Position() const {
    return Vector3{x_, config::kPlayerHeight, 0.0f};
}

void Player::Draw() const {
    // Banking: the faster we slide sideways, the more the ship rolls.
    // The ship flies toward -Z, so "rolling" means rotating around the Z axis.
    // Positive angles tip the right wing UP when seen from behind, so we use the
    // negative sign to dip the wing on the side we are turning toward.
    const float bankDegrees =
        -(velocityX_ / config::kPlayerSpeed) * config::kPlayerMaxBankDegrees;

    // ---- Transform stack ---------------------------------------------------------
    // Instead of calculating the position of every part by hand, we MOVE and
    // ROTATE the whole coordinate system, then draw the ship around the origin.
    //   rlPushMatrix  = remember the current coordinate system
    //   rlTranslatef  = move the origin to the ship's position
    //   rlRotatef     = roll the whole system around the Z axis
    //   rlPopMatrix   = go back, so other objects are not affected
    // Every Draw call in between happens in the ship's own "local" space.
    rlPushMatrix();
    rlTranslatef(x_, config::kPlayerHeight, 0.0f);
    rlRotatef(bankDegrees, 0.0f, 0.0f, 1.0f);

    const Color hull{230, 235, 245, 255};   // near-white body
    const Color wing{255, 90, 30, 255};     // orange from the old 2D sprite
    const Color glass{60, 40, 200, 255};    // the sprite's blue-purple cockpit
    const Color edge{30, 30, 50, 255};      // dark outline

    // Local space: the ship nose points to -Z, the tail to +Z.
    DrawBox({0.0f, 0.0f, 0.0f}, {0.8f, 0.5f, 2.2f}, hull, edge);          // body

    // Nose cone: a cylinder whose end radius is 0 is a cone.
    DrawCylinderEx({0.0f, 0.0f, -1.1f}, {0.0f, 0.0f, -2.5f}, 0.4f, 0.0f, 12, hull);

    DrawBox({-0.95f, 0.0f, 0.4f}, {1.1f, 0.1f, 1.4f}, wing, edge);        // left wing
    DrawBox({ 0.95f, 0.0f, 0.4f}, {1.1f, 0.1f, 1.4f}, wing, edge);        // right wing
    DrawBox({-1.45f, 0.25f, 0.7f}, {0.1f, 0.6f, 0.6f}, wing, edge);       // left fin
    DrawBox({ 1.45f, 0.25f, 0.7f}, {0.1f, 0.6f, 0.6f}, wing, edge);       // right fin
    DrawBox({0.0f, 0.45f, 0.9f}, {0.1f, 0.6f, 0.7f}, wing, edge);         // tail fin

    DrawSphereEx({0.0f, 0.3f, -0.3f}, 0.32f, 12, 12, glass);              // cockpit

    // Engine flame: its length flickers using a fast sine wave.
    const float flameLength = 0.9f + 0.25f * std::sin(time_ * 40.0f);
    DrawCylinderEx({0.0f, 0.0f, 1.1f}, {0.0f, 0.0f, 1.1f + flameLength}, 0.28f, 0.0f, 10, ORANGE);
    DrawCylinderEx({0.0f, 0.0f, 1.1f}, {0.0f, 0.0f, 1.1f + flameLength * 0.6f}, 0.15f, 0.0f, 10, YELLOW);

    rlPopMatrix();
}
