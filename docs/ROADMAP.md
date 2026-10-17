# Roadmap

Each milestone ends with something you can run, and a short note on what it teaches.

## M0 - Project setup
- [x] GitHub repo created, local folder connected (see SETUP.md)
- [x] Toolchain installed (compiler, CMake, Git)
- [x] CMake project that opens a window with raylib
- [ ] Learn: how a C++ project is built, what a game loop is

## M1 - 3D scene and camera
- [x] Camera behind the field, a ground grid for reference
- [x] Delta-time timer, FPS display
- [ ] Learn: 3D coordinates, vectors, camera, projection

## M2 - Player ship
- [x] Ship mesh (built from primitives first), left/right movement with bounds, banking
- [x] `Input` layer
- [ ] Learn: transforms, rotation, classes in C++

## M3 - Shooting and invaders
- [x] Rockets with fire-rate limit
- [x] Invader grid: sweep, reverse at the edge, step forward
- [x] Collisions rocket vs invader, score
- [x] Invaders shoot back, player can die (Game Over screen with restart)
- [ ] Learn: `std::vector`, collision tests, object lifetime

## M4 - Effects
- [x] 3D particle explosions that fade
- [x] Starfield streaming past the camera
- [x] Glow on projectiles; our first custom shader
- [ ] Learn: shaders, blending, how the GPU draws a frame

## M5 - Full game loop
- [x] Bombs (+500, big explosion)
- [x] Levels with rising difficulty (first 3 levels like the 2D game, now 7, see M6b)
- [x] Menu, HUD (level + banner), Game Over, restart
- [x] All sounds
- [ ] Learn: state machines, audio

## M6 - Ship it
- [x] Windows + Linux builds from GitHub Actions
- [x] Web build (Emscripten), see docs/WEB.md
- [x] Web build published on GitHub Pages (workflow ready; switch on Pages, see SETUP.md)
- [x] Trim large assets for web, README with play link and screenshots

## M6b - A fuller game
- [x] Glass floor and stars all around, resizable window, window size presets, fullscreen
- [x] New low-poly ship with three upgrade tiers, and the Claw from level 5
- [x] Four alien types (Scarab, Reaper, Drone, Commander), the Commander as an optional `.glb` model
- [x] Divers, power-ups (shield, rapid fire, extra life), 3 lives, high score saved
- [x] Touch and mouse controls, pause button, auto-pause in the background, sound button (remembered)
- [x] 7 levels of 3 waves, each with its own alien colours, bomb design and star colour
- [x] Key help only at the start and on the pause screen, FPS on F3
- [ ] Learn: data tables per level, `static_assert`, procedural effects without textures

## M7 - Bosses and polish
- [x] A boss after levels 3, 5 and 7 (Hive Queen, War Titan, Void Emperor)
- [x] Screen shake on hits, combo bonus, wave-clear bonus
- [ ] Nebula background that changes per level
- [ ] Balance the level numbers after real play tests

## M8 - Android (later)
- [x] Touch controls (done for the web version)
- [ ] Android NDK build
- [ ] Performance pass for phones

## Ideas backlog
High-score table with names, bloom post-processing, free 2-axis movement, gamepad support.
