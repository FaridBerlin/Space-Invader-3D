# Space Invader 3D

A 3D remake of my 2D browser game [Space Dash Invaders](../space-invader), written in **C++** with **raylib**.
It is also a learning project: the goal is to understand C++ and how real-time 3D graphics work, one step at a time.

> **Status:** planning. See [`docs/ROADMAP.md`](docs/ROADMAP.md) for what comes next.

## Goals

1. Build a good-looking, playable 3D Space Invaders (neon low-poly style).
2. Learn modern C++ and the graphics pipeline (camera, meshes, shaders, particles).
3. One codebase, one-click play on **Windows, Linux and the web**, with **Android** as a later phase.

## Planned gameplay

Everything from the 2D version carries over: move, hold to shoot, waves of invaders, falling bombs worth bonus points,
3 levels that get harder, particle explosions, score, game over and restart. In 3D, you fly behind your ship and
shoot into a wall of invaders coming toward you.

Controls (desktop): `A`/`D` or arrow keys to move, `Space` to shoot (hold for rapid fire).
Controls (touch, later): on-screen buttons or tilt.

## Tech stack

| Part | Choice |
|---|---|
| Language | C++17 or newer |
| Graphics / audio / input | [raylib](https://www.raylib.com/) |
| Build | CMake |
| Web build | Emscripten (WebAssembly + WebGL) |
| Android (later) | Android NDK |
| CI | GitHub Actions (builds + GitHub Pages deploy) |

## Documentation

- [`docs/DESIGN.md`](docs/DESIGN.md) - gameplay rules, numbers ported from the 2D game, 3D design decisions
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) - code structure and the cross-platform rules we follow
- [`docs/ROADMAP.md`](docs/ROADMAP.md) - milestones with checkboxes
- [`docs/SETUP.md`](docs/SETUP.md) - installing the tools, building, and first Git push

## Project layout (planned)

```
space-invader-3d/
  CMakeLists.txt
  src/                 C++ source
  assets/              models, sounds, textures
  web/                 HTML shell for the browser build
  docs/                the documents above
  .github/workflows/   automatic builds
```

## License

To be decided (the 2D project used an MIT-style license).
