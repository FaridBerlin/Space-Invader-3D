// Game.cpp - the rules of the game.

#include "Game.h"

#include <algorithm>   // std::clamp, std::min, std::max
#include <cmath>       // sinf, std::fmod, std::ceil

#include "Config.h"
#include "Display.h"
#include "Storage.h"
#include "Input.h"
#include "LowPoly.h"
#include "Random.h"
#include "Util.h"
#include "raymath.h"   // Vector3Add, Vector3Distance
#include "rlgl.h"      // rlDisableDepthMask, rlDrawRenderBatchActive (see Game::Draw)

namespace {

// The look of the explosions. The NUMBERS are tuning values and live in
// Config.h; the COLOURS are art and live here. Every alien type bursts in its
// own two colours; the commander also bursts bigger.
// The colours come from the level's alien colours (see Invader.cpp), so a
// wave explodes in the colours it was painted in.
ExplosionStyle AlienExplosion(AlienType type, int level) {
    const LevelAlienColors palette = AlienColorsForLevel(level);
    const Color bodyLight = lowpoly::Mix(palette.body, Color{255, 255, 255, 255}, 0.35f);
    ExplosionStyle style{
        config::kInvaderExplosionCount, config::kInvaderExplosionSpeed,
        config::kInvaderExplosionLife,  config::kInvaderExplosionSize,
        bodyLight,       // Scarab: its shell ...
        palette.glow,    // ... and its glowing lines
    };
    switch (type) {
        case AlienType::Scarab:
            break;
        case AlienType::Reaper:
            style.colorA = palette.glow;                  // stripes
            style.colorB = Color{110, 110, 125, 255};     // dark metal
            break;
        case AlienType::Drone:
            style.colorA = palette.glow;                  // crystal
            style.colorB = Color{235, 250, 255, 255};     // white
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


}  // namespace

Game::Game() {
    // The camera (explained in M1).
    camera_.position   = config::kCameraPosition;
    camera_.target     = config::kCameraTarget;
    camera_.up         = Vector3{0.0f, 1.0f, 0.0f};
    camera_.fovy       = config::kCameraFovDegrees;
    camera_.projection = CAMERA_PERSPECTIVE;

    highScore_ = storage::LoadInt("highscore");   // the best score from earlier games (0 the first time)
    audio_.SetMuted(storage::LoadInt("muted") != 0);   // muted last time? Then stay muted.
    touchMode_ = IsTouchDevice();     // phones and tablets start with the touch hints

    Restart();                 // builds a fresh game ...
    state_ = GameState::Menu;  // ... but we wait on the title screen until Enter is pressed
}

void Game::Restart() {
    player_ = Player();            // a brand-new ship in the middle
    invaders_.Respawn(0);          // the first wave of level 1
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
    wave_             = 0;
    levelBannerTimer_ = config::kLevelBannerTime;   // announce "LEVEL 1"
    waveBannerTimer_  = 0.0f;
    bannerTitle_      = "LEVEL 1";
    bannerSubtitle_   = config::kLevelIntro[0];
    stars_.SetColor(config::kLevelStarColor[0]);
    nebula_.SetColors(config::kLevelNebulaA[0], config::kLevelNebulaB[0]);
    fireCooldown_     = 0.0f;
    invaderShotTimer_ = config::kLevelShotInterval[0];
    bombTimer_        = config::kBombInterval;
    respawnTimer_     = config::kWaveRespawnDelay;
    lives_            = config::kStartLives;
    playerRespawnTimer_ = 0.0f;
    invincibleTimer_  = 0.0f;
    newHighScore_     = false;
    shieldTimer_      = 0.0f;
    rapidFireTimer_   = 0.0f;
    pickupTimer_      = 0.0f;
    helpTimer_        = config::kHelpShowTime;   // show the keys for the first seconds
    boss_.reset();
    bossDefeated_     = false;
    shake_            = 0.0f;
    comboCount_       = 0;
    comboTimer_       = 0.0f;
    bonusTimer_       = 0.0f;
    StartWaveStats();
}

void Game::EndGame(const char* reason) {
    if (state_ == GameState::GameOver) return;   // already over, keep the first reason
    state_          = GameState::GameOver;
    gameOverReason_ = reason;
    gameOverTime_   = 0.0f;

    // A new record? Remember it now, so it survives closing the game.
    if (score_ > highScore_) {
        highScore_    = score_;
        newHighScore_ = true;
        storage::SaveInt("highscore", highScore_);
    }
}

void Game::NextWave() {
    const int lastLevel = config::kLevelCount - 1;

    // The last wave of a level with a boss is done: now the boss comes.
    if (wave_ + 1 >= config::kWavesPerLevel && config::kLevelBoss[level_] >= 0 && !bossDefeated_) {
        StartBoss();
        return;
    }

    if (wave_ + 1 < config::kWavesPerLevel || level_ == lastLevel) {
        // ---- The next wave of the same level -------------------------------------
        ++wave_;
        waveBannerTimer_ = config::kWaveBannerTime;

        // The last wave of the last level is done: the whole game is beaten!
        // From here on, endless bonus waves at full speed.
        if (level_ == lastLevel && wave_ == config::kWavesPerLevel) {
            score_ += config::kAllLevelsBonus;
            bannerTitle_      = TextFormat("ALL %d LEVELS CLEARED!", config::kLevelCount);
            bannerSubtitle_   = TextFormat("+%d bonus. Endless bonus waves from here!", config::kAllLevelsBonus);
            levelBannerTimer_ = config::kLevelBannerTime + 1.0f;
            waveBannerTimer_  = 0.0f;
        }
    } else {
        // ---- The next level ----------------------------------------------------
        ++level_;
        wave_ = 0;
        bossDefeated_ = false;
        bannerTitle_      = TextFormat("LEVEL %d", level_ + 1);
        bannerSubtitle_   = config::kLevelIntro[level_];
        levelBannerTimer_ = config::kLevelBannerTime;
        stars_.SetColor(config::kLevelStarColor[level_]);   // the new colour mood
        nebula_.SetColors(config::kLevelNebulaA[level_], config::kLevelNebulaB[level_]);

        // Some levels upgrade the ship (a bigger tier, or the new "Claw" design):
        // new look, and a burst of sparkles in the new shot colour around it.
        const int  tier = config::kLevelShipTier[level_];
        const bool hero = config::kLevelHeroShip[level_];
        if (tier != player_.Tier() || hero != player_.Hero()) {
            player_.SetTier(tier);
            player_.SetHero(hero);
            if (playerAlive_) {
                particles_.Explode(player_.Position(),
                                   ExplosionStyle{70, 9.0f, 1.0f, 0.3f,
                                                  ShotColor(), Color{255, 255, 255, 255}});
            }
        }
    }

    invaders_.Respawn(level_);
    invaderShotTimer_ = config::kLevelShotInterval[level_];   // a short grace period
    StartWaveStats();
}

void Game::StartWaveStats() {
    waveActive_  = true;
    waveTime_    = 0.0f;
    waveAliens_  = invaders_.Count();
    hitThisWave_ = false;
}

void Game::AwardWaveBonus() {
    waveActive_ = false;
    // Faster than par: 25 points for every second saved, rounded down to tens.
    const float par = config::kWaveParPerAlien * static_cast<float>(waveAliens_);
    const int speedBonus = static_cast<int>(std::max(0.0f, par - waveTime_) *
                                            static_cast<float>(config::kSpeedBonusPerSecond)) / 10 * 10;
    const int noHitBonus = hitThisWave_ ? 0 : config::kNoHitBonus;
    if (speedBonus + noHitBonus == 0) return;
    score_ += speedBonus + noHitBonus;

    // Build the text from the parts that were earned. std::string can be added
    // together with +, and std::to_string turns a number into text.
    bonusText_.clear();
    if (speedBonus > 0) bonusText_ += "SPEED BONUS +" + std::to_string(speedBonus);
    if (speedBonus > 0 && noHitBonus > 0) bonusText_ += "     ";
    if (noHitBonus > 0) bonusText_ += "NO HIT +" + std::to_string(noHitBonus);
    bonusTimer_ = config::kBonusTextTime;
}

void Game::AddShake(float amount) {
    shake_ = std::min(1.0f, shake_ + amount);
}

int Game::ComboMultiplier() const {
    // 1 for the first 5 kills, 2 from the 6th, 3 from the 11th, at most 4.
    return std::min(1 + std::max(comboCount_ - 1, 0) / config::kComboStep, config::kComboMaxMultiplier);
}

void Game::StartBoss() {
    boss_.emplace(level_);   // builds the Boss right inside the optional
    bannerTitle_      = boss_->Name();
    bannerSubtitle_   = "Boss fight! Hit its core when it opens and glows.";
    levelBannerTimer_ = config::kLevelBannerTime;
    waveBannerTimer_  = 0.0f;
    bombs_.clear();          // a clean start: the boss brings its own bombs
    audio_.Play(Sfx::BombExplosion);
}

void Game::UpdateBoss(float dt) {
    if (!boss_) return;
    const BossEvents events = boss_->Update(dt, player_.X(), invaderShots_, bombs_);
    if (events.shot) {
        audio_.Play(Sfx::InvaderShot);
    }
    // The death sequence: a chain of blasts in the boss's colours.
    for (const Vector3& blast : events.blasts) {
        particles_.Explode(blast, ExplosionStyle{60, 10.0f, 0.9f, 0.4f, boss_->GlowColor(),
                                                 Color{255, 240, 200, 255}});
        audio_.Play(Sfx::InvaderDestroyed);
        AddShake(config::kShakeBossBlast);
    }
    if (events.finished) {
        // The final blast, the points, a reward, and the way to the next level.
        const Vector3 center = boss_->Position();
        for (int i = 0; i < 4; ++i) {
            particles_.Explode(center, BombExplosion(boss_->GlowColor()));
        }
        audio_.Play(Sfx::BombExplosion);
        AddShake(config::kShakeBossDeath);
        score_ += boss_->Points();
        bannerTitle_      = "BOSS DEFEATED!";
        bannerSubtitle_   = TextFormat("+%d points", boss_->Points());
        levelBannerTimer_ = config::kBossOutroTime - 0.3f;
        // Two power-ups fall out of the wreck.
        powerUps_.emplace_back(Vector3{center.x - 2.0f, center.y, center.z}, PowerUp::RandomType());
        powerUps_.emplace_back(Vector3{center.x + 2.0f, center.y, center.z}, PowerUp::RandomType());
        boss_.reset();
        bossDefeated_ = true;
        respawnTimer_ = config::kBossOutroTime;
    }
}

void Game::RespawnPlayer() {
    player_ = Player();           // a fresh ship in the middle ...
    player_.SetTier(config::kLevelShipTier[level_]);   // ... with the upgrade of this level
    player_.SetHero(config::kLevelHeroShip[level_]);
    playerAlive_     = true;
    invincibleTimer_ = config::kInvincibleTime;
    fireCooldown_    = 0.0f;
    // Fair play: remove the shots and bombs that are already on their way, so the
    // new ship does not appear in the middle of the fire that destroyed the last one.
    invaderShots_.clear();
    bombs_.clear();
}

Color Game::ShotColor() const {
    return player_.Hero() ? config::kHeroShotColor : config::kTierShotColor[player_.Tier()];
}

void Game::FireRocket() {
    // A higher ship tier fires more rockets at once, from different points
    // (nose and wing cannons, or the Claw's claw tips). The tables are in Config.h.
    const int tier = player_.Tier();
    // A reference to one row of a 2D array: "the 3 shot points of this ship".
    const config::ShotPoint (&points)[3] = player_.Hero() ? config::kHeroShots[tier] : config::kTierShots[tier];
    for (int i = 0; i < config::kTierShotCount[tier]; ++i) {
        rockets_.emplace_back(Vector3{player_.X() + points[i].x, config::kPlayerHeight, points[i].z},
                              ShotColor());
    }
    audio_.Play(Sfx::PlayerShot);   // one sound per volley, so it does not get louder and louder
}

void Game::OnAlienDestroyed(const InvaderHit& hit) {
    // Combo: a kill within the time window continues it, otherwise a new one starts.
    comboCount_ = (comboTimer_ > 0.0f) ? comboCount_ + 1 : 1;
    comboTimer_ = config::kComboWindow;
    score_ += ScoreFor(hit.type) * ComboMultiplier();
    particles_.Explode(hit.position, AlienExplosion(hit.type, level_));
    const bool commander = (hit.type == AlienType::Commander);
    if (commander) AddShake(config::kShakeCommander);
    audio_.Play(commander ? Sfx::BombExplosion : Sfx::InvaderDestroyed);

    // Maybe drop a power-up capsule. Commanders always drop one; the others
    // with a small chance. RandomFloat(0, 1) < 0.08 is true 8% of the time.
    const bool lucky = commander || RandomFloat(0.0f, 1.0f) < config::kPowerUpDropChance;
    if (lucky && static_cast<int>(powerUps_.size()) < config::kMaxPowerUps) {
        powerUps_.emplace_back(hit.position, PowerUp::RandomType());
    }
}

void Game::Collect(PowerUpType type) {
    switch (type) {
        case PowerUpType::Shield:
            shieldTimer_ = config::kShieldDuration;
            pickupText_  = "SHIELD!";
            break;
        case PowerUpType::RapidFire:
            rapidFireTimer_ = config::kRapidFireDuration;
            pickupText_     = "RAPID FIRE!";
            break;
        case PowerUpType::ExtraLife:
            lives_      = std::min(lives_ + 1, config::kMaxLives);
            pickupText_ = "EXTRA LIFE!";
            break;
    }
    pickupTimer_ = config::kPickupBannerTime;
    pickupColor_ = PowerUp(Vector3{}, type).MainColor();
    audio_.Play(Sfx::PowerUp);
}

bool Game::DamagePlayer(const char* reason) {
    if (invincibleTimer_ > 0.0f) {
        return false;   // a fresh ship cannot be hit while it blinks
    }
    if (shieldTimer_ > 0.0f) {
        // The shield absorbs the hit and breaks: a burst of cyan sparks.
        shieldTimer_ = 0.0f;
        particles_.Explode(player_.Position(),
                           ExplosionStyle{60, 10.0f, 0.7f, 0.25f,
                                          Color{80, 220, 255, 255}, Color{255, 255, 255, 255}});
        audio_.Play(Sfx::InvaderDestroyed);
        AddShake(config::kShakeShieldBreak);
        return false;
    }
    // The ship is destroyed. Its power-ups (and the combo) are lost with it.
    AddShake(config::kShakePlayerHit);
    hitThisWave_ = true;
    comboCount_  = 0;
    comboTimer_  = 0.0f;
    playerAlive_    = false;
    shieldTimer_    = 0.0f;
    rapidFireTimer_ = 0.0f;
    particles_.Explode(player_.Position(), PlayerExplosion());
    audio_.Play(Sfx::PlayerDeath);

    lives_ -= 1;
    if (lives_ > 0) {
        playerRespawnTimer_ = config::kPlayerRespawnDelay;   // the next ship comes soon
    } else {
        EndGame(reason);   // that was the last one
    }
    return true;
}

void Game::Update(float dt) {
    const PlayerInput input = ReadInput();
    // Asked every frame, in every state, so it always knows whether the game was
    // just sent to the background. Only "Playing" reacts to it (see below).
    const bool wentToBackground = WentToBackground();
    // Show touch hints after a tap, keyboard hints after a key: whatever the player uses.
    if (input.pointerPressed) {
        touchMode_ = true;
    } else if (input.move != 0.0f || input.fire) {
        touchMode_ = false;
    }
    if (input.toggleMute) {
        ToggleSound();
    }

    // The sound button works in every state (menu, playing, paused, game over).
    // A tap on it is "used up": it must not also start the game or steer the ship.
    bool tapUsed = false;
    if (input.pointerPressed) {
        const bool onMute  = CheckCollisionPointRec(input.pointer, MuteButtonRect());
        const bool onPause = state_ == GameState::Playing && CheckCollisionPointRec(input.pointer, PauseButtonRect());
        touchOnButton_ = onMute || onPause;
        if (onMute) {
            ToggleSound();
            tapUsed = true;
        }
    } else if (!input.pointerDown) {
        touchOnButton_ = false;   // the finger is lifted: the next touch is a new one
    }
    const bool tap = input.pointerPressed && !tapUsed;
    if (input.toggleFullscreen) {
        display::ToggleFullscreen();
    }
    if (input.toggleFps) {
        showFps_ = !showFps_;
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
    // While paused, explosions freeze too; only the stars keep drifting.
    stars_.Update(dt);
    nebula_.Update(dt);
    if (state_ != GameState::Paused) {
        particles_.Update(dt);
        glow_.Update(dt);
        time_ += dt;
        shake_ = std::max(0.0f, shake_ - config::kShakeFade * dt);
    }

    // Screen shake: move the camera (and the point it looks at) by a small offset.
    // Two sine waves with odd speeds give a jittery path that looks random, but
    // needs no random numbers. Squaring the trauma keeps small shakes small.
    const float amount = shake_ * shake_ * config::kShakeMaxOffset;
    const Vector3 offset{amount * std::sin(time_ * 47.0f), amount * std::sin(time_ * 61.0f + 1.3f), 0.0f};
    camera_.position = Vector3Add(config::kCameraPosition, offset);
    camera_.target   = Vector3Add(config::kCameraTarget, offset);

    // Music: calm on the menu and after a game, the level track while playing,
    // and the boss track during a boss fight. Audio fades between them by itself.
    if (state_ == GameState::Menu || state_ == GameState::GameOver) {
        audio_.SetTrack(Track::Menu);
    } else {
        audio_.SetTrack(boss_ ? Track::Boss : Track::Level);
    }
    audio_.Update(dt, state_ == GameState::Paused);

    // What else happens each frame depends on the state we are in.
    switch (state_) {
        case GameState::Menu:
            if (input.confirm || tap) {   // Enter, or a tap anywhere (but not on the sound button)
                Restart();
            }
            break;
        case GameState::Playing:
            // Pause with P, with the pause button, or by itself when the game goes
            // to the background (a phone call, another app or window).
            if (input.togglePause || wentToBackground ||
                (tap && CheckCollisionPointRec(input.pointer, PauseButtonRect()))) {
                state_ = GameState::Paused;
                break;   // nothing moves in the frame we pause
            }
            UpdatePlaying(dt, input);
            break;
        case GameState::Paused:
            // Continue only with P or the play button, so a stray tap (or picking the
            // phone back up) does not start the game by accident.
            if (input.togglePause ||
                (tap && CheckCollisionPointRec(input.pointer, PlayButtonRect()))) {
                dragActive_ = false;   // the next touch starts a fresh drag
                state_ = GameState::Playing;
            }
            break;
        case GameState::GameOver:
            // The world is frozen. After a short pause, wait for Enter.
            gameOverTime_ += dt;
            if (gameOverTime_ >= config::kGameOverDelay && (input.confirm || tap)) {
                Restart();
            }
            break;
    }
}

PlayerInput Game::ApplyTouchSteering(const PlayerInput& input) {
    PlayerInput steer = input;   // a copy we can change
    if (!input.pointerDown || touchOnButton_) {
        dragActive_ = false;
        return steer;            // no finger (or it is on a button): keyboard as usual
    }

    // How many screen pixels is one world unit at the ship's distance? We ask the
    // camera where two points 1 unit apart end up on screen.
    const Vector2 a = GetWorldToScreen(Vector3{0.0f, config::kPlayerHeight, 0.0f}, camera_);
    const Vector2 b = GetWorldToScreen(Vector3{1.0f, config::kPlayerHeight, 0.0f}, camera_);
    const float pixelsPerUnit = std::max(1.0f, b.x - a.x);

    // A new drag remembers where the finger and the ship started. From then on
    // the ship moves as far as the finger has moved: a "relative" drag, so you can
    // touch anywhere and your finger never hides the ship.
    if (!dragActive_) {
        dragActive_        = true;
        dragStartPointerX_ = input.pointer.x;
        dragStartShipX_    = player_.X();
    }
    float target = dragStartShipX_ +
                   (input.pointer.x - dragStartPointerX_) / pixelsPerUnit * config::kTouchDragGain;

    // At a wall: slide the start point along, so moving the finger back moves the
    // ship back immediately (instead of first "unwinding" the extra distance).
    const float limit = config::kFieldHalfWidth - player_.HalfWidth();
    if (target > limit)  { dragStartShipX_ -= target - limit;  target = limit; }
    if (target < -limit) { dragStartShipX_ += -limit - target; target = -limit; }

    steer.targetX = target;
    steer.fire    = true;   // while the finger is down, the ship fires by itself
    return steer;
}

void Game::ToggleSound() {
    audio_.ToggleMute();
    storage::SaveInt("muted", audio_.Muted() ? 1 : 0);   // remembered for next time
}

Rectangle Game::MuteButtonRect() const {
    const Rectangle pause = PauseButtonRect();   // same size, just left of it
    return Rectangle{pause.x - pause.width - static_cast<float>(display::Px(12)), pause.y,
                     pause.width, pause.height};
}

void Game::DrawMuteButton() const {
    const Rectangle r = MuteButtonRect();
    DrawRectangleRounded(r, 0.3f, 6, Fade(Color{20, 25, 50, 255}, 0.7f));
    DrawRectangleRoundedLinesEx(r, 0.3f, 6, 2.0f, Fade(RAYWHITE, 0.6f));

    // A loudspeaker: a small box and a cone opening to the right.
    const float s = r.width;
    const float cx = r.x + s * 0.40f;
    const float cy = r.y + s * 0.50f;
    DrawRectangleRec(Rectangle{cx - s * 0.20f, cy - s * 0.09f, s * 0.12f, s * 0.18f}, RAYWHITE);
    // The cone is two triangles (raylib wants each one's corners counter-clockwise on screen).
    DrawTriangle(Vector2{cx - s * 0.08f, cy - s * 0.09f}, Vector2{cx - s * 0.08f, cy + s * 0.09f},
                 Vector2{cx + s * 0.08f, cy + s * 0.22f}, RAYWHITE);
    DrawTriangle(Vector2{cx - s * 0.08f, cy - s * 0.09f}, Vector2{cx + s * 0.08f, cy + s * 0.22f},
                 Vector2{cx + s * 0.08f, cy - s * 0.22f}, RAYWHITE);

    if (audio_.Muted()) {
        // Sound off: a red cross next to the speaker.
        const Color red{255, 90, 90, 255};
        const float x = cx + s * 0.22f;
        DrawLineEx(Vector2{x, cy - s * 0.10f}, Vector2{x + s * 0.18f, cy + s * 0.10f}, s * 0.06f, red);
        DrawLineEx(Vector2{x, cy + s * 0.10f}, Vector2{x + s * 0.18f, cy - s * 0.10f}, s * 0.06f, red);
    } else {
        // Sound on: two sound waves (parts of rings) coming out of the speaker.
        const Vector2 centre{cx + s * 0.06f, cy};
        DrawRing(centre, s * 0.20f, s * 0.24f, -45.0f, 45.0f, 10, RAYWHITE);
        DrawRing(centre, s * 0.32f, s * 0.36f, -45.0f, 45.0f, 12, RAYWHITE);
    }
}

Rectangle Game::PlayButtonRect() const {
    using display::Px;
    const float size = static_cast<float>(Px(120));
    return Rectangle{(static_cast<float>(GetScreenWidth()) - size) / 2.0f,
                     static_cast<float>(GetScreenHeight()) / 2.0f - size / 2.0f + static_cast<float>(Px(20)),
                     size, size};
}

Rectangle Game::PauseButtonRect() const {
    using display::Px;
    const float size = static_cast<float>(Px(52));
    return Rectangle{static_cast<float>(GetScreenWidth() - Px(20)) - size,
                     static_cast<float>(Px(62)), size, size};
}

void Game::DrawPauseButton() const {
    const Rectangle r = PauseButtonRect();
    DrawRectangleRounded(r, 0.3f, 6, Fade(Color{20, 25, 50, 255}, 0.7f));
    DrawRectangleRoundedLinesEx(r, 0.3f, 6, 2.0f, Fade(RAYWHITE, 0.6f));
    // The pause symbol: two upright bars.
    const float barW = r.width * 0.16f;
    const float barH = r.height * 0.46f;
    const float top  = r.y + (r.height - barH) / 2.0f;
    DrawRectangleRec(Rectangle{r.x + r.width * 0.30f, top, barW, barH}, RAYWHITE);
    DrawRectangleRec(Rectangle{r.x + r.width * 0.54f, top, barW, barH}, RAYWHITE);
}

void Game::UpdatePlaying(float dt, const PlayerInput& input) {
    // ---- 1. Player and firing ---------------------------------------------------
    // After losing a ship (with lives left), wait a moment, then bring the next one.
    invincibleTimer_ = std::max(0.0f, invincibleTimer_ - dt);
    if (!playerAlive_) {
        playerRespawnTimer_ -= dt;
        if (playerRespawnTimer_ <= 0.0f) {
            RespawnPlayer();
        }
    }

    if (playerAlive_) {
        const PlayerInput steer = ApplyTouchSteering(input);
        player_.Update(dt, steer);

        // A cooldown limits the fire rate: it counts down, and we may only shoot
        // when it has reached zero. Holding the button fires every 0.2 seconds.
        // With rapid fire the cooldown is shorter, so we shoot more often.
        fireCooldown_ -= dt;
        if (steer.fire && fireCooldown_ <= 0.0f) {
            FireRocket();
            fireCooldown_ = (rapidFireTimer_ > 0.0f) ? config::kRapidFireInterval
                                                     : config::kRocketFireInterval;
        }
    }

    // Power-up timers run down. (std::max keeps them from going below zero.)
    shieldTimer_    = std::max(0.0f, shieldTimer_ - dt);
    rapidFireTimer_ = std::max(0.0f, rapidFireTimer_ - dt);
    pickupTimer_    = std::max(0.0f, pickupTimer_ - dt);
    comboTimer_     = std::max(0.0f, comboTimer_ - dt);
    bonusTimer_     = std::max(0.0f, bonusTimer_ - dt);
    if (waveActive_) waveTime_ += dt;

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
            invaderShotTimer_ = config::kLevelShotInterval[level_];
        }
    }

    // ---- 2b. Bombs appear every few seconds -----------------------------------------
    // (Not during a boss fight: the boss drops its own bombs.)
    if (!boss_) bombTimer_ -= dt;
    if (bombTimer_ <= 0.0f) {
        // Aimed roughly at where the ship is NOW, so the player has to react.
        // std::clamp keeps the bomb inside the walls.
        const float x = std::clamp(player_.X() + RandomFloat(-config::kBombAimSpread, config::kBombAimSpread),
                                   -config::kFieldHalfWidth + 1.0f, config::kFieldHalfWidth - 1.0f);
        bombs_.emplace_back(Vector3{x, config::kBombSpawnY, config::kBombSpawnZ},
                            config::kLevelBombSpeed[level_],
                            config::kLevelBombRadius[level_],
                            config::kLevelBombColor[level_],
                            level_);
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
    UpdateBoss(dt);

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

    // 4a1. Our rockets against the boss. A hit on the open core does triple damage
    // and sprays bigger sparks.
    for (auto& rocket : rockets_) {
        if (!rocket.alive || !boss_) continue;
        if (const auto hit = boss_->TryHit(rocket.position, config::kRocketRadius)) {
            rocket.alive = false;
            particles_.Explode(hit->position, hit->core ? BombExplosion(boss_->GlowColor()) : HitSparks());
            if (hit->destroyed) {
                audio_.Play(Sfx::BombExplosion);
            }
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
                AddShake(config::kShakeBomb);
                audio_.Play(Sfx::BombExplosion);
                break;   // this rocket is gone: stop looking at more bombs
            }
        }
    }

    // 4a'. The banners fade out by themselves.
    levelBannerTimer_ = std::max(0.0f, levelBannerTimer_ - dt);
    helpTimer_        = std::max(0.0f, helpTimer_ - dt);
    waveBannerTimer_  = std::max(0.0f, waveBannerTimer_ - dt);

    // 4b. Invader shots against our ship.
    for (auto& shot : invaderShots_) {
        if (playerAlive_ && shot.alive &&
            CheckCollisionSpheres(shot.position, config::kInvaderShotRadius,
                                  player_.Position(), player_.HitRadius())) {
            shot.alive = false;
            DamagePlayer("Your ship was hit!");
        }
    }

    // 4b2. Bombs against our ship.
    for (auto& bomb : bombs_) {
        if (bomb.alive && playerAlive_ &&
            CheckCollisionSpheres(bomb.position, bomb.radius,
                                  player_.Position(), player_.HitRadius())) {
            bomb.alive = false;
            particles_.Explode(bomb.position, BombExplosion(bomb.color));
            audio_.Play(Sfx::BombExplosion);
            DamagePlayer("A bomb destroyed your ship!");
        }
    }

    // 4b3. A diving alien crashing into our ship. The alien is always destroyed;
    // with a shield up we even get its points.
    if (playerAlive_) {
        if (const auto ram = invaders_.TryRam(player_.Position(), player_.HitRadius())) {
            if (!DamagePlayer("An alien crashed into your ship!")) {
                OnAlienDestroyed(*ram);
            } else {
                particles_.Explode(ram->position, AlienExplosion(ram->type, level_));
            }
        }
    }

    // 4b4. Flying into a power-up capsule collects it.
    for (auto& powerUp : powerUps_) {
        if (playerAlive_ && powerUp.alive &&
            CheckCollisionSpheres(powerUp.position, config::kPowerUpRadius,
                                  player_.Position(), player_.HitRadius() + 0.3f)) {
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
    // (While a boss is there, the fight is not over, even without aliens.)
    if (invaders_.Empty() && waveActive_) {
        AwardWaveBonus();   // once per wave, in the frame its last alien dies
    }
    if (invaders_.Empty() && !boss_) {
        respawnTimer_ -= dt;
        if (respawnTimer_ <= 0.0f) {
            NextWave();
        }
    } else {
        respawnTimer_ = config::kWaveRespawnDelay;
    }
}

void Game::Draw() const {
    nebula_.Draw();   // first: the background clouds fill the whole screen

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

        // A fresh ship blinks while it cannot be hit: drawn 0.1 s, hidden 0.1 s.
        const bool blinkHidden = invincibleTimer_ > 0.0f && std::fmod(invincibleTimer_, 0.2f) < 0.1f;
        if (playerAlive_ && !blinkHidden) {
            player_.Draw();
        }
        invaders_.Draw(alienModels_);
        if (boss_) boss_->Draw();
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
            DrawSphereWires(player_.Position(), ShieldRadius(), 6, 12, Fade(Color{80, 220, 255, 255}, 0.55f));
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
            // Near the ship the glow shrinks, so even a huge bomb never hides it.
            const float distance = Vector3Distance(bomb.position, player_.Position());
            const float closeness = std::clamp(distance / config::kBombGlowNearDistance, 0.0f, 1.0f);
            const float factor = config::kBombGlowNearFactor + (1.0f - config::kBombGlowNearFactor) * closeness;
            glow_.Draw(camera_, bomb.position, bomb.GlowSize(time_) * factor, bomb.color);
        }
        for (const auto& powerUp : powerUps_) {
            glow_.Draw(camera_, powerUp.position, 2.6f, powerUp.MainColor());
        }
        if (shieldVisible) {
            glow_.Draw(camera_, player_.Position(), ShieldRadius() * 2.6f, Color{60, 160, 255, 255});
        }
        if (boss_) boss_->DrawGlow(glow_, camera_);
        glow_.End();

        particles_.Draw();   // last: additive blending must be drawn on top of the solid objects

    EndMode3D();

    if (state_ == GameState::Menu) {
        DrawMenu();
        DrawMuteButton();
        return;   // no score or banner on the title screen
    }

    // 2D text on top of the 3D picture (the HUD = "heads-up display").
    // Px() scales every size and position with the window height, so the text
    // stays the same size relative to the picture in a small and in a huge window.
    using display::Px;
    DrawText(TextFormat("SCORE  %06d", score_), Px(20), Px(16), Px(32), RAYWHITE);

    // The small lines under the score are stacked: each one that is shown moves
    // the next one down, so there are never gaps.
    int lineY = Px(56);
    if (helpTimer_ > 0.0f) {
        // The key help, only during the first seconds of a game (it fades out in
        // the last second). The pause screen shows it again.
        const float alpha = std::min(1.0f, helpTimer_);
        DrawText(HelpLine(), Px(20), lineY, Px(20), Fade(LIGHTGRAY, alpha));
        lineY += Px(28);
    }
    if (showFps_) {
        DrawText(TextFormat("%d FPS", GetFPS()), Px(20), lineY, Px(20), GREEN);
        lineY += Px(28);
    }
    DrawLives();

    // The best score, in the middle at the top. While you beat it, it counts up with you.
    const char* best = TextFormat("HI %06d", std::max(highScore_, score_));
    DrawText(best, (GetScreenWidth() - MeasureText(best, Px(24))) / 2, Px(20), Px(24),
             Color{255, 200, 60, 255});
    if (audio_.Muted()) {
        DrawText("SOUND OFF", Px(20), lineY, Px(20), ORANGE);
    }

    // Level in the top right corner, right-aligned: measure the text, then
    // start that far left of the screen edge.
    // "WAVE 2/3   LEVEL 3" in the top right corner, right-aligned: measure the
    // text, then start that far left of the screen edge.
    const char* levelText = TextFormat("LEVEL %d", level_ + 1);
    const int levelX = GetScreenWidth() - Px(20) - MeasureText(levelText, Px(32));
    DrawText(levelText, levelX, Px(16), Px(32), SKYBLUE);
    const char* waveText = boss_ ? "BOSS"
        : (wave_ < config::kWavesPerLevel)
        ? TextFormat("WAVE %d/%d", wave_ + 1, config::kWavesPerLevel)
        : TextFormat("BONUS WAVE %d", wave_ - config::kWavesPerLevel + 1);
    DrawText(waveText, levelX - Px(18) - MeasureText(waveText, Px(22)), Px(24), Px(22),
             Fade(SKYBLUE, 0.75f));

    // Active power-ups under the level, right-aligned, with the seconds left.
    auto drawRight = [&](const char* text, int y, Color color) {
        DrawText(text, GetScreenWidth() - Px(20) - MeasureText(text, Px(22)), y, Px(22), color);
    };
    if (state_ == GameState::Playing) {
        DrawPauseButton();
    }
    int statusY = Px(126);   // below the pause button
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
    if (!boss_) {
        DrawCombo();   // (during a boss fight the health bar is in that place)
    }
    if (bonusTimer_ > 0.0f) {
        const float alpha = std::min(1.0f, bonusTimer_ / 0.5f);
        const int size = Px(30);
        DrawText(bonusText_.c_str(), (GetScreenWidth() - MeasureText(bonusText_.c_str(), size)) / 2,
                 GetScreenHeight() * 36 / 100, size, Fade(Color{255, 200, 60, 255}, alpha));
    }

    if (state_ == GameState::Playing && pickupTimer_ > 0.0f) {
        const float alpha = std::min(1.0f, pickupTimer_ / 0.5f);   // fades out at the end
        const int size = Px(46);
        DrawText(pickupText_, (GetScreenWidth() - MeasureText(pickupText_, size)) / 2,
                 GetScreenHeight() * 2 / 5, size, Fade(pickupColor_, alpha));   // above the ship
    }

    if (boss_) {
        DrawBossBar();
    }

    if (state_ == GameState::Playing && levelBannerTimer_ > 0.0f) {
        DrawLevelBanner();
    } else if (state_ == GameState::Playing && waveBannerTimer_ > 0.0f) {
        // The small wave banner, when no big level banner is showing.
        const float alpha = waveBannerTimer_ > 0.4f ? 1.0f : waveBannerTimer_ / 0.4f;
        const char* text = (wave_ < config::kWavesPerLevel)
            ? TextFormat("WAVE %d / %d", wave_ + 1, config::kWavesPerLevel)
            : TextFormat("BONUS WAVE %d", wave_ - config::kWavesPerLevel + 1);
        DrawText(text, (GetScreenWidth() - MeasureText(text, Px(44))) / 2, GetScreenHeight() / 4,
                 Px(44), Fade(SKYBLUE, alpha));
    }

    if (state_ == GameState::Paused) {
        DrawPaused();
    }

    if (state_ == GameState::GameOver && gameOverTime_ >= config::kGameOverDelay) {
        DrawGameOver();
    }

    DrawMuteButton();   // last, so the dark pause and game-over screens do not cover it
}

// One small ship icon per life, right of the score: a triangle with an orange flame.
void Game::DrawLives() const {
    using display::Px;
    const int scoreWidth = MeasureText(TextFormat("SCORE  %06d", score_), Px(32));
    int x = Px(20) + scoreWidth + Px(28);
    const float y = static_cast<float>(Px(32));
    const float s = static_cast<float>(Px(10));   // icon size
    for (int i = 0; i < lives_; ++i) {
        const float cx = static_cast<float>(x);
        // raylib wants the three corners counter-clockwise (as seen on screen).
        DrawTriangle(Vector2{cx, y - s}, Vector2{cx - s, y + s * 0.8f}, Vector2{cx + s, y + s * 0.8f},
                     Color{190, 205, 230, 255});                         // the ship
        DrawTriangle(Vector2{cx, y + s * 1.5f}, Vector2{cx + s * 0.35f, y + s * 0.8f},
                     Vector2{cx - s * 0.35f, y + s * 0.8f}, Color{255, 140, 40, 255});   // flame
        x += Px(26);
    }
}

void Game::DrawPaused() const {
    using display::Px;
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.55f));

