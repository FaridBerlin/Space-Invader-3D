// main.cpp - Milestone 1: a 3D scene with a camera and delta time.
//
// New in this step:
//   * A Camera3D: the "eye" through which we look at the 3D world.
//   * BeginMode3D / EndMode3D: drawing between them uses 3D coordinates.
//   * A ground grid and coloured axes so we can SEE the coordinate system.
//   * Delta time: movement is "units per second", not "units per frame".

#include <cmath>      // std::sin

#include "Config.h"   // our tuning numbers
#include "raylib.h"

int main() {
    // MSAA = multi-sample anti-aliasing: smooths jagged edges of 3D shapes.
    // This flag must be set BEFORE the window is created.
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(config::kScreenWidth, config::kScreenHeight, "Space Invader 3D");
    SetTargetFPS(60);

    // --- The camera -----------------------------------------------------------
    // A camera is defined by where it is, what it looks at, and which way is up.
    // "fovy" is how wide the lens is, and CAMERA_PERSPECTIVE makes far things
    // look smaller (like real eyes). The alternative, orthographic, has no
    // perspective at all.
    Camera3D camera{};   // the {} sets every field to zero first
    camera.position   = config::kCameraPosition;
    camera.target     = config::kCameraTarget;
    camera.up         = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy       = config::kCameraFovDegrees;
    camera.projection = CAMERA_PERSPECTIVE;

    // Total seconds since the game started. We add dt to it every frame.
    float elapsed = 0.0f;

    while (!WindowShouldClose()) {

        // --- Update ------------------------------------------------------------
        // dt = "delta time" = seconds that passed since the previous frame.
        // At 60 FPS it is about 0.0167; at 144 FPS about 0.0069.
        // If we moved something by a fixed amount per frame, a 144 Hz monitor
        // would make it 2.4x faster than a 60 Hz one. Multiplying speeds by dt
        // makes the game run at the same real-world speed everywhere.
        const float dt = GetFrameTime();
        elapsed += dt;

        // Swing a test cube left and right along X with a sine wave.
        // sin() goes smoothly between -1 and +1, so the cube travels from
        // -kFieldHalfWidth to +kFieldHalfWidth. "elapsed * speed" is the angle.
        const float testCubeX =
            std::sin(elapsed * config::kTestCubeSpeed) * config::kFieldHalfWidth;
        const Vector3 testCubePosition{testCubeX, 0.5f, 0.0f};

        // --- Draw --------------------------------------------------------------
        BeginDrawing();
            ClearBackground(Color{5, 5, 20, 255});

            // Everything between BeginMode3D and EndMode3D is drawn in 3D,
            // as seen from our camera.
            BeginMode3D(camera);

                // Ground grid: 60 squares, 1 unit each, centred on the origin.
                DrawGrid(60, 1.0f);

                // Outline of the play field on the ground (y = 0).
                const float w = config::kFieldHalfWidth;
                const float l = config::kFieldLength;
                DrawLine3D(Vector3{-w, 0, 0},  Vector3{-w, 0, -l}, SKYBLUE);  // left edge
                DrawLine3D(Vector3{ w, 0, 0},  Vector3{ w, 0, -l}, SKYBLUE);  // right edge
                DrawLine3D(Vector3{-w, 0, -l}, Vector3{ w, 0, -l}, SKYBLUE);  // far edge
                DrawLine3D(Vector3{-w, 0, 0},  Vector3{ w, 0, 0},  SKYBLUE);  // near edge

                // Axes at the origin: red = X, green = Y, blue = Z.
                // Drawn after the outline, otherwise the red X axis would be
                // hidden: it lies exactly on top of the near edge.
                DrawLine3D(Vector3{0, 0, 0}, Vector3{5, 0, 0}, RED);
                DrawLine3D(Vector3{0, 0, 0}, Vector3{0, 5, 0}, GREEN);
                DrawLine3D(Vector3{0, 0, 0}, Vector3{0, 0, 5}, BLUE);

                // The test cube: a solid cube plus a wireframe outline.
                DrawCube(testCubePosition, 1.0f, 1.0f, 1.0f, ORANGE);
                DrawCubeWires(testCubePosition, 1.0f, 1.0f, 1.0f, MAROON);

            EndMode3D();

            // 2D text is drawn AFTER EndMode3D, on top of the 3D picture.
            DrawText("SPACE INVADER 3D  -  M1: camera and delta time", 20, 16, 24, RAYWHITE);
            DrawText(TextFormat("dt: %.4f s   elapsed: %.1f s", dt, elapsed), 20, 48, 20, LIGHTGRAY);
            DrawText("Red = X (right)   Green = Y (up)   Blue = Z (toward you)", 20, 76, 20, LIGHTGRAY);
            DrawFPS(20, 104);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
