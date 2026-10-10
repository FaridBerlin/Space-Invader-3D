// Game.h - owns everything that happens during play.
//
// main.cpp only creates the window and the loop. Game holds the player, the
// rockets, the invaders and the score, and decides how they interact.

#pragma once

#include <vector>

#include "InvaderGrid.h"
#include "Player.h"
#include "Rocket.h"
#include "raylib.h"

class Game {
public:
    Game();

    void Update(float dt);
    void Draw() const;

private:
    void FireRocket();

    Camera3D camera_{};
    Player   player_;
    InvaderGrid invaders_;
    std::vector<Rocket> rockets_;

    int   score_         = 0;
    float fireCooldown_  = 0.0f;   // seconds until the next rocket may be fired
    float respawnTimer_  = 0.0f;   // counts down after a wave is destroyed
};
