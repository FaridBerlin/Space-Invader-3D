# Architecture

Small and simple on purpose: easy to read while learning, and easy to compile for every platform.

## Source layout

```
src/
  main.cpp            entry point, window, main loop (desktop and web)
  Game.h/.cpp         owns all game state: states, levels and waves, collisions, HUD and screens
  Config.h            every tuning number in one place, with one table entry per level
  Input.h/.cpp        the only place that reads keys, mouse and touch
  Display.h/.cpp      window size presets, fullscreen, scaling of the 2D text
  Player.h/.cpp       the ship: movement, upgrade tiers, the Claw design
  Invader.h/.cpp      one alien: the six types (Wasp and Moth too), colours per level, dive / fly-in state
  InvaderGrid.h/.cpp  the wave: shapes, fly-in curves, sideways sweep, step forward, divers, who shoots
  AlienModels.h/.cpp  optional .glb models (the Commander), with a built-in fallback
  Rocket.h/.cpp       the player's rockets
  InvaderShot.h/.cpp  the aliens' shots
  Bomb.h/.cpp         the bombs, one design per level
  Boss.h/.cpp         the three bosses: model, movement, attack patterns, weak core
  PowerUp.h/.cpp      the orbs: weapon, spread, laser, shield, rapid fire, extra life (3D ball + 2D symbol)
  Laser.h/.cpp        the lightning laser's look (Game::UpdateLaser decides what it hits)
  Particles.h/.cpp    explosions made of small glowing cubes
  Starfield.h/.cpp    the stars, tinted per level
  GlowRenderer.h/.cpp soft additive glow, our first custom shader
  Nebula.h/.cpp       the drifting colour clouds in the background, our second shader
  ShaderUtil.h/.cpp   loads a shader in the right GLSL dialect for desktop, web or phone
  LowPoly.h/.cpp      flat-shaded triangles for the ship and the aliens
  Audio.h/.cpp        sound effects and mute
  Storage.h/.cpp      small saved numbers (high score, mute): a file on desktop, localStorage on the web
  AssetPath.h, Random.h, Util.h   small helpers
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
Menu --(Start)--> Playing --(last life lost / wave reached the ship)--> GameOver --(Restart)--> Playing
                    ^   |
     (play button / P)  (pause button / P / window in the background)
                    |   v
                   Paused
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
