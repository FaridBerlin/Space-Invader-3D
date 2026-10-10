# Architecture

Small and simple on purpose: easy to read while learning, and easy to compile for every platform.

## Source layout (planned)

```
src/
  main.cpp          entry point, window, main loop
  Game.h/.cpp       owns all game state, update() and draw()
  GameState.h       enum { Menu, Playing, GameOver }
  Player.h/.cpp
  Invader.h/.cpp    and the InvaderGrid (movement, shooting)
  Projectile.h/.cpp player rockets and invader shots
  Bomb.h/.cpp
  Particles.h/.cpp  explosions and starfield
  Input.h/.cpp      one place that turns keyboard / touch into "move" and "fire"
  Assets.h/.cpp     loads and owns models, sounds, shaders
  Config.h          every tuning number in one place (see DESIGN.md)
```

## Rules we follow

1. **Delta time everywhere.** `update(float dt)`, speeds in units per second. Never count frames for gameplay.
2. **Update, then clean up.** Entities have an `alive` flag; dead ones are removed after the loop
   (the erase-remove idiom on a `std::vector`; `std::erase_if` needs C++20). Never erase while iterating.
3. **Game logic does not call drawing code and vice versa.** `update()` changes data, `draw()` only reads it.
4. **All tuning numbers live in `Config.h`.**
5. **Input goes through `Input`.** Game code asks "is the player moving left?", never "is the A key down?".
   Touch controls on Android then only touch this one file.
6. **Cross-platform main loop.** Desktop uses `while (!WindowShouldClose())`. The web build cannot block, so it uses
   `emscripten_set_main_loop`. We write a single `frame()` function and call it either way.
7. **Assets are loaded with relative paths from `assets/`.** The web build preloads this folder into Emscripten's virtual filesystem.
8. **Shaders are written for GLSL 330 (desktop) and GLSL ES 100/300 (web, Android)**, with the right header chosen at load time.
9. **No platform-specific code outside small `#ifdef` blocks** (`PLATFORM_WEB`, `PLATFORM_ANDROID`).

## Game flow

```
Menu --(Start)--> Playing --(player hit)--> GameOver --(Restart)--> Playing
```

## Build

One `CMakeLists.txt` for all targets. raylib is fetched with CMake `FetchContent` at a pinned release tag, so a fresh
clone builds with no manual library install.

| Target | Command idea |
|---|---|
| Desktop | `cmake -B build && cmake --build build` |
| Web | `emcmake cmake -B build-web -DPLATFORM=Web && cmake --build build-web` |
| Android | later, via NDK |

## CI (GitHub Actions)

- On every push: build Linux and Windows, upload the zipped builds as artifacts.
- On push to `main`: build the web version and deploy it to GitHub Pages.
