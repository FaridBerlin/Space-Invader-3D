// main.cpp - creates the window and runs the game loop.
//
// main.cpp stays tiny on purpose. Everything that happens in the game lives in
// the Game class (Game.cpp).
//
// The same code runs on desktop, on Android and in the browser. They only differ
// in HOW the loop is driven:
//   * Desktop and Android: we write the loop ourselves: "while the window is open,
//     do a frame". (On Android, raylib starts our main() from its own entry point
//     android_main, and ends the app when main() returns.)
//   * Web: a browser tab must never be blocked by a loop that never returns, or
//     the page would freeze. So we hand the browser a function and it calls it
//     once per screen refresh (60 times per second). That is emscripten_set_main_loop.
// Both call the very same Frame() function, so there is one game, not two.

#include <algorithm>   // std::min

#include "Config.h"
#include "Display.h"
#include "Game.h"
#include "Platform.h"
#include "raylib.h"

#if defined(GAME_WEB)
#include <emscripten/emscripten.h>
#endif

namespace {

// One iteration of the game: update, then draw.
void Frame(Game& game) {
    // Never let one very long frame become a huge time step (see Config.h).
    const float dt = std::min(GetFrameTime(), config::kMaxFrameTime);

    game.Update(dt);

    BeginDrawing();
        ClearBackground(Color{5, 5, 20, 255});
        game.Draw();
    EndDrawing();
}

#if defined(GAME_WEB)
// The browser's callback type takes a "void*" (a pointer to anything). We pass
// the Game in, and cast the pointer back to what it really is.
void FrameForBrowser(void* gamePointer) {
    Frame(*static_cast<Game*>(gamePointer));
}
#endif

}  // namespace

int main() {
    // Opens a resizable window sized for your monitor (see Display.cpp).
    display::OpenWindow("Space Invader 3D");

#if !defined(GAME_WEB)
    SetTargetFPS(60);   // in the browser, the screen refresh already sets the pace
#endif

    // The extra { } braces create a SCOPE. Game owns GPU objects (the glow
    // shader), and they must be freed while the window (the graphics context)
    // still exists. C++ destroys a variable at the closing brace of its scope,
    // so `game` is cleaned up BEFORE CloseWindow() runs below.
    {
        Game game;   // creates the player, the first wave of invaders and the camera

#if defined(GAME_WEB)
        // Arguments: our function, what to pass it, frames per second (0 = let the
        // browser decide), and "simulate infinite loop" (1 = do not return from main).
        emscripten_set_main_loop_arg(FrameForBrowser, &game, 0, 1);
#else
        // WindowShouldClose() is true when the window's close button (X) is
        // clicked; QuitRequested() when the player pressed Esc.
        while (!WindowShouldClose() && !game.QuitRequested()) {
            Frame(game);
        }
#endif
    }   // <- `game` is destroyed here (desktop), and its destructors free the GPU objects

    CloseWindow();
    return 0;
}
