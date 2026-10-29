// Game.cpp - the rules of the game.

#include "Game.h"

#include <algorithm>   // std::clamp, std::min, std::max
#include <cmath>       // sinf, std::fmod, std::ceil

#include "Config.h"
#include "Display.h"
#include "Storage.h"
#include "Input.h"
#include "Platform.h"
#include "LowPoly.h"
#include "Random.h"
#include "Util.h"
#include "raymath.h"   // Vector3Add, Vector3Scale, Vector3Distance
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
        case AlienType::Wasp:
            style.colorA = palette.glow;                  // its stripes
            style.colorB = Color{255, 245, 200, 255};     // glassy wing bits
            break;
        case AlienType::Mite:
            style.colorA = palette.glow;                  // its eyes
            style.colorB = Color{190, 200, 215, 255};     // bits of steel
            break;
        case AlienType::Moth:
            style.count  = style.count * 3 / 2;           // a cloud of wing dust
            style.colorA = bodyLight;
            style.colorB = palette.glow;
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
        case AlienType::Wasp:      return config::kScorePerWasp;
        case AlienType::Moth:      return config::kScorePerMoth;
        case AlienType::Mite:      return config::kScorePerMite;
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


// ---- Text helpers for the 2D screens ------------------------------------------------
// The tall screen is narrow, so long sentences must be broken into several lines.

// Splits a text into lines that are at most maxWidth pixels wide, breaking between
// words. A '\n' in the text always starts a new line. Several spaces in a row stay
// as they are (they separate groups, like "Fire: Space     Pause: P").
std::vector<std::string> WrapText(const std::string& text, int fontSize, int maxWidth) {
    std::vector<std::string> lines;
    std::string line;
    std::string word;
    std::string gap;   // the spaces before the current word
    // Adds the word to the line, or starts a new line when it would get too wide.
    auto addWord = [&]() {
        if (word.empty()) return;
        const std::string candidate = line.empty() ? word : line + gap + word;
        if (!line.empty() && MeasureText(candidate.c_str(), fontSize) > maxWidth) {
            lines.push_back(line);
            line = word;
        } else {
            line = candidate;
        }
        word.clear();
        gap.clear();
    };
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) addWord();
            gap += c;
        } else if (c == '\n') {
            addWord();
            lines.push_back(line);
            line.clear();
            gap.clear();
        } else {
            word += c;
        }
    }
    addWord();
    if (!line.empty()) lines.push_back(line);
    return lines;
}

// Draws a text centred on the vertical line x = centerX, wrapped to maxWidth.
// Returns the y just below the last line, so the next text can follow it.
int DrawCentered(const std::string& text, float centerX, int y, int fontSize, int maxWidth, Color color) {
    for (const std::string& line : WrapText(text, fontSize, maxWidth)) {
        const int width = MeasureText(line.c_str(), fontSize);
        DrawText(line.c_str(), static_cast<int>(centerX) - width / 2, y, fontSize, color);
        y += fontSize + fontSize / 4;   // line height: the font size plus a little gap
    }
    return y;
}

// Draws a text so that it ENDS at x = rightX (right-aligned).
void DrawRight(const char* text, float rightX, int y, int fontSize, Color color) {
    DrawText(text, static_cast<int>(rightX) - MeasureText(text, fontSize), y, fontSize, color);
}

}  // namespace

Game::Game() {
    // The camera (explained in M1). Where it stands is worked out by FitCamera.
    camera_.up         = Vector3{0.0f, 1.0f, 0.0f};
    camera_.fovy       = config::kCameraFovDegrees;
    camera_.projection = CAMERA_PERSPECTIVE;
    FitCamera();

    highScore_ = storage::LoadInt("highscore");   // the best score from earlier games (0 the first time)
    audio_.SetMuted(storage::LoadInt("muted") != 0);   // muted last time? Then stay muted.
    touchMode_ = IsTouchDevice();     // phones and tablets start with the touch hints

    Restart();                 // builds a fresh game ...
    state_ = GameState::Menu;  // ... but we wait on the title screen until Enter is pressed
}

