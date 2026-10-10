// Game.cpp - the rules of the game.

#include "Game.h"

#include <algorithm>   // std::remove_if

#include "Config.h"
#include "Input.h"

namespace {

// A function TEMPLATE: one definition that works for any type T. The compiler
// writes a separate version for Rocket and for InvaderShot, because we call it
// with both. T only has to have an "alive" flag.
//
// This is the erase-remove idiom: remove_if moves all items we want to keep to
// the front and returns where the leftover (dead) ones start; erase then cuts
// them off. We call it AFTER the loops that use the items, never during them,
// which avoids the classic bug of changing a list while walking through it.
template <typename T>
void RemoveDead(std::vector<T>& items) {
    items.erase(
        std::remove_if(items.begin(), items.end(),
                       [](const T& item) { return !item.alive; }),
        items.end());
}

}  // namespace

Game::Game() {
    // The camera (explained in M1).
    camera_.position   = config::kCameraPosition;
    camera_.target     = config::kCameraTarget;
    camera_.up         = Vector3{0.0f, 1.0f, 0.0f};
    camera_.fovy       = config::kCameraFovDegrees;
    camera_.projection = CAMERA_PERSPECTIVE;

    Restart();
}

void Game::Restart() {
    player_ = Player();            // a brand-new ship in the middle
    invaders_.Respawn();
    rockets_.clear();
    invaderShots_.clear();

    state_            = GameState::Playing;
    playerAlive_      = true;
    gameOverReason_   = "";
    score_            = 0;
    fireCooldown_     = 0.0f;
    invaderShotTimer_ = config::kInvaderShotInterval;
    respawnTimer_     = config::kWaveRespawnDelay;
}

void Game::EndGame(const char* reason) {
    if (state_ == GameState::GameOver) return;   // already over, keep the first reason
    state_          = GameState::GameOver;
    gameOverReason_ = reason;
}

void Game::FireRocket() {
    // The rocket appears just in front of the ship's nose.
    rockets_.emplace_back(Vector3{player_.X(), config::kPlayerHeight, config::kRocketSpawnZ});
}

void Game::Update(float dt) {
    const PlayerInput input = ReadInput();

    // What happens each frame depends on the state we are in.
    switch (state_) {
        case GameState::Playing:
            UpdatePlaying(dt, input);
            break;
        case GameState::GameOver:
            // The world is frozen. Wait for Enter.
            if (input.confirm) {
                Restart();
            }
            break;
    }
}

void Game::UpdatePlaying(float dt, const PlayerInput& input) {
    // ---- 1. Player and firing ---------------------------------------------------
    player_.Update(dt, input);

    // A cooldown limits the fire rate: it counts down, and we may only shoot
    // when it has reached zero. Holding the button fires every 0.2 seconds.
    fireCooldown_ -= dt;
    if (input.fire && fireCooldown_ <= 0.0f) {
        FireRocket();
        fireCooldown_ = config::kRocketFireInterval;
    }

    // ---- 2. The invaders fire back ------------------------------------------------
    if (!invaders_.Empty()) {
        invaderShotTimer_ -= dt;
        if (invaderShotTimer_ <= 0.0f) {
            // PickShooterPosition returns an optional. "if (auto p = ...; p)"
            // declares p and runs the block only when it holds a value.
            if (const auto shooter = invaders_.PickShooterPosition()) {
                // *shooter reads the value inside the optional.
                invaderShots_.emplace_back(
                    Vector3{shooter->x, shooter->y, shooter->z + 0.6f});
            }
            invaderShotTimer_ = config::kInvaderShotInterval;
        }
    }

    // ---- 3. Move everything ---------------------------------------------------
    for (auto& rocket : rockets_) {
        rocket.Update(dt);
    }
    for (auto& shot : invaderShots_) {
        shot.Update(dt);
    }
    invaders_.Update(dt);

    // ---- 4. Collisions ----------------------------------------------------------
    // 4a. Our rockets against the invaders.
    for (auto& rocket : rockets_) {
        if (invaders_.TryHit(rocket.position, config::kRocketRadius)) {
            rocket.alive = false;                 // the rocket is used up
            score_ += config::kScorePerInvader;
        }
    }

    // 4b. Invader shots against our ship.
    for (auto& shot : invaderShots_) {
        if (CheckCollisionSpheres(shot.position, config::kInvaderShotRadius,
                                  player_.Position(), config::kPlayerHitRadius)) {
            shot.alive   = false;
            playerAlive_ = false;
            EndGame("Your ship was hit!");
        }
    }

    // 4c. Did the wave reach us?
    if (!invaders_.Empty() && invaders_.FrontZ() >= config::kInvaderReachZ) {
        EndGame("The invaders reached you!");
    }

    // ---- 5. Clean up dead objects -------------------------------------------------
    RemoveDead(rockets_);
    RemoveDead(invaderShots_);

    // ---- 6. New wave ------------------------------------------------------------
    if (invaders_.Empty()) {
        respawnTimer_ -= dt;
        if (respawnTimer_ <= 0.0f) {
            invaders_.Respawn();
            invaderShotTimer_ = config::kInvaderShotInterval;   // a short grace period
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

        if (playerAlive_) {
            player_.Draw();
        }
        invaders_.Draw();
        for (const auto& rocket : rockets_) {
            rocket.Draw();
        }
        for (const auto& shot : invaderShots_) {
            shot.Draw();
        }

    EndMode3D();

    // 2D text on top of the 3D picture.
    DrawText(TextFormat("SCORE  %06d", score_), 20, 16, 32, RAYWHITE);
    DrawText("Move: A / D or arrows     Fire: Space (hold)", 20, 56, 20, LIGHTGRAY);
    DrawFPS(20, 84);

    if (state_ == GameState::GameOver) {
        DrawGameOver();
    }
}

void Game::DrawGameOver() const {
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();

    // Darken the whole picture. Fade(color, alpha) makes a colour see-through.
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.6f));

    // A small helper that centres a line of text horizontally.
    auto drawCentered = [&](const char* text, int y, int fontSize, Color color) {
        const int width = MeasureText(text, fontSize);
        DrawText(text, (screenW - width) / 2, y, fontSize, color);
    };

    const int middle = screenH / 2;
    drawCentered("GAME OVER", middle - 120, 80, RED);
    drawCentered(gameOverReason_, middle - 30, 30, RAYWHITE);
    drawCentered(TextFormat("Final score: %06d", score_), middle + 20, 36, YELLOW);
    drawCentered("Press ENTER to play again", middle + 90, 26, LIGHTGRAY);
}
