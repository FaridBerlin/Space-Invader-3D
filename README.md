# Space Invader 3D

**[▶ Play the free demo in your browser](https://faridberlin.github.io/Space-Invader-3D/)** (levels 1-3; keyboard, mouse or touch screen).
The full game with all 10 levels is coming to Google Play.

A 3D remake of my 2D browser game *Space Dash Invaders*, written in **C++17** with
[raylib](https://www.raylib.com/). It is also a learning project: the goal is to understand C++ and how
real-time 3D graphics work, built one small, reviewed step at a time.

| | | | |
|:---:|:---:|:---:|:---:|
| ![The title screen](docs/img/menu.jpg) | ![Level 2: Wasps, Drones and Scarabs fly in, the ship fires three shots](docs/img/play-level-2.jpg) | ![Level 6: the Claw ship fires the green spread shot at giant Moths](docs/img/play-level-6.jpg) | ![Level 9: the purple Horn fires three lightning bolts into the wave](docs/img/play-level-9.jpg) |
| Title screen | Level 2 | Level 6: spread shot | Level 9: lightning laser |

### Ten levels, ten colour moods

Every level brings new aliens and its own colours, and the aliens get tougher: 3 hits in level 1, up to 12 in level 10.

![The alien waves of levels 1 to 10](docs/img/aliens.jpg)

### Six bosses

![The six bosses: Hive Queen, War Titan, Void Emperor, and the crab family Shear King, Shear Emperor and Crimson Tyrant](docs/img/bosses.jpg)

### Your ship through the levels

![The ship in levels 1-3 and 4, the Claw in levels 5 and 6-7, and the Horn in green, purple and red in levels 8, 9 and 10](docs/img/ships.jpg)

### Power-up orbs

![The six orbs: weapon, spread, laser, rapid fire, shield and extra life](docs/img/orbs.jpg)

## Play

| Platform | How |
|---|---|
| **Web (free demo)** | Open the link above. Click the game once so it gets the keyboard. |
| **Windows / Linux (free demo)** | Download the latest zip / tar.gz from [Releases](../../releases), unpack it and start `space-invader-3d`. |
| **Android (full game)** | Coming soon to Google Play. |

The demo contains levels 1 to 3 and the first boss. The full game adds levels 4 to 10, the Claw and the Horn ships,
the lightning laser's full power, and five more bosses: the War Titan, the Void Emperor and the three crabs.

**Touch / mouse:** drag anywhere to move, the ship fires by itself while your finger (or the mouse button) is down; tap to start; the pause and sound buttons sit at the top right (a muted game stays muted next time). The game also pauses itself when you switch to another app or window.

**Keyboard:** arrow keys or `W` `A` `S` `D` to move (up and down too), `Space` to shoot (hold for rapid fire), `Enter` to start / restart, `P` to pause, `M` to mute, `F3` to show the FPS. On desktop also `F` for fullscreen, `1`-`5` to pick the window size (you can drag the window edges too) and `Esc` to leave fullscreen or quit.

## The game

- Waves of invaders sweep sideways and step toward you. Shoot them before they reach you. They shoot back.
- A tall **portrait** screen, like a phone shooter. Your ship moves in all directions: left and right, and up
  to the middle of the screen.
- Seven alien types: **Scarabs** (100), striped **Wasps** (150, dive a lot), crystal **Drones** (150), **Shear Mites**
  (180, little crabs), **Reapers** (200, shoot more often), big **Moths** (250) and **Commanders** (500). The aliens get tougher every level: 3 hits in level 1, up to 12 in level 10, and they darken as you damage them. Every few seconds
  one alien **dives** at you in an S-curve.
- Every wave **flies in** along curved paths, in changing **formations** (block, arrow, diamond, pincer, ...).
- **3 lives** (up to 5), and your **high score** is saved (on desktop and in the browser).
- Shot aliens sometimes drop a glowing **orb**: purple **weapon** orbs make your weapon stronger (5 levels), green
  and blue orbs switch to the **spread shot** or the **lightning laser** (1 to 3 blue bolts that chain to nearby aliens), plus a **shield** that absorbs one hit,
  **rapid fire**, or rarely an **extra life**.
- Your ship changes its look as you go: from level 5 you fly the **Claw**, a copper ship with claw-shaped wings, and
  in levels 8 to 10 the **Horn**, with two big curved horn-wings, in green, purple and red.
- **Combos** multiply your points (up to x4) for quick kills in a row, and a fast wave without losing a ship
  earns a **wave-clear bonus**. Every hit counts for points, and every cleared level pays a **level-clear bonus**. Big hits and explosions shake the screen.
- A glowing **bomb** flies in every few seconds. Shoot it for **+500** points, or dodge it. Every level
  has its own bomb: from a calm yellow ball in level 1 to crackling, lightning-wrapped ones in the last levels.
- **Boss fights** at the end of levels 3, 5 and 7: the **Hive Queen**, the **War Titan** and the
  **Void Emperor**, each with a health bar, its own attack patterns and a core that opens for triple damage.
  Then the **crab family** ends levels 8, 9 and 10: the green **Shear King**, the purple **Shear Emperor** and the
  red **Crimson Tyrant**, armoured crabs with giant snapping **scissor claws**, each bigger than the last.
- **10 levels of 4 waves each.** Every level adds something new (Drones, divers, Reapers, Commanders, Moths,
  Shear Mites, new ships), gets faster and has its own colours: the aliens, the bombs and the stars change with
  every level. After level 10: a bonus and endless bonus waves.
- Particle explosions, a streaming starfield over a drifting **nebula** in each level's colours, glowing
  projectiles (both are custom GLSL shaders), sound effects and
  chiptune **music**: a calm menu track, a level track and a boss track that fade into each other.

## Building (for development)

This is how the game is built while working on it. (Publishing your own builds is not allowed, see [License](#license).)
You need a C++17 compiler, CMake and Git. On Linux also a few development libraries (see [`docs/SETUP.md`](docs/SETUP.md)).

```bash
cmake -B build
cmake --build build
./build/space-invader-3d
```

The first run downloads raylib automatically. The web version needs Emscripten, see [`docs/WEB.md`](docs/WEB.md).
Add `-DSPACE_INVADER_DEMO=ON` to the first command to build the demo (levels 1-3), as the web version and the
downloads are built.

## Tech stack

| Part | Choice |
|---|---|
| Language | C++17 |
| Graphics / audio / input | raylib 5.5 (OpenGL 3.3 on desktop, WebGL 1 in the browser) |
| Build | CMake |
| Web build | Emscripten (WebAssembly) |
| CI / hosting | GitHub Actions, GitHub Pages and Releases |
| Android | NDK, NativeActivity (no Java/Kotlin code), built with Android Studio |

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
- Music (all CC0, from [OpenGameArt](https://opengameart.org/)): "8-bit Epic Space Shooter Music" by **HydroGene**,
  "Boss Fight" from Chiptune Adventures by **Juhani Junkala**, and "Observing The Star" by **yd**.
  Details in [`assets/audio/README.md`](assets/audio/README.md).

## License

**All rights reserved.** You may read the code and learn from it, and play the game through the official links,
but you may not copy, modify, publish or sell it, or release your own builds (for example on an app store),
without permission. See [`LICENSE`](LICENSE). Third-party parts (raylib, the CC0 music) keep their own licences.
