# Design

How the 3D game plays today is in sections 3 and 4. Section 1 keeps the numbers of the original 2D game,
which were the starting point.

## 1. What the 2D game did (the starting point)

Taken from `space-invader/index.js`. Values are per frame at 60 fps; the 3D game uses **per-second** values
(multiply by 60) so speed does not depend on the monitor's refresh rate. They are starting points to tune, not law.

| Thing | 2D value | Per second |
|---|---|---|
| Player move speed | 9 px/frame, ship tilts 0.3 rad while moving | 540 px/s |
| Player rocket | 10 px/frame upward | 600 px/s |
| Fire rate (holding Space) | one rocket every 200 ms | 5 shots/s |
| Invader grid size | 5-14 columns x 2-6 rows, 30 px spacing | |
| Grid sideways speed | 2 px/frame; at an edge it reverses and drops 30 px | 120 px/s |
| New grid spawn | random every 500-1000 frames | ~8-17 s |
| Invader shot | one random invader fires every 500 frames | ~8.3 s |
| Invader shot speed | 1 / 2 / 3 px/frame for level 1 / 2 / 3 | 60 / 120 / 180 px/s |
| Bomb spawn | every 300 frames, random x, falls from the top | every 5 s |
| Bomb speed / radius / colour | 3 / 6 / 9 px/frame, radius 30 / 40 / 50, yellow / orange / red | 180 / 360 / 540 px/s |
| Score | invader +100, bomb +500 | |
| Level up | level 2 at 10,000 points, level 3 at 20,000 | |
| Lose condition | player hit by an invader shot or a bomb | |
| Player death | big red explosion, then Game Over after about 2 s | |

Assets from the 2D game: invader sprite, ship sprite, 7 sounds (see section 5).

## 2. Lessons from the 2D code (do better in 3D)

- Use **delta time**, not frame counts.
- Mark objects dead and remove them after the update loop. No deferred `splice` inside callbacks.
- One consistent asset path scheme.
- No logging inside the main loop.
- Keep game rules (data) separate from drawing code.

## 3. 3D design

### Coordinate system
raylib is right-handed, Y up. Our play field:

- **X**: left / right (player movement)
- **Y**: up (small vertical offset only, for look)
- **-Z**: forward, into the screen. Invaders start far away and advance toward the player.

### Camera
Third-person, behind and slightly above the ship, looking down the field toward the invader wall. A slight
smooth follow in X gives a feeling of speed. A tilted top-down camera can be added later as an option.

### Player
Low-poly ship, moves left/right within field bounds, banks (rolls) when moving like the 2D ship's rotation.
Movement only along X; free 2-axis movement is a possible later mode.

- **3 lives** (at most 5). After a hit the next ship appears in the middle and blinks for 2.5 s, unhittable.
- **Two designs:** the first ship (levels 1-4) and the **Claw** (levels 5-7), a copper ship with claw-shaped
  wings. Its collision sphere is bigger (1.5 instead of 1.1), because it is wider.
- **Upgrades by level:** 1 rocket (levels 1-3), 2 rockets (4-5), 3 rockets (6-7). The ship grows a little each time.

### Invaders
A wave (grid in X and Z) that sweeps sideways, reverses at the field edge and steps **toward the player**
(replaces "drops 30 px"). Random columns and rows. Reaching the player's line ends the game.

| Alien | Points | What it does |
|---|---|---|
| Scarab | 100 | the front rows |
| Reaper | 200 | the middle rows, shoots more often than the others |
| Drone | 150 | the back row, a spinning crystal |
| Commander | 500 | back row, needs 3 hits, always drops a power-up |

**Divers:** every few seconds one alien leaves the wave and swoops at the player in an S-curve. If it hits the
ship, both are destroyed. If it misses, it flies back into its place.

**Power-ups:** a destroyed alien drops one with an 8% chance: a **shield** (absorbs one hit, 20 s),
**rapid fire** (twice as fast, 8 s) or, rarely, an **extra life**.

### Projectiles and bombs
- Player rockets: fly along -Z with an emissive trail.
- Invader shots: fly along +Z toward the player.
- Bombs: glowing spheres that fly in from far away toward the player, one every 5 s; shoot them for +500, they
  explode on contact with the ship. Every level has its own bomb (see section 4).
- Collisions: simple spheres/boxes (raylib `CheckCollisionSpheres`, `CheckCollisionBoxes`). No physics engine needed.

### Visual style
Neon low-poly on a dark space background: unlit or simply lit colourful meshes, an additive glow on projectiles
and explosions, a starfield of points streaming past the camera (replaces the scrolling white dots), 3D particle
bursts that fade out (replaces the 2D fading circles). Bloom/glow via a post-process shader is a later learning step.

### UI
Drawn with raylib's 2D functions on top of the 3D scene, scaled with the window height:

- **HUD:** score and lives (top left), high score (top middle), wave and level (top right), sound and pause buttons.
  The key help shows for the first 6 seconds of a game and on the pause screen; F3 shows the FPS.
- **Banners:** "LEVEL n" with a short intro text, "WAVE n / 3", and power-up names.
- **Screens:** menu, pause (also automatic when the window or app goes to the background), game over with restart.
- **Touch / mouse:** drag anywhere to move, the ship fires by itself while the finger is down.

## 4. Difficulty and levels

