// Game.cpp - the rules of the game.

#include "Game.h"

#include <algorithm>   // std::clamp, std::min, std::max
#include <cmath>       // sinf, std::fmod, std::ceil

#include "Config.h"
#include "Display.h"
#include "Input.h"
#include "Random.h"
#include "Util.h"
#include "rlgl.h"      // rlDisableDepthMask, rlDrawRenderBatchActive (see Game::Draw)

namespace {

// The look of the explosions. The NUMBERS are tuning values and live in
// Config.h; the COLOURS are art and live here. Every alien type bursts in its
// own two colours; the commander also bursts bigger.
ExplosionStyle AlienExplosion(AlienType type) {
    ExplosionStyle style{
        config::kInvaderExplosionCount, config::kInvaderExplosionSpeed,
        config::kInvaderExplosionLife,  config::kInvaderExplosionSize,
        Color{140,  90, 220, 255},   // Scarab: violet (its shell) ...
        Color{170, 255,  90, 255},   // ... and lime (its glowing lines)
    };
    switch (type) {
        case AlienType::Scarab:
            break;
        case AlienType::Reaper:
            style.colorA = Color{230,  40,  50, 255};   // red stripes
            style.colorB = Color{110, 110, 125, 255};   // dark metal
            break;
        case AlienType::Drone:
            style.colorA = Color{ 70, 195, 255, 255};   // cyan crystal
            style.colorB = Color{235, 250, 255, 255};   // white
            break;
        case AlienType::Commander:
            style.count *= 2;
            style.speed *= 1.3f;
            style.life  *= 1.2f;
            style.colorA = Color{255, 200,  60, 255};   // gold
            style.colorB = Color{255, 255, 230, 255};   // white-hot
            break;
    }
    return style;
}

// Points for destroying an alien of this type.
int ScoreFor(AlienType type) {
    switch (type) {
        case AlienType::Scarab:    return config::kScorePerInvader;
        case AlienType::Reaper:    return config::kScorePerReaper;
        case AlienType::Drone:     return config::kScorePerDrone;
        case AlienType::Commander: return config::kScorePerCommander;
    }
    return 0;   // not reached: the switch above covers every type
}

// A small spray of sparks when a commander is hit but survives.
ExplosionStyle HitSparks() {
    return ExplosionStyle{12, 6.0f, 0.35f, 0.2f,
                          Color{255, 220, 120, 255}, Color{255, 255, 255, 255}};
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

    // An invisible line (alpha 0) is not drawn at all. Drawing it would still
    // cost time, and (if depth writing were on) cut holes into the glow.
    auto drawLine = [](Vector3 from, Vector3 to, Color color) {
        if (color.a > 0) DrawLine3D(from, to, color);
    };
    for (int x = -halfWidth; x <= halfWidth; ++x) {
        const float fx = static_cast<float>(x);
        drawLine(Vector3{fx, 0.0f, -l}, Vector3{fx, 0.0f, 0.0f},
                 x == 0 ? centerColor : lineColor);
    }
    for (int z = -length; z <= 0; ++z) {
        const float fz = static_cast<float>(z);
        drawLine(Vector3{-w, 0.0f, fz}, Vector3{w, 0.0f, fz},
                 z == 0 ? centerColor : lineColor);
    }

    // Outline of the play field.
    const Color border = config::kFieldBorderColor;
    drawLine(Vector3{-w, 0, 0},  Vector3{-w, 0, -l}, border);  // left edge
    drawLine(Vector3{ w, 0, 0},  Vector3{ w, 0, -l}, border);  // right edge
    drawLine(Vector3{-w, 0, -l}, Vector3{ w, 0, -l}, border);  // far edge
    drawLine(Vector3{-w, 0, 0},  Vector3{ w, 0, 0},  border);  // near edge
}

// A tinted glass pane over the play field. It is two triangles (a rectangle) at y = 0
// with a see-through colour, so the stars underneath shine through it.
void DrawGlassFloor() {
    if (config::kGlassColor.a == 0) return;   // fully transparent: nothing to draw

    const float w = config::kFieldHalfWidth;
    const float l = config::kFieldLength;
    const Vector3 a{-w, 0.0f, 0.0f};    // near left
    const Vector3 b{ w, 0.0f, 0.0f};    // near right
    const Vector3 c{ w, 0.0f, -l};      // far right
    const Vector3 d{-w, 0.0f, -l};      // far left

    // Triangles are only drawn when their corners go counter-clockwise as seen
    // from the camera (above the floor), so the order a, b, c / a, c, d matters.
    DrawTriangle3D(a, b, c, config::kGlassColor);
    DrawTriangle3D(a, c, d, config::kGlassColor);
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
    powerUps_.clear();
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
    shieldTimer_      = 0.0f;
    rapidFireTimer_   = 0.0f;
    pickupTimer_      = 0.0f;
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

void Game::OnAlienDestroyed(const InvaderHit& hit) {
    score_ += ScoreFor(hit.type);
    particles_.Explode(hit.position, AlienExplosion(hit.type));
    const bool commander = (hit.type == AlienType::Commander);
    audio_.Play(commander ? Sfx::BombExplosion : Sfx::InvaderDestroyed);

    // Maybe drop a power-up capsule. Commanders always drop one; the others
    // with a small chance. RandomFloat(0, 1) < 0.08 is true 8% of the time.
    const bool lucky = commander || RandomFloat(0.0f, 1.0f) < config::kPowerUpDropChance;
    if (lucky && static_cast<int>(powerUps_.size()) < config::kMaxPowerUps) {
        powerUps_.emplace_back(hit.position, PowerUp::RandomType());
    }
}

void Game::Collect(PowerUpType type) {
    if (type == PowerUpType::Shield) {
        shieldTimer_ = config::kShieldDuration;
        pickupText_  = "SHIELD!";
    } else {
        rapidFireTimer_ = config::kRapidFireDuration;
        pickupText_     = "RAPID FIRE!";
    }
    pickupTimer_ = config::kPickupBannerTime;
    pickupColor_ = PowerUp(Vector3{}, type).MainColor();
    audio_.Play(Sfx::PowerUp);
}

bool Game::DamagePlayer(const char* reason) {
    if (shieldTimer_ > 0.0f) {
        // The shield absorbs the hit and breaks: a burst of cyan sparks.
        shieldTimer_ = 0.0f;
        particles_.Explode(player_.Position(),
                           ExplosionStyle{60, 10.0f, 0.7f, 0.25f,
                                          Color{80, 220, 255, 255}, Color{255, 255, 255, 255}});
        audio_.Play(Sfx::InvaderDestroyed);
        return false;
    }
    playerAlive_ = false;
    particles_.Explode(player_.Position(), PlayerExplosion());
    audio_.Play(Sfx::PlayerDeath);
    EndGame(reason);
    return true;
}

void Game::Update(float dt) {
    const PlayerInput input = ReadInput();
    if (input.toggleMute) {
        audio_.ToggleMute();
    }
    if (input.toggleFullscreen) {
        display::ToggleFullscreen();
    }
    // Esc: first step out of fullscreen; if we are not in fullscreen, quit.
    // These window keys work in EVERY state: menu, playing and game over.
    if (input.back) {
        if (display::IsFullscreen()) {
            display::ExitFullscreen();
        } else {
            quitRequested_ = true;
        }
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
    // With rapid fire the cooldown is shorter, so we shoot more often.
    fireCooldown_ -= dt;
    if (input.fire && fireCooldown_ <= 0.0f) {
        FireRocket();
        fireCooldown_ = (rapidFireTimer_ > 0.0f) ? config::kRapidFireInterval
                                                 : config::kRocketFireInterval;
    }

    // Power-up timers run down. (std::max keeps them from going below zero.)
    shieldTimer_    = std::max(0.0f, shieldTimer_ - dt);
    rapidFireTimer_ = std::max(0.0f, rapidFireTimer_ - dt);
    pickupTimer_    = std::max(0.0f, pickupTimer_ - dt);

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
    for (auto& powerUp : powerUps_) {
        powerUp.Update(dt);
    }
    invaders_.Update(dt, level_, player_.X());

    // ---- 4. Collisions ----------------------------------------------------------
    // 4a. Our rockets against the invaders.
    for (auto& rocket : rockets_) {
        if (const auto hit = invaders_.TryHit(rocket.position, config::kRocketRadius)) {
            rocket.alive = false;                 // the rocket is used up
            if (!hit->destroyed) {
                particles_.Explode(hit->position, HitSparks());   // damaged, not dead yet
                continue;
            }
            OnAlienDestroyed(*hit);
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
        if (playerAlive_ && shot.alive &&
            CheckCollisionSpheres(shot.position, config::kInvaderShotRadius,
                                  player_.Position(), config::kPlayerHitRadius)) {
            shot.alive = false;
            DamagePlayer("Your ship was hit!");
        }
    }

    // 4b2. Bombs against our ship.
    for (auto& bomb : bombs_) {
        if (bomb.alive && playerAlive_ &&
            CheckCollisionSpheres(bomb.position, bomb.radius,
                                  player_.Position(), config::kPlayerHitRadius)) {
            bomb.alive = false;
            particles_.Explode(bomb.position, BombExplosion(bomb.color));
            audio_.Play(Sfx::BombExplosion);
            DamagePlayer("A bomb destroyed your ship!");
        }
    }

    // 4b3. A diving alien crashing into our ship. The alien is always destroyed;
    // with a shield up we even get its points.
    if (playerAlive_) {
        if (const auto ram = invaders_.TryRam(player_.Position(), config::kPlayerHitRadius)) {
            if (!DamagePlayer("An alien crashed into your ship!")) {
                OnAlienDestroyed(*ram);
            } else {
                particles_.Explode(ram->position, AlienExplosion(ram->type));
            }
        }
    }

    // 4b4. Flying into a power-up capsule collects it.
    for (auto& powerUp : powerUps_) {
        if (playerAlive_ && powerUp.alive &&
            CheckCollisionSpheres(powerUp.position, config::kPowerUpRadius,
                                  player_.Position(), config::kPlayerHitRadius + 0.3f)) {
            powerUp.alive = false;
            Collect(powerUp.type);
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
    RemoveDead(powerUps_);

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

        // The floor (glass pane, grid, border) is decoration lying flat at y = 0.
        // It must NOT write into the depth buffer, the GPU's memory of "what is
        // in front": otherwise the glow squares, which reach below y = 0, get
        // cut off along the floor and along every grid line.
        //
        // Careful: raylib collects draw calls in a "batch" and sends them to the
        // GPU later. rlDisableDepthMask() acts IMMEDIATELY. So we first send
        // everything collected so far (rlDrawRenderBatchActive), switch depth
        // writing off, draw the floor, send THAT, and only then switch it back on.
        rlDrawRenderBatchActive();
        rlDisableDepthMask();
        DrawGlassFloor();   // after the stars (they shine through), before the grid lines
        DrawGround();
        rlDrawRenderBatchActive();
        rlEnableDepthMask();

        if (playerAlive_) {
            player_.Draw();
        }
        invaders_.Draw(alienModels_);
        for (const auto& rocket : rockets_) {
            rocket.Draw();
        }
        for (const auto& shot : invaderShots_) {
            shot.Draw();
        }
        for (const auto& bomb : bombs_) {
            bomb.Draw(time_);
        }
        for (const auto& powerUp : powerUps_) {
            powerUp.Draw();
        }

        // The shield: a wire bubble around the ship. In its last 3 seconds it
        // blinks (on for 0.15 s, off for 0.15 s) as a warning.
        const bool shieldVisible = playerAlive_ && shieldTimer_ > 0.0f &&
                                   (shieldTimer_ > 3.0f || std::fmod(shieldTimer_, 0.3f) > 0.15f);
        if (shieldVisible) {
            DrawSphereWires(player_.Position(), 1.9f, 6, 12, Fade(Color{80, 220, 255, 255}, 0.55f));
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
        for (const auto& powerUp : powerUps_) {
            glow_.Draw(camera_, powerUp.position, 2.6f, powerUp.MainColor());
        }
        if (shieldVisible) {
            glow_.Draw(camera_, player_.Position(), 5.0f, Color{60, 160, 255, 255});
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
    DrawText("Move: A / D or arrows     Fire: Space (hold)     Mute: M     Fullscreen: F     Window size: 1-5     Quit: Esc",
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

    // Active power-ups under the level, right-aligned, with the seconds left.
    auto drawRight = [&](const char* text, int y, Color color) {
        DrawText(text, GetScreenWidth() - Px(20) - MeasureText(text, Px(22)), y, Px(22), color);
    };
    int statusY = Px(84);   // below the help line
    if (shieldTimer_ > 0.0f) {
        drawRight(TextFormat("SHIELD %d", static_cast<int>(std::ceil(shieldTimer_))), statusY,
                  Color{80, 220, 255, 255});
        statusY += Px(28);
    }
    if (rapidFireTimer_ > 0.0f) {
        drawRight(TextFormat("RAPID FIRE %d", static_cast<int>(std::ceil(rapidFireTimer_))), statusY,
                  Color{255, 150, 40, 255});
    }

    // "SHIELD!" / "RAPID FIRE!" in the middle of the screen right after a pickup.
    if (state_ == GameState::Playing && pickupTimer_ > 0.0f) {
        const float alpha = std::min(1.0f, pickupTimer_ / 0.5f);   // fades out at the end
        const int size = Px(46);
        DrawText(pickupText_, (GetScreenWidth() - MeasureText(pickupText_, size)) / 2,
                 GetScreenHeight() * 2 / 5, size, Fade(pickupColor_, alpha));   // above the ship
    }

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
#if !defined(__EMSCRIPTEN__)
    drawCentered("Esc to quit", middle + Px(130), Px(22), GRAY);
#endif
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
    drawCentered("Fullscreen: F     Window size: 1 - 5     Quit: Esc", Px(415), Px(22), LIGHTGRAY);
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
