// main.cpp - Milestone 0: open a window and run a game loop.
//
// Every game, from Pong to the biggest 3D titles, is built around the same idea:
//
//     while (the game is running) {
//         1. read input
//         2. update the game state
//         3. draw the frame
//     }
//
// This file is the smallest possible version of that loop.

#include "raylib.h"   // the raylib library: window, drawing, input, audio

int main() {
    // Window size in pixels. "constexpr" = a constant the compiler knows while compiling.
    constexpr int kScreenWidth  = 1280;
    constexpr int kScreenHeight = 720;

    // Create the window and the OpenGL context that the GPU will draw into.
    InitWindow(kScreenWidth, kScreenHeight, "Space Invader 3D");

    // Ask raylib to limit us to 60 frames per second so we don't burn 100% CPU/GPU.
    SetTargetFPS(60);

    // The game loop. WindowShouldClose() becomes true when the user clicks the
    // window's close button or presses ESC (raylib's default exit key).
    while (!WindowShouldClose()) {

        // --- 1 + 2. Input and update -------------------------------------
        // Nothing to do yet. In later milestones the player ship goes here.

        // GetFrameTime() = seconds the last frame took (about 0.0167 at 60 FPS).
        // We don't use it yet, but from M1 on every movement will be multiplied by it,
        // so the game runs at the same speed on any computer.
        // const float dt = GetFrameTime();

        // --- 3. Draw ------------------------------------------------------
        // Everything we draw must happen between BeginDrawing() and EndDrawing().
        // EndDrawing() also swaps the finished image onto the screen.
        BeginDrawing();

            // Fill the whole frame with a dark space colour (R, G, B, Alpha: 0-255 each).
            ClearBackground(Color{5, 5, 20, 255});

            DrawText("SPACE INVADER 3D", 40, 40, 48, RAYWHITE);
            DrawText("Milestone 0: the window works.", 40, 100, 24, LIGHTGRAY);

            // Draws "NN FPS" in the top-left area.
            DrawFPS(40, 150);

        EndDrawing();
    }

    // Free the window and the GPU resources before the program ends.
    CloseWindow();
    return 0;   // 0 = "finished without errors"
}
