// Game.h - owns everything that happens during play.
//
// main.cpp only creates the window and the loop. Game holds the player, the
// rockets, the invaders and the score, and decides how they interact.

#pragma once

#include <vector>

#include "AlienModels.h"
#include "Audio.h"
#include "Bomb.h"
#include "GlowRenderer.h"
#include "InvaderGrid.h"
#include "InvaderShot.h"
#include "Particles.h"
#include "Player.h"
#include "PowerUp.h"
#include "Rocket.h"
#include "Starfield.h"
#include "raylib.h"

// An "enum class" is a type with a fixed list of named values. The game is
// always in exactly ONE of these states, which keeps the rules simple:
// what Update and Draw do depends on the current state.
enum class GameState {
    Menu,       // title screen, waiting for Enter
    Playing,
    Paused,     // the world is frozen until P is pressed again
    GameOver,
};

class Game {
public:
    Game();

    void Update(float dt);
    void Draw() const;

    // True once the player asked to quit (Esc outside fullscreen). main.cpp
    // then ends the loop and closes the window.
    bool QuitRequested() const { return quitRequested_; }

private:
    void Restart();                       // back to a fresh game
    void EndGame(const char* reason);     // switch to GameOver
    void UpdatePlaying(float dt, const PlayerInput& input);
    void FireRocket();
    void OnAlienDestroyed(const InvaderHit& hit);   // score, explosion, sound, maybe a power-up
    void Collect(PowerUpType type);
    // Something hit the ship. With a shield, the shield takes the hit and this
    // returns false. Without one, the ship is destroyed and this returns true.
    bool DamagePlayer(const char* reason);
    void DrawMenu() const;
    void DrawLevelBanner() const;
    void DrawGameOver() const;
    void DrawPaused() const;
    void DrawLives() const;
    void RespawnPlayer();
    void DrawPauseButton() const;
    // Turns a finger drag into steering (targetX) and auto-fire. Returns the input
    // the ship should use this frame.
    PlayerInput ApplyTouchSteering(const PlayerInput& input);
    Rectangle PauseButtonRect() const;   // the on-screen pause button (top right)
    Rectangle PlayButtonRect() const;    // the big play button on the pause screen

    Camera3D camera_{};
    Player   player_;
    InvaderGrid invaders_;
    std::vector<Rocket>       rockets_;
    std::vector<InvaderShot>  invaderShots_;
    std::vector<PowerUp>      powerUps_;
    std::vector<Bomb>         bombs_;
    ParticleSystem            particles_;
    Starfield                 stars_;
    Audio                     audio_;     // sound effects, see Audio.h
    GlowRenderer              glow_;      // owns the shader; see GlowRenderer.h
    AlienModels               alienModels_;   // optional 3D model files, see AlienModels.h

    GameState state_       = GameState::Menu;
    bool  playerAlive_     = true;     // false once the ship is destroyed (it is then not drawn)
    const char* gameOverReason_ = "";  // text shown on the Game Over screen
    float gameOverTime_    = 0.0f;     // seconds since the game ended

    int   score_           = 0;
    int   level_           = 0;      // 0-based index into the level tables (shown as level_ + 1)
    float levelBannerTimer_ = 0.0f;  // seconds left to show the "LEVEL n" banner
    float fireCooldown_    = 0.0f;   // seconds until the next rocket may be fired
    float invaderShotTimer_ = 0.0f;  // seconds until the wave fires its next shot
    float bombTimer_       = 0.0f;   // seconds until the next bomb appears
    float time_            = 0.0f;   // seconds since the program started, for animations
    float respawnTimer_    = 0.0f;   // counts down after a wave is destroyed
    bool  touchMode_       = false;  // playing with a finger/mouse: show touch hints and the pause button
    bool  dragActive_      = false;  // a finger is down and steering the ship
    float dragStartPointerX_ = 0.0f; // where the finger was when the drag began (pixels)
    float dragStartShipX_  = 0.0f;   // where the ship was when the drag began (world units)
    int   lives_           = 0;      // ships left, including the one flying now
    float playerRespawnTimer_ = 0.0f;   // > 0 while waiting for the next ship
    float invincibleTimer_ = 0.0f;   // > 0 while a fresh ship blinks and cannot be hit
    int   highScore_       = 0;      // best score ever, loaded at start (see HighScore.h)
    bool  newHighScore_    = false;  // did this game beat it?
    float shieldTimer_     = 0.0f;   // > 0 while the shield is up (seconds left)
    float rapidFireTimer_  = 0.0f;   // > 0 while rapid fire is active (seconds left)
    float pickupTimer_     = 0.0f;   // > 0 while the "SHIELD!" / "RAPID FIRE!" text shows
    const char* pickupText_ = "";
    Color pickupColor_{255, 255, 255, 255};
    bool  quitRequested_   = false;
};