void Game::FitCamera() {
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();
    if (screenW == fittedWidth_ && screenH == fittedHeight_) return;   // nothing changed
    fittedWidth_  = screenW;
    fittedHeight_ = screenH;

    // The camera looks down at kCameraPitchDegrees and always points at a spot
    // on the floor (target). It stands "distance" units back from that spot, in
    // the direction it looks from. Two numbers are open: how far back (distance)
    // and which spot (targetZ). We find both by trying, with a "binary search":
    // guess the middle of a range, see if it is too much or too little, and halve
    // the range. 40 halvings are far more exact than a pixel.
    const float pitch = config::kCameraPitchDegrees * DEG2RAD;
    const Vector3 back{0.0f, std::sin(pitch), std::cos(pitch)};   // from the target to the camera

    const Rectangle column = display::Column();
    const float top    = column.y + static_cast<float>(display::Px(config::kViewTopPx));
    const float bottom = column.y + column.height - static_cast<float>(display::Px(config::kViewBottomPx));
    const float halfW  = config::kFieldHalfWidth + config::kViewSideMargin;
    const Vector3 nearMid{0.0f, 0.0f, config::kViewNearZ};
    const Vector3 farMid {0.0f, 0.0f, config::kViewFarZ};
    const Vector3 nearRight{halfW, 0.0f, config::kViewNearZ};   // the widest point on screen

    Camera3D test = camera_;
    auto place = [&](float distance, float targetZ) {
        test.target   = Vector3{0.0f, 0.0f, targetZ};
        test.position = Vector3Add(test.target, Vector3Scale(back, distance));
    };
    auto screenPos = [&](Vector3 point) { return GetWorldToScreenEx(point, test, screenW, screenH); };

    // For one distance: the targetZ that puts the field in the middle (vertically)
    // of the free space between the HUD and the bottom edge.
    auto centerTarget = [&](float distance) {
        float lo = -80.0f, hi = 40.0f;   // targetZ range to search in
        for (int i = 0; i < 40; ++i) {
            const float mid = 0.5f * (lo + hi);
            place(distance, mid);
            const float middleY = 0.5f * (screenPos(nearMid).y + screenPos(farMid).y);
            // A target further away (smaller z) moves the picture down the screen.
            if (middleY > 0.5f * (top + bottom)) lo = mid; else hi = mid;
        }
        return 0.5f * (lo + hi);
    };
    // Does the field fit at this distance?
    auto fits = [&](float distance) {
        place(distance, centerTarget(distance));
        const Vector2 n = screenPos(nearMid);
        const Vector2 f = screenPos(farMid);
        const Vector2 r = screenPos(nearRight);
        const float halfColumn = 0.5f * column.width;
        return f.y >= top && n.y <= bottom && (r.x - n.x) <= halfColumn;
    };

    float lo = 5.0f, hi = 400.0f;   // closest and furthest distance we consider
    for (int i = 0; i < 40; ++i) {
        const float mid = 0.5f * (lo + hi);
        if (fits(mid)) hi = mid; else lo = mid;   // fits: try closer; else further away
    }
    place(hi, centerTarget(hi));

    // The ship's limits up and down the screen (see kPlayerUpLimitShare in Config.h):
    // which Z is on the middle line of the screen, and at which Z does the back of
    // the ship touch the bottom edge? A point further up the field (smaller z) is
    // higher on the screen (smaller y), so again we can halve a range.
    auto zAtScreenY = [&](float wantedY, float offsetZ) {
        float lo = -60.0f, hi = 30.0f;
        for (int i = 0; i < 40; ++i) {
            const float mid = 0.5f * (lo + hi);
            const float y = screenPos(Vector3{0.0f, config::kPlayerHeight, mid + offsetZ}).y;
            if (y > wantedY) hi = mid; else lo = mid;
        }
        return 0.5f * (lo + hi);
    };
    playerMinZ_ = zAtScreenY(column.y + column.height * config::kPlayerUpLimitShare, 0.0f);
    playerMaxZ_ = zAtScreenY(column.y + column.height, config::kPlayerTailLength);

    cameraBasePosition_ = test.position;
    cameraBaseTarget_   = test.target;
    camera_.position    = test.position;
    camera_.target      = test.target;
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
    weaponLevel_      = 1;
    weaponType_       = WeaponType::Blaster;
    laserFiring_      = false;
    laserTick_.fill(0.0f);
    pickupTimer_      = 0.0f;
    helpTimer_        = config::kHelpShowTime;   // show the keys for the first seconds
    boss_.reset();
    bossDefeated_     = false;
    shake_            = 0.0f;
    comboCount_       = 0;
    comboTimer_       = 0.0f;
    bonusTimer_       = 0.0f;
    demoComplete_     = false;
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
        // The demo ends here: after the last demo level (and its boss), show the
        // "get the full game" screen instead of the next level.
        if (config::kDemo && level_ >= config::kDemoLastLevel) {
            demoComplete_ = true;
            EndGame("Demo complete");
            return;
        }
        ++level_;
        wave_ = 0;
        // Tougher aliens from here on: the weapon is lifted to the level's minimum.
        weaponLevel_ = std::max(weaponLevel_, config::kLevelMinWeapon[level_]);
        bossDefeated_ = false;
        bannerTitle_      = TextFormat("LEVEL %d", level_ + 1);
        bannerSubtitle_   = config::kLevelIntro[level_];
        levelBannerTimer_ = config::kLevelBannerTime;
        stars_.SetColor(config::kLevelStarColor[level_]);   // the new colour mood
        nebula_.SetColors(config::kLevelNebulaA[level_], config::kLevelNebulaB[level_]);

        // Some levels upgrade the ship (a bigger tier, a new design, or the Horn in a
        // new colour): new look, and a burst of sparkles in the new shot colour around it.
        const int tier   = config::kLevelShipTier[level_];
        const int design = config::kLevelShipDesign[level_];
        const int color  = config::kLevelHornColor[level_];
        if (tier != player_.Tier() || design != player_.Design() || color != player_.HornColor()) {
            player_.SetTier(tier);
            player_.SetDesign(design, color);
            if (playerAlive_) {
                particles_.Explode(player_.Position(),
                                   ExplosionStyle{70, 9.0f, 1.0f, 0.3f,
                                                  ShotColor(), Color{255, 255, 255, 255}});
            }
        }
    }

    invaders_.Respawn(level_, wave_);
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
    // The last wave of a level without a boss also pays the level-clear bonus.
    // (In a level with a boss, the boss pays it, see UpdateBoss.)
    const bool levelDone = wave_ + 1 == config::kWavesPerLevel && config::kLevelBoss[level_] < 0;
    const int levelBonus = levelDone ? config::kLevelClearBonus : 0;
    if (speedBonus + noHitBonus + levelBonus == 0) return;
    score_ += speedBonus + noHitBonus + levelBonus;

    // Build the text from the parts that were earned. std::string can be added
    // together with +, and std::to_string turns a number into text.
    bonusText_.clear();
    if (speedBonus > 0) bonusText_ += "SPEED BONUS +" + std::to_string(speedBonus);
    if (speedBonus > 0 && noHitBonus > 0) bonusText_ += "\n";   // a new line (the screen is narrow)
    if (noHitBonus > 0) bonusText_ += "NO HIT +" + std::to_string(noHitBonus);
    if (levelBonus > 0) {
        if (!bonusText_.empty()) bonusText_ += "\n";
        bonusText_ += "LEVEL CLEAR +" + std::to_string(levelBonus);
    }
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
    const BossEvents events = boss_->Update(dt, player_.Position(), invaderShots_, bombs_);
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
        score_ += boss_->Points() + config::kLevelClearBonus;   // the boss ends the level
        bannerTitle_      = "BOSS DEFEATED!";
        bannerSubtitle_   = TextFormat("+%d points, level clear +%d", boss_->Points(), config::kLevelClearBonus);
        levelBannerTimer_ = config::kBossOutroTime - 0.3f;
        // Two power-ups fall out of the wreck; one is always a weapon orb.
        powerUps_.emplace_back(Vector3{center.x - 2.0f, center.y, center.z}, PowerUpType::Weapon);
        powerUps_.emplace_back(Vector3{center.x + 2.0f, center.y, center.z}, PowerUp::RandomType());
        boss_.reset();
        bossDefeated_ = true;
        respawnTimer_ = config::kBossOutroTime;
    }
}

