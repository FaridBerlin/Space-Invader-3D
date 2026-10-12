// Game.h - owns everything that happens during play.
//
// main.cpp only creates the window and the loop. Game holds the player, the
// rockets, the invaders and the score, and decides how they interact.

#pragma once

#include <vector>

#include "Audio.h"
#include "Bomb.h"
#include "GlowRenderer.h"
#include "InvaderGrid.h"
#include "InvaderShot.h"
#include "Particles.h"
#include "Player.h"
#include "Rocket.h"
#include "Starfield.h"
#include "raylib.h"

// An "enum class" is a type with a fixed list of named values. The game is
// always in exactly ONE of these states, which keeps the rules simple:
// what Update and Draw do depends on the current state.
enum class GameState {
    Menu,       // title screen, waiting for Enter
    Playing,
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
    void DrawMenu() const;
    void DrawLevelBanner() const;
    void DrawGameOver() const;

    Camera3D camera_{};
    Player   player_;
    InvaderGrid invaders_;
    std::vector<Rocket>       rockets_;
    std::vector<InvaderShot>  invaderShots_;
    std::vector<Bomb>         bombs_;
    ParticleSystem            particles_;
    Starfield                 stars_;
    Audio                     audio_;     // sound effects, see Audio.h
    GlowRenderer              glow_;      // owns the shader; see GlowRenderer.h

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
    bool  quitRequested_   = false;
};