    const Rectangle play = PlayButtonRect();
    const char* title = "PAUSED";
    DrawText(title, (GetScreenWidth() - MeasureText(title, Px(70))) / 2,
             static_cast<int>(play.y) - Px(100), Px(70), RAYWHITE);

    // The play button: a glowing circle with a triangle pointing right ("play").
    const Vector2 c{play.x + play.width / 2.0f, play.y + play.height / 2.0f};
    const float r = play.width / 2.0f;
    const float pulse = 0.5f + 0.5f * sinf(static_cast<float>(GetTime()) * 3.0f);
    DrawCircleV(c, r, Fade(Color{40, 140, 90, 255}, 0.85f));
    DrawCircleLinesV(c, r, Fade(Color{150, 255, 170, 255}, 0.6f + 0.4f * pulse));
    // raylib wants the corners counter-clockwise as seen on screen: top-left,
    // bottom-left, then the tip on the right.
    DrawTriangle(Vector2{c.x - r * 0.30f, c.y - r * 0.42f},
                 Vector2{c.x - r * 0.30f, c.y + r * 0.42f},
                 Vector2{c.x + r * 0.45f, c.y}, RAYWHITE);

    const char* hint = IsTouchDevice() ? "Tap the play button to continue"
                                       : "Click the play button or press P to continue";
    DrawText(hint, (GetScreenWidth() - MeasureText(hint, Px(24))) / 2,
             static_cast<int>(play.y + play.height) + Px(28), Px(24), LIGHTGRAY);
    // The controls, for anyone who missed them at the start.
    DrawText(HelpLine(), (GetScreenWidth() - MeasureText(HelpLine(), Px(20))) / 2,
             static_cast<int>(play.y + play.height) + Px(68), Px(20), GRAY);
}

