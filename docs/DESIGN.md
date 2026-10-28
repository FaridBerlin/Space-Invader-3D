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

### Screen and camera
The game is **tall (portrait)**, like most phone shooters: the phone is held upright, the desktop window is
9:16 (sizes 450x800 up to 900x1600), the web canvas is 720x1280. The field is 19 units wide (X from -9.5 to
+9.5); waves of at most 6 columns fill it.

The camera is above and behind the ship and looks down at 50 degrees: the field stands upright on the screen
(aliens at the top, the ship at the bottom), and the perspective keeps a real 3D look. (66 degrees looked too
flat, like a 2D game.) It is not placed
by hand: `Game::FitCamera` moves it as close as possible while the whole field width and the depth from just
behind the ship (Z = +1.8) to the back rows (Z = -36) still fit. So it works on every screen shape.

On a wider screen (a landscape monitor in fullscreen, a tablet) the game uses a tall **play column** in the
middle (at most 0.62 x its height wide); the sides show only space. The HUD is laid out inside the column and
scaled with it (`display::Column`, `display::Px`, design size 540 x 960).

Aliens are drawn 1.35x bigger than designed and turn their face 30 degrees up toward the camera; the bosses
lean back 10 degrees for the same reason.

### Player
Low-poly ship. It moves left/right across the field and up/down in the lower half of the screen: down until its
tail touches the bottom edge, up to the middle of the screen (`Game::FitCamera` works out both limits from the
camera, so they fit every screen shape). In a boss fight it only goes up to Z = -5, so it cannot fly into the boss. It banks (rolls) when moving sideways, and dips its nose a little when flying forward.
Keys: arrows or W A S D. Touch: drag anywhere, the ship follows the finger in both directions.
Flying into an alien (a diver, or one still in the formation) is a crash: the alien is destroyed, the ship too
(unless the shield is up).

- **3 lives** (at most 5). After a hit the next ship appears in the middle and blinks for 2.5 s, unhittable.
- **Three designs:** the first ship (levels 1-4), the **Claw** (levels 5-7), a copper ship with claw-shaped wings
  (collision sphere 1.5 instead of 1.1, because it is wider), and the **Horn** (levels 8-10), an arrowhead with two
  big curved horn-wings and a glowing canopy, in **green** (level 8), **purple** (9) and **red** (10); its rockets
  have its glow colour. It was drawn on the design canvas first and built from that picture's points.
