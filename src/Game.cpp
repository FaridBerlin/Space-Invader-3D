// Game.cpp - the rules of the game.

#include "Game.h"

#include <algorithm>   // std::clamp
#include <cmath>       // sinf

#include "Config.h"
#include "Display.h"
#include "Input.h"
#include "Random.h"
#include "Util.h"
#include "rlgl.h"      // rlDisableDepthMask (the glass must not hide what is behind it)

namespace {

// The look of our two explosions. The NUMBERS are tuning values and live in
// Config.h; the COLOURS are art and live here.
ExplosionStyle InvaderExplosion() {
    return ExplosionStyle{
        config::kInvaderExplosionCount, config::kInvaderExplosionSpeed,
        config::kInvaderExplosionLife,  config::kInvaderExplosionSize,
        Color{190, 120, 255, 255},   // light purple (the invader's colour)
        Color{255, 210, 255, 255},   // hot pink-white
    };
}

ExplosionStyle PlayerExplosion() {
    return ExplosionStyle{
        config::kPlayerExplosionCount, config::kPlayerExplosionSpeed,
        config::kPlayerExplosionLife,  config::kPlayerExplosionSize,
        Color{255, 210, 70, 255},    // yellow-orange
        Color{255, 60, 30, 255},     // red
    };
}

// The bomb colour gets "hotter" with each level: yellow, orange, red.
Color BombColor(int level) {
    const Color colors[config::kLevelCount] = {YELLOW, ORANGE, RED};
    return colors[level];
}

ExplosionStyle BombExplosion(Color base) {
    return ExplosionStyle{
        config::kBombExplosionCount, config::kBombExplosionSpeed,
        config::kBombExplosionLife,  config::kBombExplosionSize,
        base,
        Color{255, 255, 230, 255},   // white-hot
    };
}

// The floor grid, cut to the play field: it covers exactly the area inside the
// blue border (X from -kFieldHalfWidth to +kFieldHalfWidth, Z from 0 to
// -kFieldLength). Outside it there is no floor, only space and stars.
// raylib has DrawGrid(), but we draw our own: it is just two loops, lines
// running into the distance (along Z) and lines running across (along X),
// one every unit. All lines stay in front of the camera (see Config.h).
void DrawGround() {
    const Color lineColor   = config::kGridLineColor;
    const Color centerColor = config::kGridCenterColor;
    const int   halfWidth = static_cast<int>(config::kFieldHalfWidth);
    const int   length    = static_cast<int>(config::kFieldLength);
    const float w = config::kFieldHalfWidth;
    const float l = config::kFieldLength;

    for (int x = -halfWidth; x <= halfWidth; ++x) {
        const float fx = static_cast<float>(x);
        DrawLine3D(Vector3{fx, 0.0f, -l}, Vector3{fx, 0.0f, 0.0f},
                   x == 0 ? centerColor : lineColor);
    }
    for (int z = -length; z <= 0; ++z) {
        const float fz = static_cast<float>(z);
        DrawLine3D(Vector3{-w, 0.0f, fz}, Vector3{w, 0.0f, fz},
                   z == 0 ? centerColor : lineColor);
    }
}

// A tinted glass pane over the play field. It is two triangles (a rectangle) at y = 0
// with a see-through colour, so the stars underneath shine through it. We switch off
// "depth writing" while drawing it: otherwise the pane would count as a solid wall
// and anything drawn after it behind it would be cut away.
void DrawGlassFloor() {
    const float w = config::kFieldHalfWidth;
    const float l = config::kFieldLength;
    const Vector3 a{-w, 0.0f, 0.0f};    // near left
    const Vector3 b{ w, 0.0f, 0.0f};    // near right
    const Vector3 c{ w, 0.0f, -l};      // far right
    const Vector3 d{-w, 0.0f, -l};      // far left

    rlDisableDepthMask();
    // Triangles are only drawn when their corners go counter-clockwise as seen
    // from the camera (above the floor), so the order a, b, c / a, c, d matters.
    DrawTriangle3D(a, b, c, config::kGlassColor);
    DrawTriangle3D(a, c, d, config::kGlassColor);
    rlEnableDepthMask();
}

// Which level belongs to this score? Walk the thresholds, keep the highest one reached.
int LevelForScore(int score) {
    int level = 0;
    for (int i = 1; i < config::kLevelCount; ++i) {
        if (score >= config::kLevelStartScore[i]) {
            level = i;
        }
    }
    return level;
}

}  // namespace