const char* Game::HelpLine() const {
    // Only the keys needed while playing; the window keys are listed in the menu.
    return touchMode_ ? "Drag anywhere to move. The ship fires by itself."
                      : "Move: A / D or arrows     Fire: Space (hold)     Pause: P     Mute: M     FPS: F3";
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
    if (newHighScore_) {
        // Blinks twice per second: on while the fraction of the time is below one half.
        const bool on = std::fmod(static_cast<float>(GetTime()) * 2.0f, 1.0f) < 0.5f;
        if (on) drawCentered("NEW HIGH SCORE!", middle + Px(60), Px(26), Color{255, 200, 60, 255});
    }
    drawCentered(touchMode_ ? (IsTouchDevice() ? "Tap to play again" : "Click to play again")
                            : "Press ENTER to play again",
                 middle + Px(100), Px(26), LIGHTGRAY);
#if !defined(__EMSCRIPTEN__)
    drawCentered("Esc to quit", middle + Px(140), Px(22), GRAY);
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
    if (highScore_ > 0) {
        drawCentered(TextFormat("HIGH SCORE  %06d", highScore_), Px(275), Px(26), Color{255, 200, 60, 255});
    }
    if (touchMode_) {
        drawCentered(IsTouchDevice() ? "Tap to start" : "Click to start", Px(330), Px(32), Fade(RAYWHITE, pulse));
        drawCentered("Drag anywhere to move. The ship fires by itself.", Px(380), Px(22), LIGHTGRAY);
    } else {
        drawCentered("Press ENTER to start", Px(330), Px(32), Fade(RAYWHITE, pulse));
        drawCentered("Move: A / D or arrows     Fire: Space (hold)     Pause: P", Px(380), Px(22), LIGHTGRAY);
    }
#if !defined(__EMSCRIPTEN__)
    drawCentered("Fullscreen: F     Window size: 1 - 5     Quit: Esc", Px(415), Px(22), LIGHTGRAY);
#endif
}

