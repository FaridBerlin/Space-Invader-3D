// main.cpp - Milestone 3 (part 1): rockets, invaders and a score.
//
// main.cpp is now tiny on purpose. It creates the window and runs the loop.
// Everything that happens in the game lives in the Game class (Game.cpp),
// which in turn uses Player, InvaderGrid, Invader and Rocket.

#include <algorithm>   // std::min

#include "Config.h"
#include "Game.h"
#include "raylib.h"

int main() {
    // MSAA = multi-sample anti-aliasing: smooths jagged edges of 3D shapes.
    // This flag must be set BEFORE the window is created.
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(config::kScreenWidth, config::kScreenHeight, "Space Invader 3D");
    SetTargetFPS(60);

    // The extra { } braces create a SCOPE. Game now owns GPU objects (the glow
    // shader), and they must be freed while the window (the graphics context)
    // still exists. C++ destroys a variable at the closing brace of its scope,
    // so `game` is cleaned up BEFORE CloseWindow() runs below.
    {
        Game game;   // creates the player, the first wave of invaders and the camera

        while (!WindowShouldClose()) {
            // Never let one very long frame become a huge time step (see Config.h).
            const float dt = std::min(GetFrameTime(), config::kMaxFrameTime);

            game.Update(dt);

            BeginDrawing();
                ClearBackground(Color{5, 5, 20, 255});
                game.Draw();
            EndDrawing();
        }
    }   // <- `game` is destroyed here, and its destructor frees the shader

    CloseWindow();
    return 0;
}
