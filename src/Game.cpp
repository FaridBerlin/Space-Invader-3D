// Game.cpp - the rules of the game.

#include "Game.h"

#include <algorithm>   // std::remove_if

#include "Config.h"
#include "Input.h"

Game::Game() {
    // The camera (explained in M1).
    camera_.position   = config::kCameraPosition;
    camera_.target     = config::kCameraTarget;
    camera_.up         = Vector3{0.0f, 1.0f, 0.0f};
    camera_.fovy       = config::kCameraFovDegrees;
    camera_.projection = CAMERA_PERSPECTIVE;
}

void Game::FireRocket() {
    // The rocket appears just in front of the ship's nose.
    rockets_.emplace_back(Vector3{player_.X(), config::kPlayerHeight, config::kRocketSpawnZ});
}

void Game::Update(float dt) {
    // ---- 1. Player ------------------------------------------------------------
    const PlayerInput input = ReadInput();
    player_.Update(dt, input);

    // ---- 2. Firing --------------------------------------------------------------
    // A cooldown limits the fire rate: it counts down, and we may only shoot
    // when it has reached zero. Holding the button fires every 0.2 seconds.
    fireCooldown_ -= dt;
    if (input.fire && fireCooldown_ <= 0.0f) {
        FireRocket();
        fireCooldown_ = config::kRocketFireInterval;
    }

    // ---- 3. Move everything ---------------------------------------------------
    for (auto& rocket : rockets_) {
        rocket.Update(dt);
    }
    invaders_.Update(dt);

    // ---- 4. Collisions: rocket against invader ----------------------------------
    for (auto& rocket : rockets_) {
        if (invaders_.TryHit(rocket.position, config::kRocketRadius)) {
            rocket.alive = false;                 // the rocket is used up
            score_ += config::kScorePerInvader;
        }
    }

    // ---- 5. Clean up dead rockets ------------------------------------------------
    // The "erase-remove idiom". remove_if shuffles all rockets we want to keep to
    // the front and returns where the leftover (dead) ones start; erase then cuts
    // them off. Removing AFTER the loops above, never during them, avoids
    // the classic bug of changing a list while walking through it.
    rockets_.erase(
        std::remove_if(rockets_.begin(), rockets_.end(),
                       [](const Rocket& r) { return !r.alive; }),
        rockets_.end());

    // ---- 6. New wave ------------------------------------------------------------
    if (invaders_.Empty()) {
        respawnTimer_ -= dt;
        if (respawnTimer_ <= 0.0f) {
            invaders_.Respawn();
        }
    } else {
        respawnTimer_ = config::kWaveRespawnDelay;
    }
}

void Game::Draw() const {
    BeginMode3D(camera_);

        DrawGrid(60, 1.0f);

        // Outline of the play field on the ground (y = 0).
        const float w = config::kFieldHalfWidth;
        const float l = config::kFieldLength;
        DrawLine3D(Vector3{-w, 0, 0},  Vector3{-w, 0, -l}, SKYBLUE);  // left edge
        DrawLine3D(Vector3{ w, 0, 0},  Vector3{ w, 0, -l}, SKYBLUE);  // right edge
        DrawLine3D(Vector3{-w, 0, -l}, Vector3{ w, 0, -l}, SKYBLUE);  // far edge
        DrawLine3D(Vector3{-w, 0, 0},  Vector3{ w, 0, 0},  SKYBLUE);  // near edge

        player_.Draw();
        invaders_.Draw();
        for (const auto& rocket : rockets_) {
            rocket.Draw();
        }

    EndMode3D();

    // 2D text on top of the 3D picture.
    DrawText(TextFormat("SCORE  %06d", score_), 20, 16, 32, RAYWHITE);
    DrawText("Move: A / D or arrows     Fire: Space (hold)", 20, 56, 20, LIGHTGRAY);
    DrawFPS(20, 84);
}
