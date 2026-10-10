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
- [ ] 3D particle explosions that fade
- [ ] Starfield streaming past the camera
- [ ] Glow on projectiles; our first custom shader
- [ ] Learn: shaders, blending, how the GPU draws a frame

## M5 - Full game loop
- [ ] Bombs (+500, big explosion)
- [ ] 3 levels with the 2D difficulty rules
- [ ] Menu, HUD, Game Over, restart
- [ ] All sounds
- [ ] Learn: state machines, audio

## M6 - Ship it
- [ ] Windows + Linux builds from GitHub Actions
- [ ] Web build on GitHub Pages
- [ ] Trim large assets for web, README with play link and screenshots

## M7 - Android (later)
- [ ] Android NDK build
- [ ] Touch controls
- [ ] Performance pass for phones

## Ideas backlog
Shields, power-ups, boss wave, high-score table, bloom post-processing, free 2-axis movement, gamepad support.