Game::Game() {
    // The camera (explained in M1).
    camera_.position   = config::kCameraPosition;
    camera_.target     = config::kCameraTarget;
    camera_.up         = Vector3{0.0f, 1.0f, 0.0f};
    camera_.fovy       = config::kCameraFovDegrees;
    camera_.projection = CAMERA_PERSPECTIVE;

    Restart();                 // builds a fresh game ...
    state_ = GameState::Menu;  // ... but we wait on the title screen until Enter is pressed
}

void Game::Restart() {
    player_ = Player();            // a brand-new ship in the middle
    invaders_.Respawn();
    rockets_.clear();
    invaderShots_.clear();
    bombs_.clear();
    particles_.Clear();

    state_            = GameState::Playing;
    playerAlive_      = true;
    gameOverReason_   = "";
    gameOverTime_     = 0.0f;
    score_            = 0;
    level_            = 0;
    levelBannerTimer_ = config::kLevelBannerTime;   // announce "LEVEL 1"
    fireCooldown_     = 0.0f;
    invaderShotTimer_ = config::kInvaderShotInterval;
    bombTimer_        = config::kBombInterval;
    respawnTimer_     = config::kWaveRespawnDelay;
}

void Game::EndGame(const char* reason) {
    if (state_ == GameState::GameOver) return;   // already over, keep the first reason
    state_          = GameState::GameOver;
    gameOverReason_ = reason;
    gameOverTime_   = 0.0f;
}

void Game::FireRocket() {
    // A higher ship tier fires more rockets at once, from different points
    // (nose and wing cannons). The table is in Config.h.
    const int tier = player_.Tier();
    for (int i = 0; i < config::kTierShotCount[tier]; ++i) {
        const config::ShotPoint& point = config::kTierShots[tier][i];
        rockets_.emplace_back(Vector3{player_.X() + point.x, config::kPlayerHeight, point.z}, tier);
    }
    audio_.Play(Sfx::PlayerShot);   // one sound per volley, so it does not get louder and louder
}