**7 levels of 3 waves each.** A wave is cleared when all its aliens are destroyed; after the third wave the next
level starts. Levels 3, 5 and 7 end with a **boss** (see below). After level 7: +10,000 points and endless bonus waves at level 7 strength. All numbers live in
the "Levels" section of `src/Config.h`, one table entry per level.

| Level | New | Rows | Wave speed | Shot every | Divers (every / at once) | Bomb | Ship |
|---|---|---|---|---|---|---|---|
| 1 | Scarabs | 2-3 | 4.0 | 2.4 s | none | yellow, calm | 1 rocket |
| 2 | Drones, divers | 2-3 | 4.5 | 2.2 s | 5.0 s / 1 | green, pulsing cage | 1 rocket |
| 3 | Reapers | 3-4 | 5.0 | 2.0 s | 4.5 s / 1 | cyan, energy ring | 1 rocket |
| 4 | Commanders | 3-4 | 5.5 | 1.8 s | 4.0 s / 1 | violet, spikes | 2 rockets |
| 5 | the Claw | 3-5 | 6.0 | 1.6 s | 3.5 s / 2 | orange, sparks | Claw, 2 rockets |
| 6 | | 4-5 | 6.8 | 1.4 s | 3.0 s / 2 | red, flicker | Claw, 3 rockets |
| 7 | | 4-5 | 7.5 | 1.2 s | 2.5 s / 3 | crimson, cracked core and lightning | Claw, 3 rockets |

Every level also has its own **colours**: the aliens (body and glow), the bombs and the tint of the stars.
Bombs get bigger and faster with every level (radius 0.9 to 1.5).

### Bosses

After the third wave of levels 3, 5 and 7 a huge alien flies in, with its name and a health bar at the top of the
screen. It slides from side to side and attacks in a fixed order. Every hit on its armour costs it 1 hit point; its
**core** opens every 6 seconds for 2.5 seconds (it glows, "CORE OPEN!"), and a hit there costs 3. Below half health it
gets **angry**: its eyes and health bar turn red, it moves faster, attacks more often and fires more shots. When it is
destroyed it explodes in a chain of blasts and drops two power-ups. The normal bombs pause during a boss fight.

| Boss | Level | Hit points | Points | Attacks |
|---|---|---|---|---|
| Hive Queen | 3 | 100 | 5,000 | fan of 5 shots, aimed burst of 3, a bomb |
| War Titan | 5 | 200 | 8,000 | adds horns, 2 bombs and a rain of shots from its wing guns |
| Void Emperor | 7 | 300 | 12,000 | adds a crown, a ring of blades, 3 bombs and a sweeping spiral of shots |

All numbers are in the "Bosses" section of `src/Config.h`; the model and the attacks are in `src/Boss.cpp`.

### Game feel

- **Screen shake** when the ship is destroyed, the shield breaks, a bomb is shot down, a commander dies, and
  strongest when a boss explodes. Big moments add "trauma" that fades within a second; the camera moves by trauma
  squared, so small bumps stay subtle.
- **Combo:** kills less than 1.5 s apart form a combo. The points multiplier is x2 from the 6th kill, x3 from the
  11th, at most x4. "COMBO x2" and a small timer bar show under the high score. Losing a ship ends the combo.
- **Wave-clear bonus:** a wave has a "par time" of 1 second per alien. Every second faster gives 25 points, and a
  wave without losing a ship gives 1,000 more ("SPEED BONUS +300  NO HIT +1000").
- **Readability:** the Claw fires white-gold rockets (alien shots are yellow), and a bomb's glow shrinks when it is
  close to the ship, so it never hides it.

All numbers are in the "Game feel" section of `src/Config.h`.

- **Nebula:** soft colour clouds behind the stars, calculated by a shader from layered noise ("fbm" with domain
  warping, see `src/Nebula.cpp`). Two colours per level (`kLevelNebulaA/B`), fading when the level changes. They are
  brighter at the top and calmer at the bottom, where the ship flies. For speed they are rendered at a quarter of
  the window size and stretched smoothly over the screen.

## 5. Audio mapping

| 2D file | Use in 3D |
|---|---|
| `laser.wav` | player shot |
| `laser2.wav` | invader shot |
| `laser3.wav` (1.2 MB) | player death. Compress/trim: it is large for a web build |
| `laser4.wav` | bomb explosion |
| `explosion.wav` | invader destroyed |
| `mechanical_explosion.wav`, `xplode666.wav` | unused in 2D; candidates for boss or power-ups |

Browsers only start audio after a user click; the Start button covers this.

**Music** (CC0, see `assets/audio/README.md`): `menu.ogg` on the menu and game-over screens, `level.ogg` while
playing, `boss.ogg` during a boss fight. Music is *streamed* (decoded a little at a time) instead of loaded whole;
`Audio::Update` feeds the streams every frame and fades between tracks in 1.2 s. The level track continues where it
paused after a boss; menu and boss tracks start from the beginning. The pause screen turns the music down to 35%,
and the sound button mutes music and effects together. Numbers: "Music" in `src/Config.h`.

## 6. Open questions

- Final 3D models: make in Blender, use free CC0 packs, or generate from primitives in code first?
- ~~Mobile controls~~: decided, touch-drag with automatic fire.
- Name and license of the project.
