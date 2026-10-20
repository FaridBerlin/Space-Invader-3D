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
- [x] Music: menu, level and boss tracks with cross-fades
- [x] Nebula background that changes per level (our second shader, rendered at 1/16 of the pixels)
- [ ] Balance the level numbers after real play tests

## M8 - Android
- [x] Touch controls (done for the web version)
- [x] Android NDK build (Android Studio, NativeActivity, no Kotlin code), see `android/`
- [x] App icon and Play Store icon
- [ ] Performance pass for phones

## M9 - A mobile arcade shooter
Ideas from playing popular phone shooters (Galaxy Invaders and others). We keep our own
look and designs; we take over what makes those games good to play on a phone.
Each step is its own branch and ends with something you can play.

1. **Portrait screen and a higher camera** (all platforms)
   - [ ] Tall picture: phone in portrait, a tall window on the desktop, a tall canvas on the web
   - [ ] The camera looks down on the field from higher up; it fits itself to any screen shape
   - [ ] HUD, menus and banners laid out for a tall screen
2. **Move up and down too**
   - [ ] The ship follows the finger in both directions (relative drag, as today)
   - [ ] Keyboard: up / down arrows and W / S
   - [ ] The ship may use about the bottom third of the field
3. **Clear power-up orbs**
   - [ ] Glowing orbs with three arrows inside, one colour per kind
   - [ ] Weapon levels 1, 2, 3, ... collected with orbs (instead of fixed per level); a hit costs one
4. **New weapons**
   - [ ] Lightning laser: a beam with jagged electric branches
   - [ ] Spread shot: a fan of shots that widens with the weapon level
   - [ ] Special orbs switch the weapon type
5. **More aliens and fly-in attacks**
   - [ ] New low-poly aliens (insect-like, winged, ...)
   - [ ] Aliens fly in on curved paths before they take their places (like Galaga)
   - [ ] Different formations per level
6. **Google Play release**
   - [ ] Signed app bundle (.aab), store page, screenshots of the new portrait game
   - [ ] Closed test (12 testers, 14 days), then the paid release

## Ideas backlog
High-score table with names, bloom post-processing, gamepad support.