void Game::DrawLevelBanner() const {
    using display::Px;
    // Fade out during the last half second of the banner time.
    const float alpha = levelBannerTimer_ > 0.5f ? 1.0f : levelBannerTimer_ / 0.5f;
    const int titleSize = Px(70);
    const int subSize   = Px(28);
    const int y = GetScreenHeight() / 4;
    // .c_str() turns a std::string into the plain "const char*" that raylib wants.
    DrawText(bannerTitle_.c_str(), (GetScreenWidth() - MeasureText(bannerTitle_.c_str(), titleSize)) / 2,
             y, titleSize, Fade(SKYBLUE, alpha));
    DrawText(bannerSubtitle_.c_str(), (GetScreenWidth() - MeasureText(bannerSubtitle_.c_str(), subSize)) / 2,
             y + titleSize + Px(12), subSize, Fade(RAYWHITE, alpha));
}

void Game::DrawBossBar() const {
    using display::Px;
    // The boss's name and a health bar, centred under the high score. The bar
    // has a dark background, the remaining health in the boss's colour (red
    // when it is angry), and a thin white frame. It shakes a little when angry.
    const int width  = GetScreenWidth() * 2 / 5;
    const int height = Px(14);
    const int shake  = boss_->Angry() ? static_cast<int>(2.0f * std::sin(time_ * 40.0f)) : 0;
    const int x = (GetScreenWidth() - width) / 2 + shake;
    const int y = Px(80);

    const char* name = boss_->Name();
    DrawText(name, (GetScreenWidth() - MeasureText(name, Px(22))) / 2, Px(52), Px(22), RAYWHITE);

    const Color fill = boss_->Angry() ? Color{255, 60, 60, 255} : boss_->GlowColor();
    DrawRectangle(x, y, width, height, Fade(BLACK, 0.6f));
    DrawRectangle(x, y, static_cast<int>(static_cast<float>(width) * boss_->Health()), height, fill);
    DrawRectangleLines(x, y, width, height, Fade(RAYWHITE, 0.8f));
    // Quarter marks, so the player can see the progress.
    for (int i = 1; i < 4; ++i) {
        DrawLine(x + width * i / 4, y, x + width * i / 4, y + height, Fade(RAYWHITE, 0.5f));
    }
    if (boss_->CoreOpen()) {
        const char* hint = "CORE OPEN!";
        DrawText(hint, (GetScreenWidth() - MeasureText(hint, Px(20))) / 2, y + height + Px(8), Px(20),
                 Fade(fill, 0.6f + 0.4f * std::sin(time_ * 10.0f)));
    }
}

void Game::DrawCombo() const {
    using display::Px;
    if (comboTimer_ <= 0.0f || ComboMultiplier() < 2) return;   // only worth showing from x2
    // "COMBO x3" under the high score, and a small bar that runs out with the time
    // left to keep the combo going.
    const char* text = TextFormat("COMBO x%d", ComboMultiplier());
    const int size = Px(26);
    const Color color = ComboMultiplier() >= config::kComboMaxMultiplier ? Color{255, 90, 200, 255}
                                                                         : Color{120, 230, 255, 255};
    // While the key help is still showing (first seconds of a game), its long line
    // runs under the middle of the screen: then the combo moves down a line.
    const int y = (helpTimer_ > 0.0f) ? Px(84) : Px(50);
    DrawText(text, (GetScreenWidth() - MeasureText(text, size)) / 2, y, size, color);
    const int width = Px(120);
    const int x = (GetScreenWidth() - width) / 2;
    DrawRectangle(x, y + Px(30), static_cast<int>(static_cast<float>(width) * comboTimer_ / config::kComboWindow),
                  Px(4), color);
}
