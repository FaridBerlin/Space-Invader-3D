// main.cpp - Milestone 2: a ship you can fly.
//
// New in this step:
//   * Input.h/.cpp  : reads the keyboard and turns it into "what the player wants".
//   * Player.h/.cpp : our first class. It moves, banks and draws the ship.
//   * main.cpp stays short: it only connects the pieces in the game loop.

#include "Config.h"   // our tuning numbers
#include "Input.h"
#include "Player.h"
#include "raylib.h"

int main() {
    // MSAA = multi-sample anti-aliasing: smooths jagged edges of 3D shapes.
    // This flag must be set BEFORE the window is created.
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(config::kScreenWidth, config::kScreenHeight, "Space Invader 3D");
    SetTargetFPS(60);

    // --- The camera (explained in M1) -------------------------------------------
    Camera3D camera{};
    camera.position   = config::kCameraPosition;
    camera.target     = config::kCameraTarget;
    camera.up         = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy       = config::kCameraFovDegrees;
    camera.projection = CAMERA_PERSPECTIVE;

    // Creating the ship: this calls Player's constructor.
    Player player;

    while (!WindowShouldClose()) {

        // --- Input + update ------------------------------------------------------
        const float dt = GetFrameTime();

        const PlayerInput input = ReadInput();   // 1. what does the player want?
        player.Update(dt, input);                // 2. move the ship accordingly

        // --- Draw ----------------------------------------------------------------
        BeginDrawing();
            ClearBackground(Color{5, 5, 20, 255});

            BeginMode3D(camera);

                DrawGrid(60, 1.0f);

                // Outline of the play field on the ground (y = 0).
                const float w = config::kFieldHalfWidth;
                const float l = config::kFieldLength;
                DrawLine3D(Vector3{-w, 0, 0},  Vector3{-w, 0, -l}, SKYBLUE);  // left edge
                DrawLine3D(Vector3{ w, 0, 0},  Vector3{ w, 0, -l}, SKYBLUE);  // right edge
                DrawLine3D(Vector3{-w, 0, -l}, Vector3{ w, 0, -l}, SKYBLUE);  // far edge
                DrawLine3D(Vector3{-w, 0, 0},  Vector3{ w, 0, 0},  SKYBLUE);  // near edge

                player.Draw();

            EndMode3D();

            // 2D text is drawn AFTER EndMode3D, on top of the 3D picture.
            DrawText("SPACE INVADER 3D  -  M2: player ship", 20, 16, 24, RAYWHITE);
            DrawText("Move: A / D or Left / Right arrows", 20, 48, 20, LIGHTGRAY);
            DrawText(TextFormat("ship x: %.1f", player.X()), 20, 76, 20, LIGHTGRAY);
            DrawFPS(20, 104);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