void Game::Update(float dt) {
    const PlayerInput input = ReadInput();
    if (input.toggleMute) {
        audio_.ToggleMute();
    }
    if (input.toggleFullscreen) {
        display::ToggleFullscreen();
    }
    if (input.windowPreset >= 0) {
        display::ApplyPreset(input.windowPreset);
    }

    // The background and the explosions keep moving in EVERY state, so the
    // death explosion plays out nicely before the Game Over screen appears.
    stars_.Update(dt);
    particles_.Update(dt);
    glow_.Update(dt);
    time_ += dt;

    // What else happens each frame depends on the state we are in.
    switch (state_) {
        case GameState::Menu:
            if (input.confirm) {
                Restart();
            }
            break;
        case GameState::Playing:
            UpdatePlaying(dt, input);
            break;
        case GameState::GameOver:
            // The world is frozen. After a short pause, wait for Enter.
            gameOverTime_ += dt;
            if (gameOverTime_ >= config::kGameOverDelay && input.confirm) {
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
            // PickShooterPosition returns an optional. "if (const auto s = ...)"
            // declares s and runs the block only when it holds a value.
            if (const auto shooter = invaders_.PickShooterPosition()) {
                // *shooter reads the value inside the optional.
                invaderShots_.emplace_back(
                    Vector3{shooter->x, shooter->y, shooter->z + 0.6f},
                    config::kLevelShotSpeed[level_]);
                audio_.Play(Sfx::InvaderShot);
            }
            invaderShotTimer_ = config::kInvaderShotInterval;
        }
    }

    // ---- 2b. Bombs appear every few seconds -----------------------------------------
    bombTimer_ -= dt;
    if (bombTimer_ <= 0.0f) {
        // Aimed roughly at where the ship is NOW, so the player has to react.
        // std::clamp keeps the bomb inside the walls.
        const float x = std::clamp(player_.X() + RandomFloat(-config::kBombAimSpread, config::kBombAimSpread),
                                   -config::kFieldHalfWidth + 1.0f, config::kFieldHalfWidth - 1.0f);
        bombs_.emplace_back(Vector3{x, config::kBombSpawnY, config::kBombSpawnZ},
                            config::kLevelBombSpeed[level_],
                            config::kLevelBombRadius[level_],
                            BombColor(level_));
        bombTimer_ = config::kBombInterval;
    }

    // ---- 3. Move everything ---------------------------------------------------
    for (auto& rocket : rockets_) {
        rocket.Update(dt);
    }
    for (auto& shot : invaderShots_) {
        shot.Update(dt);
    }
    for (auto& bomb : bombs_) {
        bomb.Update(dt);
    }
    invaders_.Update(dt);

    // ---- 4. Collisions ----------------------------------------------------------
    // 4a. Our rockets against the invaders.
    for (auto& rocket : rockets_) {
        if (const auto hit = invaders_.TryHit(rocket.position, config::kRocketRadius)) {
            rocket.alive = false;                 // the rocket is used up
            score_ += config::kScorePerInvader;
            particles_.Explode(*hit, InvaderExplosion());   // *hit = where the invader was
            audio_.Play(Sfx::InvaderDestroyed);
        }
    }

    // 4a2. Our rockets against the bombs. A bomb is worth more than an invader and
    // explodes bigger. "continue" skips rockets that were already used up above.
    for (auto& rocket : rockets_) {
        if (!rocket.alive) continue;
        for (auto& bomb : bombs_) {
            if (bomb.alive && CheckCollisionSpheres(rocket.position, config::kRocketRadius,
                                                    bomb.position, bomb.radius)) {
                rocket.alive = false;
                bomb.alive   = false;
                score_ += config::kScorePerBomb;
                particles_.Explode(bomb.position, BombExplosion(bomb.color));
                audio_.Play(Sfx::BombExplosion);
                break;   // this rocket is gone: stop looking at more bombs
            }
        }
    }

    // 4a'. Did the score just reach the next level? Levels only go up.
    const int newLevel = LevelForScore(score_);
    if (newLevel > level_) {
        level_            = newLevel;
        levelBannerTimer_ = config::kLevelBannerTime;

        // Level up = ship upgrade: new look, and a burst of sparkles in the new
        // shot colour around the ship.
        player_.SetTier(level_);
        particles_.Explode(player_.Position(),
                           ExplosionStyle{70, 9.0f, 1.0f, 0.3f,
                                          config::kTierShotColor[level_], Color{255, 255, 255, 255}});
    }
    if (levelBannerTimer_ > 0.0f) {
        levelBannerTimer_ -= dt;
    }

    // 4b. Invader shots against our ship.
    for (auto& shot : invaderShots_) {
        if (CheckCollisionSpheres(shot.position, config::kInvaderShotRadius,
                                  player_.Position(), config::kPlayerHitRadius)) {
            shot.alive   = false;
            playerAlive_ = false;
            particles_.Explode(player_.Position(), PlayerExplosion());
            audio_.Play(Sfx::PlayerDeath);
            EndGame("Your ship was hit!");
        }
    }

    // 4b2. Bombs against our ship.
    for (auto& bomb : bombs_) {
        if (bomb.alive && playerAlive_ &&
            CheckCollisionSpheres(bomb.position, bomb.radius,
                                  player_.Position(), config::kPlayerHitRadius)) {
            bomb.alive   = false;
            playerAlive_ = false;
            particles_.Explode(bomb.position, BombExplosion(bomb.color));
            particles_.Explode(player_.Position(), PlayerExplosion());
            audio_.Play(Sfx::BombExplosion);
            audio_.Play(Sfx::PlayerDeath);
            EndGame("A bomb destroyed your ship!");
        }
    }

    // 4c. Did the wave reach us?
    if (!invaders_.Empty() && invaders_.FrontZ() >= config::kInvaderReachZ) {
        EndGame("The invaders reached you!");
    }

    // ---- 5. Clean up dead objects -------------------------------------------------
    RemoveDead(rockets_);
    RemoveDead(invaderShots_);
    RemoveDead(bombs_);

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

        // Draw order matters: far things first, see-through effects last.
        stars_.Draw();

        DrawGlassFloor();   // after the stars (they shine through), before the grid lines
        DrawGround();

        // Outline of the play field on the ground (y = 0).
        const float w = config::kFieldHalfWidth;
        const float l = config::kFieldLength;
        const Color border = config::kFieldBorderColor;
        DrawLine3D(Vector3{-w, 0, 0},  Vector3{-w, 0, -l}, border);  // left edge
        DrawLine3D(Vector3{ w, 0, 0},  Vector3{ w, 0, -l}, border);  // right edge
        DrawLine3D(Vector3{-w, 0, -l}, Vector3{ w, 0, -l}, border);  // far edge
        DrawLine3D(Vector3{-w, 0, 0},  Vector3{ w, 0, 0},  border);  // near edge

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
        for (const auto& bomb : bombs_) {
            bomb.Draw(time_);
        }

        // Glow around the projectiles, drawn by our custom shader. Like the
        // particles it is additive and see-through, so it comes after the solid objects.
        glow_.Begin();
        for (const auto& rocket : rockets_) {
            glow_.Draw(camera_, rocket.position, config::kRocketGlowSize,
                       rocket.color);               // same colour as the rocket
        }
        for (const auto& shot : invaderShots_) {
            glow_.Draw(camera_, shot.position, config::kInvaderShotGlowSize,
                       Color{255, 220, 60, 255});   // yellow
        }
        for (const auto& bomb : bombs_) {
            glow_.Draw(camera_, bomb.position, bomb.radius * config::kBombGlowFactor, bomb.color);
        }
        glow_.End();

        particles_.Draw();   // last: additive blending must be drawn on top of the solid objects

    EndMode3D();

    if (state_ == GameState::Menu) {
        DrawMenu();
        return;   // no score or banner on the title screen
    }

    // 2D text on top of the 3D picture (the HUD = "heads-up display").
    // Px() scales every size and position with the window height, so the text
    // stays the same size relative to the picture in a small and in a huge window.
    using display::Px;
    DrawText(TextFormat("SCORE  %06d", score_), Px(20), Px(16), Px(32), RAYWHITE);
#if defined(__EMSCRIPTEN__)
    DrawText("Move: A / D or arrows     Fire: Space (hold)     Mute: M",
             Px(20), Px(56), Px(20), LIGHTGRAY);
#else
    DrawText("Move: A / D or arrows     Fire: Space (hold)     Mute: M     Fullscreen: F     Window size: 1-5",
             Px(20), Px(56), Px(20), LIGHTGRAY);
#endif
    DrawText(TextFormat("%d FPS", GetFPS()), Px(20), Px(84), Px(20), GREEN);
    if (audio_.Muted()) {
        DrawText("SOUND OFF", Px(20), Px(112), Px(20), ORANGE);
    }

    // Level in the top right corner, right-aligned: measure the text, then
    // start that far left of the screen edge.
    const char* levelText = TextFormat("LEVEL %d", level_ + 1);
    DrawText(levelText, GetScreenWidth() - Px(20) - MeasureText(levelText, Px(32)), Px(16), Px(32), SKYBLUE);

    if (state_ == GameState::Playing && levelBannerTimer_ > 0.0f) {
        DrawLevelBanner();
    }

    if (state_ == GameState::GameOver && gameOverTime_ >= config::kGameOverDelay) {
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

    using display::Px;
    const int middle = screenH / 2;
    drawCentered("GAME OVER", middle - Px(120), Px(80), RED);
    drawCentered(gameOverReason_, middle - Px(30), Px(30), RAYWHITE);
    drawCentered(TextFormat("Final score: %06d", score_), middle + Px(20), Px(36), YELLOW);
    drawCentered("Press ENTER to play again", middle + Px(90), Px(26), LIGHTGRAY);
}

void Game::DrawMenu() const {
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.55f));

    auto drawCentered = [&](const char* text, int y, int fontSize, Color color) {
        const int width = MeasureText(text, fontSize);
        DrawText(text, (screenW - width) / 2, y, fontSize, color);
    };

    // A gentle pulse for the "press Enter" line: sin() swings between -1 and +1,
    // we turn that into an opacity between 0.4 and 1.
    const float pulse = 0.7f + 0.3f * sinf(static_cast<float>(GetTime()) * 4.0f);

    using display::Px;
    drawCentered("SPACE INVADER", Px(70), Px(90), Color{190, 120, 255, 255});
    drawCentered("3D", Px(160), Px(90), SKYBLUE);
    drawCentered("Press ENTER to start", Px(330), Px(32), Fade(RAYWHITE, pulse));
    drawCentered("Move: A / D or arrows     Fire: Space (hold)", Px(380), Px(22), LIGHTGRAY);
#if !defined(__EMSCRIPTEN__)
    drawCentered("Fullscreen: F     Window size: 1 - 5", Px(415), Px(22), LIGHTGRAY);
#endif
}

void Game::DrawLevelBanner() const {
    // Fade out during the last half second of the banner time.
    const float alpha = levelBannerTimer_ > 0.5f ? 1.0f : levelBannerTimer_ / 0.5f;
    const char* text = TextFormat("LEVEL %d", level_ + 1);
    const int fontSize = display::Px(70);
    DrawText(text, (GetScreenWidth() - MeasureText(text, fontSize)) / 2,
             GetScreenHeight() / 4, fontSize, Fade(SKYBLUE, alpha));
}
