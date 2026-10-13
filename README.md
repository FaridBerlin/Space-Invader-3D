# Space Invader 3D

**[▶ Play it in your browser](https://faridberlin.github.io/Space-Invader-3D/)** (needs a keyboard)

A 3D remake of my 2D browser game *Space Dash Invaders*, written in **C++17** with
[raylib](https://www.raylib.com/). It is also a learning project: the goal is to understand C++ and how
real-time 3D graphics work, built one small, reviewed step at a time.

![Menu](docs/img/menu.png)

| | |
|---|---|
| ![Action](docs/img/action.png) | ![Bomb](docs/img/bomb.png) |

## Play

| Platform | How |
|---|---|
| **Web** | Open the link above. Click the game once so it gets the keyboard. |
| **Windows / Linux** | Download the latest zip / tar.gz from [Releases](../../releases), unpack it and start `space-invader-3d`. |
| **From source** | See [Build it yourself](#build-it-yourself). |

**Controls:** `A`/`D` or arrow keys to move, `Space` to shoot (hold for rapid fire), `Enter` to start / restart, `M` to mute. On desktop also `F` for fullscreen, `1`-`5` to pick the window size (you can drag the window edges too) and `Esc` to leave fullscreen or quit.

## The game

- Waves of invaders sweep sideways and step toward you. Shoot them before they reach you. They shoot back.
- Four alien types: **Scarabs** (100), **Reapers** (200, shoot more often), crystal **Drones** (150) and
  **Commanders** (500, three hits). Every few seconds one alien **dives** at you in an S-curve.
- Shot aliens sometimes drop a **power-up**: a **shield** that absorbs one hit, or **rapid fire**.
- Your ship **upgrades** at every level: bigger, new colours, two and then three rockets per shot.
- A glowing **bomb** flies in every few seconds. Shoot it for **+500** points, or dodge it.
- **3 levels** (at 10,000 and 20,000 points): invader shots and bombs get faster, bombs get bigger and redder.
- Particle explosions, a streaming starfield, glowing projectiles (a custom GLSL shader) and sound effects.

## Build it yourself

You need a C++17 compiler, CMake and Git. On Linux also a few development libraries (see [`docs/SETUP.md`](docs/SETUP.md)).

```bash
cmake -B build
cmake --build build
./build/space-invader-3d
```

The first run downloads raylib automatically. The web version needs Emscripten, see [`docs/WEB.md`](docs/WEB.md).

## Tech stack

| Part | Choice |
|---|---|
| Language | C++17 |
| Graphics / audio / input | raylib 5.5 (OpenGL 3.3 on desktop, WebGL 1 in the browser) |
| Build | CMake |
| Web build | Emscripten (WebAssembly) |
| CI / hosting | GitHub Actions, GitHub Pages and Releases |
| Android | planned (NDK, touch controls) |

## Documentation

- [`docs/DESIGN.md`](docs/DESIGN.md): gameplay rules, numbers ported from the 2D game, 3D design decisions
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md): code structure and the cross-platform rules we follow
- [`docs/ROADMAP.md`](docs/ROADMAP.md): milestones with checkboxes
- [`docs/SETUP.md`](docs/SETUP.md): tools, building, publishing releases
- [`docs/WEB.md`](docs/WEB.md): building and testing the web version

## Project layout

```
space-invader-3d/
  CMakeLists.txt
  src/                 C++ source (Game, Player, InvaderGrid, Bomb, Audio, GlowRenderer, ...)
  assets/audio/        sound effects
  web/shell.html       the page around the browser build
  docs/                documentation and screenshots
  .github/workflows/   automatic builds, web deploy, releases
```

## Credits

- Built with [raylib](https://www.raylib.com/) (zlib license).
- Sound effects from the original 2D game.

## License

To be decided.