- **Ship looks by level:** three colour tiers (levels 1-3, 4-5, 6-10); the ship grows a little each time.
- **Three weapons, each with levels 1-5** (not tied to the game level). Every game starts with the blaster at 1.
  - **Blaster:** 1, 2 and 3 shots side by side, then 5 (two angled 9 degrees) and 7 (a fan up to 18 degrees).
  - **Spread:** a fan of 3 to 7 shots, from +-12 to +-32 degrees: best against big waves.
  - **Laser:** blue-white lightning (a white-hot core in a blue sleeve, thin violet strands winding around it
    and small forks, re-drawn 25 times a second). Level 1 fires 1 bolt, level 2 two, level 3 three fanned out a
    little; levels 4 and 5 keep three bolts but make them thicker and faster. Each bolt stops at the first
    alien, bomb or boss in its way and hits it every 0.12 s (1 bolt) to 0.24 s (3 bolts at level 3), then
    0.20 s and 0.16 s. **Chain lightning** (idea from Chicken Invaders' "Lightning Fryer"): when a bolt kills
    an alien, the spark jumps on to the nearest alien within 4.5 units and hits it, 1 jump at levels 1-2,
    2 at levels 3-4 and 3 at level 5. Best against crowded formations and bosses.
    (`Laser.cpp` draws it; `Game::UpdateLaser` decides the hits.)
  - A **purple** orb raises the level of the weapon you have; a **green** (spread) or **blue** (laser) orb
    switches to that weapon at the same level, or raises its level if you already have it. At level 5 an orb
    gives 500 points instead. Losing a ship costs one level; you keep the weapon.
  - The HUD shows a "POWER" meter and the weapon's name in its colour.

### Invaders
A wave (grid in X and Z) that sweeps sideways, reverses at the field edge and steps **toward the player**
(replaces "drops 30 px"). Random columns (4-6) and rows. Reaching the player's line ends the game.

**Fly-in:** a new wave flies in from outside the screen, alien after alien in streams, along curved paths
(cubic Bezier curves) into its places, like in Galaga. Three entries take turns: a **side swoop** (from both top
corners, down through the middle and up into place), **rain** (down from above each column, past its place and
back up) and a **cross loop** (whole rows low in from one side, across the field and up in a wide arc). The
formation only starts moving when everybody has arrived. Aliens on the way in do not shoot or ram, but can be
shot; the wave's clock for the speed bonus starts when they have arrived.

**Formations:** besides the full block, a wave can be an **arrow** (pointing at the player), a **diamond**, a
**pincer** (a gap in the middle), a **checkerboard** or a **chevron**. The shape changes from wave to wave; the
very first wave of a game is always the block.

Rows, front to back: Scarabs, Wasps, Reapers, Moths, and Drones with the Commanders in the back row (only the
types the level has; extra rows are Scarabs).

| Alien | Points | From level | What it does |
|---|---|---|---|
| Scarab | 100 | 1 | the front rows. Like the Wasp, Drone, Reaper and Mite it takes **3 hits** |
| Wasp | 150 | 2 | a striped insect with buzzing wings; dives 3x as often as the others |
| Drone | 150 | 2 | the back row, a spinning crystal |
| Reaper | 200 | 3 | the middle rows, shoots more often than the others |
| Commander | 500 | 4 | back row, needs 8 hits, always drops a power-up |
| Moth | 250 | 6 | a big insect with glowing eye-spots on its wings; 6 hits, drops an orb 30% of the time |
| Shear Mite | 180 | 8 | a small armoured crab with snapping scissor claws (the crab bosses' cousin); the front row, shoots a lot |

**Divers:** every few seconds one alien leaves the wave and swoops at the player in an S-curve. If it hits the
ship, both are destroyed. If it misses, it flies back into its place.

**Power-ups:** a destroyed alien drops a glowing **orb** with a 10% chance (commanders always; a boss drops two,
one of them a weapon orb). Each kind has its own colour and symbol, drawn in 2D over the ball so it stays sharp:
**weapon** (purple, three arrows gliding upward, 33%), **spread** (green, a fan, 10%), **laser** (blue, a
lightning bolt, 10%), **shield** (cyan, a shield, absorbs one hit, 20 s, 20%), **rapid fire** (yellow, "x2", twice
as fast for 8 s, 17%), **extra life** (pink, "+1", 10%).

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

**Aliens must stand out** from the dark background, also on a dim phone or tablet screen (play tests showed
the first, dark alien colours were hard to see on Android). So: light body colours in every level, more light
on the shadow side of the aliens (`kAlienShadowLight` 0.6 instead of 0.35), bigger glowing eyes and cores, and
a soft halo in the level's glow colour behind every alien, like the power-up orbs (`kAlienHaloSize`,
`kAlienHaloAlpha`). The bosses use the same light body colours.

### UI
Drawn with raylib's 2D functions on top of the 3D scene, scaled with the window height:

- **HUD:** score and lives (top left), high score (top middle), wave and level (top right), sound and pause buttons.
  The key help shows for the first 6 seconds of a game and on the pause screen; F3 shows the FPS.
- **Banners:** "LEVEL n" with a short intro text, "WAVE n / 3", and power-up names.
- **Screens:** menu, pause (also automatic when the window or app goes to the background), game over with restart.
- **Touch / mouse:** drag anywhere to move, the ship fires by itself while the finger is down.

## 4. Difficulty and levels

**10 levels of 4 waves each.** A wave is cleared when all its aliens are destroyed; after the fourth wave the next
level starts. Play tests showed the levels were too short (an alien died from one hit, a level took about a
minute), so the aliens now take 3 hits (Moths 6, Commanders 8) and every level has 4 waves instead of 3: a
level lasts about two to three times as long, and the aliens stay on screen long enough to be seen. Levels 3, 5, 7, 8, 9 and 10 end with a **boss** (see below). After level 10: +20,000 points and endless
bonus waves at level 10 strength. All numbers live in
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
| 8 | Shear Mites, the Horn | 4-5 | 7.8 | 1.15 s | 2.4 s / 3 | gold | Horn, green |
| 9 | | 4-5 | 8.2 | 1.05 s | 2.2 s / 3 | cyan | Horn, purple |
| 10 | | 5 | 8.6 | 0.95 s | 2.0 s / 4 | hot pink | Horn, red |

(The "Ship" column shows the look; how many rockets it fires comes from the weapon level, see "Player".)

Every level also has its own **colours**: the aliens (body and glow), the bombs and the tint of the stars.
Bombs get bigger and faster with every level (radius 0.9 to 1.5).

### Bosses

After the third wave of levels 3, 5, 7, 8, 9 and 10 a huge alien flies in, with its name and a health bar at the top of the
screen. It slides from side to side and attacks in a fixed order. Every hit on its armour costs it 1 hit point; its
**core** opens every 6 seconds for 2.5 seconds (it glows, "CORE OPEN!"), and a hit there costs 3. Below half health it
gets **angry**: its eyes and health bar turn red, it moves faster, attacks more often and fires more shots. When it is
destroyed it explodes in a chain of blasts and drops two power-ups. The normal bombs pause during a boss fight.

| Boss | Level | Hit points | Points | Attacks |
|---|---|---|---|---|
| Hive Queen | 3 | 100 | 5,000 | fan of 5 shots, aimed burst of 3, a bomb |
| War Titan | 5 | 200 | 8,000 | adds horns, 2 bombs and a rain of shots from its wing guns |
| Void Emperor | 7 | 300 | 12,000 | adds a crown, a ring of blades, 3 bombs and a sweeping spiral of shots |
| Shear King | 8 | 380 | 15,000 | the first **crab**: two giant **scissor claws** fire streams that cross like closing blades (the claws snap shut), and an **eye ring**: every eye fires outward |
| Shear Emperor | 9 | 470 | 18,000 | bigger, longer blades, spikes on shell, arms and legs; adds claw rain and the spiral |
| Crimson Tyrant | 10 | 580 | 25,000 | the biggest, with glowing veins; every attack, faster |

The crabs hover a little further back (Z = -20) because their claws reach far forward. Their weak spot is the big
eye in front, behind four armour petals; the many small eyes fire the eye ring. They take their colours from their
level: gunmetal and green, purple and crimson, red and gold.

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