void Game::RespawnPlayer() {
    player_ = Player();           // a fresh ship in the middle ...
    player_.SetTier(config::kLevelShipTier[level_]);   // ... with the upgrade of this level
    player_.SetDesign(config::kLevelShipDesign[level_], config::kLevelHornColor[level_]);
    playerAlive_     = true;
    invincibleTimer_ = config::kInvincibleTime;
    fireCooldown_    = 0.0f;
    // Fair play: remove the shots and bombs that are already on their way, so the
    // new ship does not appear in the middle of the fire that destroyed the last one.
    invaderShots_.clear();
    bombs_.clear();
}

Color Game::ShotColor() const {
    switch (player_.Design()) {
        case 2:  return config::kHornGlow[player_.HornColor()];   // the Horn: its glow colour
        case 1:  return config::kHeroShotColor;                   // the Claw: white-gold
        default: return config::kTierShotColor[player_.Tier()];
    }
}

void Game::FireRocket() {
    // The weapon level decides how many rockets fly at once, and where to (the
    // tables are in Config.h). The ship's look only changes their colour and spacing.
    const int level = weaponLevel_ - 1;   // the tables count from 0
    const float scale = config::kTierScale[player_.Tier()];
    if (weaponType_ == WeaponType::Spread) {
        // A fan: n rockets from the nose, their angles spread evenly from -max to +max.
        const int n = config::kSpreadShots[level];
        const float maxAngle = config::kSpreadAngle[level];
        const Vector3 nose{player_.X(), config::kPlayerHeight, player_.Z() + config::kWeaponMuzzleZ * scale};
        for (int i = 0; i < n; ++i) {
            const float angle = -maxAngle + 2.0f * maxAngle * static_cast<float>(i) / static_cast<float>(n - 1);
            rockets_.emplace_back(nose, config::kSpreadShotColor, angle);
        }
        audio_.Play(Sfx::PlayerShot);
        return;
    }
    // A reference to one row of a 2D array: "the shots of this weapon level".
    const config::WeaponShot (&shots)[config::kMaxWeaponShots] = config::kWeaponShots[level];
    for (int i = 0; i < config::kWeaponShotCount[level]; ++i) {
        const Vector3 start{player_.X() + shots[i].x * scale, config::kPlayerHeight,
                            player_.Z() + config::kWeaponMuzzleZ * scale};
        rockets_.emplace_back(start, ShotColor(), shots[i].angle);
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

    // Maybe drop a power-up orb. Commanders always drop one, Moths often; the others
    // with a small chance. RandomFloat(0, 1) < 0.10 is true 10% of the time.
    const float chance = (hit.type == AlienType::Moth) ? config::kMothOrbChance : config::kPowerUpDropChance;
    const bool lucky = commander || RandomFloat(0.0f, 1.0f) < chance;
    if (lucky && static_cast<int>(powerUps_.size()) < config::kMaxPowerUps) {
        powerUps_.emplace_back(hit.position, PowerUp::RandomType());
    }
}

void Game::Collect(PowerUpType type) {
    switch (type) {
        case PowerUpType::Weapon:
            if (RaiseWeaponLevel()) {
                pickupText_ = (weaponLevel_ == config::kWeaponLevels) ? "MAX POWER!" : "POWER UP!";
            } else {
                score_ += config::kMaxPowerBonus;   // already at the top: points instead
                pickupText_ = TextFormat("+%d", config::kMaxPowerBonus);
            }
            break;
        case PowerUpType::Spread:
            SwitchWeapon(WeaponType::Spread);
            break;
        case PowerUpType::Laser:
            SwitchWeapon(WeaponType::Laser);
            break;
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

bool Game::RaiseWeaponLevel() {
    if (weaponLevel_ >= config::kWeaponLevels) return false;
    ++weaponLevel_;
    // A burst of sparks in the weapon's colour: the ship is stronger now.
    particles_.Explode(player_.Position(), ExplosionStyle{50, 8.0f, 0.8f, 0.25f, WeaponColor(),
                                                         Color{255, 255, 255, 255}});
    return true;
}

void Game::SwitchWeapon(WeaponType type) {
    if (type == weaponType_) {
        // The weapon you already have: it gets stronger instead (or points at the top).
        if (RaiseWeaponLevel()) {
            pickupText_ = (weaponLevel_ == config::kWeaponLevels) ? "MAX POWER!" : "POWER UP!";
        } else {
            score_ += config::kMaxPowerBonus;
            pickupText_ = TextFormat("+%d", config::kMaxPowerBonus);
        }
        return;
    }
    // A new weapon, at the level you had.
    weaponType_ = type;
    pickupText_ = (type == WeaponType::Spread) ? "SPREAD SHOT!" : "LIGHTNING LASER!";
    particles_.Explode(player_.Position(), ExplosionStyle{60, 9.0f, 0.9f, 0.25f, WeaponColor(),
                                                         Color{255, 255, 255, 255}});
}

Color Game::WeaponColor() const {
    switch (weaponType_) {
        case WeaponType::Spread: return config::kSpreadShotColor;
        case WeaponType::Laser:  return config::kLaserColor;
        default:                 return Color{170, 80, 255, 255};   // the blaster: purple, like the weapon orb
    }
}

const char* Game::WeaponName() const {
    switch (weaponType_) {
        case WeaponType::Spread: return "SPREAD";
        case WeaponType::Laser:  return "LASER";
        default:                 return "BLASTER";
    }
}

void Game::UpdateLaser(float dt) {
    const int level = weaponLevel_ - 1;   // the tables count from 0
    const float scale = config::kTierScale[player_.Tier()];
    const Vector3 nose{player_.X(), config::kPlayerHeight, player_.Z() + config::kWeaponMuzzleZ * scale};
    const float halfWidth = config::kLaserHalfWidth[level];
    const int bolts = config::kLaserBolts[level];

    laser_.Update(dt, laserFiring_, weaponLevel_);
    if (!laserFiring_) {
        laserTick_.fill(0.0f);   // the next press hits at once
        for (int i = 0; i < config::kLaserMaxBolts; ++i) laser_.SetBolt(i, false, nose, nose, false);
        return;
    }

    // The buzz of the laser, repeated while it fires.
    laserSoundTimer_ -= dt;
    if (laserSoundTimer_ <= 0.0f) {
        audio_.Play(Sfx::PlayerShot);
        laserSoundTimer_ = config::kLaserSoundTime;
    }

    const ExplosionStyle sparks{6, 5.0f, 0.3f, 0.18f, config::kLaserColor, Color{255, 255, 255, 255}};
    for (int i = 0; i < config::kLaserMaxBolts; ++i) {
        if (i >= bolts) {
            laser_.SetBolt(i, false, nose, nose, false);
            continue;
        }
        // The bolts fan out a little: one straight up, two leaning apart, or three
        // (left, middle, right). Each one starts a bit to the side of the nose.
        // "side" is -1, 0 or +1 (for two bolts: -0.5 and +0.5).
        const float side = bolts == 1 ? 0.0f
                         : bolts == 2 ? (i == 0 ? -0.5f : 0.5f)
                                      : static_cast<float>(i - 1);
        const float slope = std::tan(side * config::kLaserBoltAngle * DEG2RAD);
        const Vector3 start{nose.x + side * config::kLaserBoltSpacing, nose.y, nose.z};
        // x of this bolt at height z
        auto lineX = [&](float z) { return start.x + slope * (start.z - z); };

        // Where does the bolt end? At the NEAREST thing in its way: an alien, a
        // bomb or the boss. If nothing is there, it reaches the far end of the field.
        enum class Target { None, Alien, Bomb, Boss };
        Target target = Target::None;
        float endZ = -config::kFieldLength;
        Vector3 alienPosition{};
        Bomb* bombInWay = nullptr;   // a pointer to the bomb in the vector (or none)
        if (const auto alien = invaders_.FirstInLine(start.x, halfWidth, start.z, slope)) {
            alienPosition = *alien;
            endZ = alien->z;
            target = Target::Alien;
        }
        for (auto& bomb : bombs_) {
            if (bomb.alive && bomb.position.z < start.z && bomb.position.z > endZ &&
                std::fabs(bomb.position.x - lineX(bomb.position.z)) <= bomb.radius + halfWidth) {
                endZ = bomb.position.z;
                target = Target::Bomb;
                bombInWay = &bomb;
            }
        }
        if (boss_) {
            // The boss is not a simple ball: walk up the bolt in small steps until
            // a step touches it (Touches only looks, it does no damage).
            for (float z = start.z; z > endZ; z -= 0.4f) {
                if (boss_->Touches(Vector3{lineX(z), config::kPlayerHeight, z}, halfWidth)) {
                    endZ = z;
                    target = Target::Boss;
                    break;
                }
            }
        }
        // An alien is hit at its middle, so the bolt ends right there.
        const Vector3 end = target == Target::Alien
                                ? Vector3{alienPosition.x, config::kPlayerHeight, endZ}
                                : Vector3{lineX(endZ), config::kPlayerHeight, endZ};
        laser_.SetBolt(i, true, start, end, target != Target::None);

        // The damage comes in "ticks": every kLaserTick seconds one hit on the target.
        float& tick = laserTick_[static_cast<std::size_t>(i)];
        tick -= dt;
        if (tick > 0.0f || target == Target::None) continue;
        tick = config::kLaserTick[level] * (rapidFireTimer_ > 0.0f ? 0.5f : 1.0f);

        switch (target) {
            case Target::Alien: {
                const auto hit = invaders_.TryHit(alienPosition, 0.05f);
                if (!hit) break;
                if (!hit->destroyed) {
                    particles_.Explode(end, sparks);
                    score_ += config::kScorePerHit;
                    break;
                }
                OnAlienDestroyed(*hit);
                // Chain lightning: when the bolt KILLS an alien, the spark jumps on
                // to the nearest aliens around it, one after the other (each alien
                // only once per chain), and hits each of them once.
                std::vector<Vector3> chain{alienPosition};
                Vector3 from = alienPosition;
                for (int jump = 0; jump < config::kLaserChainJumps[level]; ++jump) {
                    const auto next = invaders_.NearestAlien(from, config::kLaserChainRange, chain);
                    if (!next) break;
                    laser_.AddArc(Vector3{from.x, config::kPlayerHeight, from.z},
                                  Vector3{next->x, config::kPlayerHeight, next->z});
                    chain.push_back(*next);
                    if (const auto chainHit = invaders_.TryHit(*next, 0.05f)) {
                        if (chainHit->destroyed) {
                            OnAlienDestroyed(*chainHit);
                        } else {
                            particles_.Explode(*next, sparks);
                            score_ += config::kScorePerHit;
                        }
                    }
                    from = *next;
                }
                break;
            }
            case Target::Bomb:
                bombInWay->alive = false;
                score_ += config::kScorePerBomb;
                particles_.Explode(bombInWay->position, BombExplosion(bombInWay->color));
                AddShake(config::kShakeBomb);
                audio_.Play(Sfx::BombExplosion);
                break;
            case Target::Boss:
                if (const auto hit = boss_->TryHit(end, halfWidth)) {
                    particles_.Explode(end, sparks);
                    if (hit->destroyed) audio_.Play(Sfx::BombExplosion);
                }
                break;
            case Target::None:
                break;
        }
    }
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
    // The ship is destroyed. Its power-ups (and the combo) are lost with it, and
    // the weapon drops one level (never below the level's minimum, kLevelMinWeapon).
    AddShake(config::kShakePlayerHit);
    weaponLevel_ = std::max(config::kLevelMinWeapon[level_], weaponLevel_ - 1);   // never below the level's minimum
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
    } else if (input.move != 0.0f || input.moveUp != 0.0f || input.fire) {
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
#if defined(GAME_ANDROID)
        // Android's back button: a running game pauses; anywhere else the app closes
        // (so pressing back twice during play leaves the game, as players expect).
        if (state_ == GameState::Playing) {
            state_ = GameState::Paused;
        } else {
            quitRequested_ = true;
        }
#else
        if (display::IsFullscreen()) {
            display::ExitFullscreen();
        } else {
            quitRequested_ = true;
        }
#endif
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
    FitCamera();   // the window may have been resized (or the phone rotated)
    camera_.position = Vector3Add(cameraBasePosition_, offset);
    camera_.target   = Vector3Add(cameraBaseTarget_, offset);

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
            // Demo complete: G or a tap on the button opens the full game's store page
            // (once it exists). OpenURL opens the browser, or the Play Store app on a phone.
            if (demoComplete_ && config::kFullGameUrl[0] != '\0' &&
                (input.openStore || (tap && CheckCollisionPointRec(input.pointer, StoreButtonRect())))) {
                OpenURL(config::kFullGameUrl);
                break;
            }
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

    // How many screen pixels is one world unit where the ship is? We ask the
    // camera where points 1 unit apart end up on screen: sideways (X), and up the
    // screen (Z). Because of the perspective, the two are not the same.
    const Vector3 ship = player_.Position();
    const Vector2 a = GetWorldToScreen(ship, camera_);
    const Vector2 b = GetWorldToScreen(Vector3{ship.x + 1.0f, ship.y, ship.z}, camera_);
    const Vector2 c = GetWorldToScreen(Vector3{ship.x, ship.y, ship.z - 1.0f}, camera_);
    const float pixelsPerUnitX = std::max(1.0f, b.x - a.x);
    const float pixelsPerUnitZ = std::max(1.0f, a.y - c.y);   // one unit further = higher on screen

    // A new drag remembers where the finger and the ship started. From then on
    // the ship moves as far as the finger has moved: a "relative" drag, so you can
    // touch anywhere and your finger never hides the ship.
    if (!dragActive_) {
        dragActive_        = true;
        dragStartPointerX_ = input.pointer.x;
        dragStartPointerY_ = input.pointer.y;
        dragStartShipX_    = player_.X();
        dragStartShipZ_    = player_.Z();
    }
    float targetX = dragStartShipX_ +
                    (input.pointer.x - dragStartPointerX_) / pixelsPerUnitX * config::kTouchDragGain;
    // Finger up the screen (smaller y) = ship up the screen (smaller z).
    float targetZ = dragStartShipZ_ +
                    (input.pointer.y - dragStartPointerY_) / pixelsPerUnitZ * config::kTouchDragGain;

    // At a wall: slide the start point along, so moving the finger back moves the
    // ship back immediately (instead of first "unwinding" the extra distance).
    const float limit = config::kFieldHalfWidth - player_.HalfWidth();
    if (targetX > limit)  { dragStartShipX_ -= targetX - limit;  targetX = limit; }
    if (targetX < -limit) { dragStartShipX_ += -limit - targetX; targetX = -limit; }
    const float minZ = player_.MinZ();
    const float maxZ = player_.MaxZ();
    if (targetZ < minZ) { dragStartShipZ_ += minZ - targetZ; targetZ = minZ; }
    if (targetZ > maxZ) { dragStartShipZ_ -= targetZ - maxZ; targetZ = maxZ; }

    steer.targetX = targetX;
    steer.targetZ = targetZ;
    steer.fire    = true;   // while the finger is down, the ship fires by itself
    return steer;
}

void Game::ToggleSound() {
    audio_.ToggleMute();
    storage::SaveInt("muted", audio_.Muted() ? 1 : 0);   // remembered for next time
}

Rectangle Game::MuteButtonRect() const {
    const Rectangle pause = PauseButtonRect();   // same size, just left of it
    return Rectangle{pause.x - pause.width - static_cast<float>(display::Px(10)), pause.y,
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
    const Rectangle column = display::Column();
    const float size = static_cast<float>(Px(120));
    return Rectangle{column.x + (column.width - size) / 2.0f,
                     column.y + column.height / 2.0f - size / 2.0f + static_cast<float>(Px(20)),
                     size, size};
}

Rectangle Game::PauseButtonRect() const {
    // Top right of the column, under the level number.
    using display::Px;
    const Rectangle column = display::Column();
    const float size = static_cast<float>(Px(44));
    return Rectangle{column.x + column.width - static_cast<float>(Px(14)) - size,
                     column.y + static_cast<float>(Px(64)), size, size};
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

    laserFiring_ = false;   // (set again below while the ship is alive and firing)
    if (playerAlive_) {
        // A boss is big and hangs low: during the fight the ship may not fly as far up.
        player_.SetZLimits(boss_ ? std::max(playerMinZ_, config::kPlayerBossMinZ) : playerMinZ_, playerMaxZ_);
        const PlayerInput steer = ApplyTouchSteering(input);
        player_.Update(dt, steer);

        // A cooldown limits the fire rate: it counts down, and we may only shoot
        // when it has reached zero. Holding the button fires every 0.2 seconds.
        // With rapid fire the cooldown is shorter, so we shoot more often.
        // The laser fires as long as the button is held (see UpdateLaser); the
        // other weapons fire volleys of rockets.
        fireCooldown_ -= dt;
        laserFiring_ = weaponType_ == WeaponType::Laser && steer.fire;
        if (weaponType_ != WeaponType::Laser && steer.fire && fireCooldown_ <= 0.0f) {
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
    // The wave's clock (for the speed bonus) starts when the aliens have flown in.
    if (waveActive_ && !invaders_.Entering()) waveTime_ += dt;

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
    UpdateLaser(dt);

    // ---- 4. Collisions ----------------------------------------------------------
    // 4a. Our rockets against the invaders.
    for (auto& rocket : rockets_) {
        if (const auto hit = invaders_.TryHit(rocket.position, config::kRocketRadius)) {
            rocket.alive = false;                 // the rocket is used up
            if (!hit->destroyed) {
                particles_.Explode(hit->position, HitSparks());   // damaged, not dead yet
                score_ += config::kScorePerHit;
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
        laser_.Draw();
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
        invaders_.DrawGlow(glow_, camera_);   // a soft halo behind every alien
        for (const auto& powerUp : powerUps_) {
            glow_.Draw(camera_, powerUp.position, config::kPowerUpOrbSize * 3.6f, powerUp.MainColor());
        }
        if (shieldVisible) {
            glow_.Draw(camera_, player_.Position(), ShieldRadius() * 2.6f, Color{60, 160, 255, 255});
        }
        if (boss_) boss_->DrawGlow(glow_, camera_);
        laser_.DrawGlow(glow_, camera_);
        glow_.End();

        particles_.Draw();   // last: additive blending must be drawn on top of the solid objects

    EndMode3D();

    // The power-up symbols, in 2D on top of the 3D picture (see PowerUp::DrawIcon).
    for (const auto& powerUp : powerUps_) {
        powerUp.DrawIcon(camera_);
    }

    if (state_ == GameState::Menu) {
        DrawMenu();
        DrawMuteButton();
        return;   // no score or banner on the title screen
    }

    // 2D text on top of the 3D picture (the HUD = "heads-up display").
    // Everything is placed inside the play column (see display::Column), and Px()
    // scales every size and position with it, so the HUD looks the same on a small
    // phone, a big tablet and a desktop window.
    using display::Px;
    const Rectangle column = display::Column();
    const float left    = column.x + static_cast<float>(Px(14));
    const float right   = column.x + column.width - static_cast<float>(Px(14));
    const float centerX = column.x + column.width / 2.0f;
    const int   textWidth = static_cast<int>(column.width) - Px(40);   // longest line for wrapped text
    const int   screenH = GetScreenHeight();
    const Color label = Fade(LIGHTGRAY, 0.75f);

    // Top left: the score, with a small label over it, and the lives under it.
    DrawText("SCORE", static_cast<int>(left), Px(10), Px(14), label);
    DrawText(TextFormat("%06d", score_), static_cast<int>(left), Px(26), Px(30), RAYWHITE);
    DrawLives();

    // Top middle: the best score. While you beat it, it counts up with you.
    const Color gold{255, 200, 60, 255};
    DrawCentered("BEST", centerX, Px(10), Px(14), textWidth, Fade(gold, 0.75f));
    DrawCentered(TextFormat("%06d", std::max(highScore_, score_)), centerX, Px(28), Px(22), textWidth, gold);

    // Top right: the level, and the wave under it.
    DrawRight(TextFormat("LEVEL %d", level_ + 1), right, Px(10), Px(26), SKYBLUE);
    const char* waveText = boss_ ? "BOSS"
        : (wave_ < config::kWavesPerLevel)
        ? TextFormat("WAVE %d/%d", wave_ + 1, config::kWavesPerLevel)
        : TextFormat("BONUS WAVE %d", wave_ - config::kWavesPerLevel + 1);
    DrawRight(waveText, right, Px(40), Px(16), Fade(SKYBLUE, 0.75f));

    // The weapon level, under the lives: "POWER" and one bar per level.
    DrawPowerMeter();

    // Small lines on the left, under the power meter: FPS and "SOUND OFF". Each one
    // that is shown moves the next one down, so there are never gaps.
    int lineY = Px(112);
    if (showFps_) {
        DrawText(TextFormat("%d FPS", GetFPS()), static_cast<int>(left), lineY, Px(16), GREEN);
        lineY += Px(22);
    }
    if (audio_.Muted()) {
        DrawText("SOUND OFF", static_cast<int>(left), lineY, Px(16), ORANGE);
    }

    // Active power-ups on the right, under the buttons, with the seconds left.
    if (state_ == GameState::Playing) {
        DrawPauseButton();
    }
    int statusY = Px(116);
    if (shieldTimer_ > 0.0f) {
        DrawRight(TextFormat("SHIELD %d", static_cast<int>(std::ceil(shieldTimer_))), right, statusY, Px(16),
                  Color{80, 220, 255, 255});
        statusY += Px(22);
    }
    if (rapidFireTimer_ > 0.0f) {
        DrawRight(TextFormat("RAPID FIRE %d", static_cast<int>(std::ceil(rapidFireTimer_))), right, statusY,
                  Px(16), Color{255, 225, 60, 255});
    }

    if (boss_) {
        DrawBossBar();   // the boss's health bar sits where the combo would be
    } else {
        DrawCombo();
    }

    // The controls, only during the first seconds of a game (they fade out in the
    // last second), low in the middle where no aliens are yet. The pause screen
    // shows them again.
    if (helpTimer_ > 0.0f) {
        const float alpha = std::min(1.0f, helpTimer_);
        DrawCentered(HelpLine(), centerX, screenH * 60 / 100, Px(18), textWidth, Fade(LIGHTGRAY, alpha));
    }

    // The wave-clear bonus, one line per part.
    if (bonusTimer_ > 0.0f) {
        const float alpha = std::min(1.0f, bonusTimer_ / 0.5f);
        DrawCentered(bonusText_, centerX, screenH * 36 / 100, Px(24), textWidth, Fade(gold, alpha));
    }

    // "SHIELD!" / "RAPID FIRE!" in the middle of the screen right after a pickup.
    if (state_ == GameState::Playing && pickupTimer_ > 0.0f) {
        const float alpha = std::min(1.0f, pickupTimer_ / 0.5f);   // fades out at the end
        DrawCentered(pickupText_, centerX, screenH * 46 / 100, Px(40), textWidth, Fade(pickupColor_, alpha));
    }

    if (state_ == GameState::Playing && levelBannerTimer_ > 0.0f) {
        DrawLevelBanner();
    } else if (state_ == GameState::Playing && waveBannerTimer_ > 0.0f) {
        // The small wave banner, when no big level banner is showing.
        const float alpha = waveBannerTimer_ > 0.4f ? 1.0f : waveBannerTimer_ / 0.4f;
        const char* text = (wave_ < config::kWavesPerLevel)
            ? TextFormat("WAVE %d / %d", wave_ + 1, config::kWavesPerLevel)
            : TextFormat("BONUS WAVE %d", wave_ - config::kWavesPerLevel + 1);
        DrawCentered(text, centerX, screenH * 26 / 100, Px(38), textWidth, Fade(SKYBLUE, alpha));
    }

    if (state_ == GameState::Paused) {
        DrawPaused();
    }

    if (state_ == GameState::GameOver && gameOverTime_ >= config::kGameOverDelay) {
        DrawGameOver();
    }

    DrawMuteButton();   // last, so the dark pause and game-over screens do not cover it
}

// One small ship icon per life, under the score: a triangle with an orange flame.
void Game::DrawLives() const {
    using display::Px;
    const Rectangle column = display::Column();
    float x = column.x + static_cast<float>(Px(14 + 8));
    const float y = static_cast<float>(Px(72));
    const float s = static_cast<float>(Px(8));   // icon size
    for (int i = 0; i < lives_; ++i) {
        // raylib wants the three corners counter-clockwise (as seen on screen).
        DrawTriangle(Vector2{x, y - s}, Vector2{x - s, y + s * 0.8f}, Vector2{x + s, y + s * 0.8f},
                     Color{190, 205, 230, 255});                         // the ship
        DrawTriangle(Vector2{x, y + s * 1.5f}, Vector2{x + s * 0.35f, y + s * 0.8f},
                     Vector2{x - s * 0.35f, y + s * 0.8f}, Color{255, 140, 40, 255});   // flame
        x += static_cast<float>(Px(22));
    }
}

// "POWER", five small bars and the weapon's name under the lives; the filled bars
// are the weapon level.
// The bars get taller from left to right, like a signal meter.
void Game::DrawPowerMeter() const {
    using display::Px;
    const Rectangle column = display::Column();
    const int x = static_cast<int>(column.x) + Px(14);
    const int y = Px(88);
    // The bars and the weapon's name are in the weapon's colour.
    const Color color = lowpoly::Mix(WeaponColor(), Color{255, 255, 255, 255}, 0.25f);
    DrawText("POWER", x, y + Px(3), Px(12), Fade(LIGHTGRAY, 0.75f));
    int barX = x + MeasureText("POWER", Px(12)) + Px(6);
    for (int i = 0; i < config::kWeaponLevels; ++i) {
        const int height = Px(6 + 2 * i);
        const bool on = i < weaponLevel_;
        DrawRectangle(barX, y + Px(14) - height, Px(7), height, on ? color : Fade(GRAY, 0.35f));
        barX += Px(10);
    }
    DrawText(WeaponName(), barX + Px(4), y + Px(3), Px(12), color);
}

void Game::DrawPaused() const {
    using display::Px;
    const Rectangle column = display::Column();
    const float centerX = column.x + column.width / 2.0f;
    const int textWidth = static_cast<int>(column.width) - Px(40);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.55f));

    const Rectangle play = PlayButtonRect();
    DrawCentered("PAUSED", centerX, static_cast<int>(play.y) - Px(96), Px(60), textWidth, RAYWHITE);

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
    int y = DrawCentered(hint, centerX, static_cast<int>(play.y + play.height) + Px(28), Px(20), textWidth,
                         LIGHTGRAY);
    // The controls, for anyone who missed them at the start.
    DrawCentered(HelpLine(), centerX, y + Px(16), Px(17), textWidth, GRAY);
}

const char* Game::HelpLine() const {
    // Only the keys needed while playing; the window keys are listed in the menu.
    return touchMode_ ? "Drag anywhere to move. The ship fires by itself."
                      : "Move: arrows or W A S D     Fire: Space (hold)\nPause: P     Mute: M     FPS: F3";
}

void Game::DrawGameOver() const {
    if (demoComplete_) {
        DrawDemoEnd();
        return;
    }
    using display::Px;
    const Rectangle column = display::Column();
    const float centerX = column.x + column.width / 2.0f;
    const int textWidth = static_cast<int>(column.width) - Px(40);
    const int middle = GetScreenHeight() / 2;

    // Darken the whole picture. Fade(color, alpha) makes a colour see-through.
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));

    int y = DrawCentered("GAME OVER", centerX, middle - Px(160), Px(64), textWidth, RED);
    y = DrawCentered(gameOverReason_, centerX, y + Px(16), Px(24), textWidth, RAYWHITE);
    y = DrawCentered(TextFormat("Final score: %06d", score_), centerX, y + Px(16), Px(30), textWidth, YELLOW);
    if (newHighScore_) {
        // Blinks twice per second: on while the fraction of the time is below one half.
        const bool on = std::fmod(static_cast<float>(GetTime()) * 2.0f, 1.0f) < 0.5f;
        if (on) DrawCentered("NEW HIGH SCORE!", centerX, y + Px(8), Px(24), textWidth, Color{255, 200, 60, 255});
    }
    y += Px(8) + Px(30);   // room for the blinking line, also while it is off
    y = DrawCentered(touchMode_ ? (IsTouchDevice() ? "Tap to play again" : "Click to play again")
                                : "Press ENTER to play again",
                     centerX, y + Px(24), Px(24), textWidth, LIGHTGRAY);
#if defined(GAME_DESKTOP)
    DrawCentered("Esc to quit", centerX, y + Px(10), Px(18), textWidth, GRAY);
#endif
}

void Game::DrawMenu() const {
    using display::Px;
    const Rectangle column = display::Column();
    const float centerX = column.x + column.width / 2.0f;
    const int textWidth = static_cast<int>(column.width) - Px(40);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.55f));

    // A gentle pulse for the "press Enter" line: sin() swings between -1 and +1,
    // we turn that into an opacity between 0.4 and 1.
    const float pulse = 0.7f + 0.3f * sinf(static_cast<float>(GetTime()) * 4.0f);

    const Color purple{190, 120, 255, 255};
    DrawCentered("SPACE", centerX, Px(170), Px(76), textWidth, purple);
    DrawCentered("INVADER", centerX, Px(250), Px(76), textWidth, purple);
    DrawCentered("3D", centerX, Px(330), Px(96), textWidth, SKYBLUE);
    if (highScore_ > 0) {
        DrawCentered(TextFormat("HIGH SCORE  %06d", highScore_), centerX, Px(470), Px(22), textWidth,
                     Color{255, 200, 60, 255});
    }
    int y = Px(530);
    if (touchMode_) {
        y = DrawCentered(IsTouchDevice() ? "Tap to start" : "Click to start", centerX, y, Px(32), textWidth,
                         Fade(RAYWHITE, pulse));
        DrawCentered("Drag anywhere to move. The ship fires by itself.", centerX, y + Px(16), Px(18), textWidth,
                     LIGHTGRAY);
    } else {
        y = DrawCentered("Press ENTER to start", centerX, y, Px(32), textWidth, Fade(RAYWHITE, pulse));
        y = DrawCentered("Move: arrows or W A S D\nFire: Space (hold)     Pause: P", centerX, y + Px(16), Px(18),
                         textWidth, LIGHTGRAY);
#if defined(GAME_DESKTOP)
        DrawCentered("Fullscreen: F     Window size: 1 - 5     Quit: Esc", centerX, y + Px(6), Px(18), textWidth,
                     LIGHTGRAY);
#endif
    }
    if (config::kDemo) {
        DrawCentered(TextFormat("FREE DEMO: levels 1 to 3. The full game with all %d levels is coming to Google Play.",
                                config::kLevelCount),
                     centerX, Px(720), Px(18), textWidth, Color{255, 170, 60, 255});
    }
}

void Game::DrawLevelBanner() const {
    using display::Px;
    const Rectangle column = display::Column();
    const float centerX = column.x + column.width / 2.0f;
    const int textWidth = static_cast<int>(column.width) - Px(40);
    // Fade out during the last half second of the banner time.
    const float alpha = levelBannerTimer_ > 0.5f ? 1.0f : levelBannerTimer_ / 0.5f;
    const int y = DrawCentered(bannerTitle_, centerX, GetScreenHeight() * 24 / 100, Px(52), textWidth,
                               Fade(SKYBLUE, alpha));
    DrawCentered(bannerSubtitle_, centerX, y + Px(6), Px(22), textWidth, Fade(RAYWHITE, alpha));
}

void Game::DrawBossBar() const {
    using display::Px;
    // The boss's name and a health bar, centred under the best score. The bar
    // has a dark background, the remaining health in the boss's colour (red
    // when it is angry), and a thin white frame. It shakes a little when angry.
    const Rectangle column = display::Column();
    const float centerX = column.x + column.width / 2.0f;
    const int width  = static_cast<int>(column.width * 0.42f);
    const int height = Px(10);
    const int shake  = boss_->Angry() ? static_cast<int>(2.0f * std::sin(time_ * 40.0f)) : 0;
    const int x = static_cast<int>(centerX) - width / 2 + shake;
    const int y = Px(80);

    DrawCentered(boss_->Name(), centerX, Px(58), Px(18), width + Px(40), RAYWHITE);

    const Color fill = boss_->Angry() ? Color{255, 60, 60, 255} : boss_->GlowColor();
    DrawRectangle(x, y, width, height, Fade(BLACK, 0.6f));
    DrawRectangle(x, y, static_cast<int>(static_cast<float>(width) * boss_->Health()), height, fill);
    DrawRectangleLines(x, y, width, height, Fade(RAYWHITE, 0.8f));
    // Quarter marks, so the player can see the progress.
    for (int i = 1; i < 4; ++i) {
        DrawLine(x + width * i / 4, y, x + width * i / 4, y + height, Fade(RAYWHITE, 0.5f));
    }
    if (boss_->CoreOpen()) {
        DrawCentered("CORE OPEN!", centerX, y + height + Px(6), Px(16), width + Px(40),
                     Fade(fill, 0.6f + 0.4f * std::sin(time_ * 10.0f)));
    }
}

void Game::DrawCombo() const {
    using display::Px;
    if (comboTimer_ <= 0.0f || ComboMultiplier() < 2) return;   // only worth showing from x2
    // "COMBO x3" under the best score, and a small bar that runs out with the time
    // left to keep the combo going.
    const Rectangle column = display::Column();
    const float centerX = column.x + column.width / 2.0f;
    const Color color = ComboMultiplier() >= config::kComboMaxMultiplier ? Color{255, 90, 200, 255}
                                                                         : Color{120, 230, 255, 255};
    const int y = Px(60);
    DrawCentered(TextFormat("COMBO x%d", ComboMultiplier()), centerX, y, Px(22), static_cast<int>(column.width),
                 color);
    const int width = Px(110);
    DrawRectangle(static_cast<int>(centerX) - width / 2, y + Px(26),
                  static_cast<int>(static_cast<float>(width) * comboTimer_ / config::kComboWindow), Px(4), color);
}

Rectangle Game::StoreButtonRect() const {
    using display::Px;
    const Rectangle column = display::Column();
    const float width  = std::min(static_cast<float>(Px(440)), column.width - static_cast<float>(Px(40)));
    const float height = static_cast<float>(Px(60));
    return Rectangle{column.x + (column.width - width) / 2.0f,
                     static_cast<float>(GetScreenHeight() / 2 + Px(80)), width, height};
}

void Game::DrawDemoEnd() const {
    using display::Px;
    const Rectangle column = display::Column();
    const float centerX = column.x + column.width / 2.0f;
    const int textWidth = static_cast<int>(column.width) - Px(40);
    const int middle  = GetScreenHeight() / 2;
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.65f));

    int y = DrawCentered("DEMO COMPLETE!", centerX, middle - Px(250), Px(52), textWidth, Color{120, 230, 255, 255});
    y = DrawCentered("You beat the Hive Queen. Well done!", centerX, y + Px(16), Px(24), textWidth, RAYWHITE);
    y = DrawCentered(TextFormat("Score: %06d", score_), centerX, y + Px(12), Px(28), textWidth, YELLOW);
    DrawCentered("The full game has 7 more levels, two more ships and five more bosses, "
                 "including the crab family with their giant scissor claws.",
                 centerX, y + Px(20), Px(20), textWidth, LIGHTGRAY);

    // The store button: a link once the app is published, otherwise "coming soon".
    const Rectangle button = StoreButtonRect();
    const bool live = config::kFullGameUrl[0] != '\0';
    const float pulse = 0.6f + 0.4f * sinf(static_cast<float>(GetTime()) * 3.0f);
    DrawRectangleRounded(button, 0.3f, 8, Fade(Color{40, 140, 90, 255}, live ? 0.9f : 0.4f));
    DrawRectangleRoundedLinesEx(button, 0.3f, 8, 2.0f, Fade(Color{150, 255, 170, 255}, live ? pulse : 0.4f));
    const char* label = live ? "Get the full game on Google Play" : "Full game coming soon to Google Play";
    // The label in one line if it fits, otherwise smaller.
    int labelSize = Px(20);
    while (labelSize > Px(12) && MeasureText(label, labelSize) > static_cast<int>(button.width) - Px(16)) {
        --labelSize;
    }
    DrawCentered(label, button.x + button.width / 2.0f,
                 static_cast<int>(button.y + (button.height - static_cast<float>(labelSize)) / 2.0f), labelSize,
                 static_cast<int>(button.width), RAYWHITE);
    if (live && !touchMode_) {
        DrawCentered("(or press G)", centerX, static_cast<int>(button.y + button.height) + Px(8), Px(18), textWidth,
                     GRAY);
    }

    DrawCentered(touchMode_ ? "Tap anywhere else to play again" : "Press ENTER to play again",
                 centerX, static_cast<int>(button.y + button.height) + Px(70), Px(22), textWidth, LIGHTGRAY);
}
