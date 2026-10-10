# Design

## 1. What the 2D game does (reference)

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
Movement only along X for version 1; free 2-axis movement is a possible later mode.

### Invaders
A wall (grid in X and Y) that sweeps sideways, reverses at the field edge and steps **toward the player**
(replaces "drops 30 px"). Random columns and rows as in 2D. Reaching the player's line can also end the game
(new rule, optional).

### Projectiles and bombs
- Player rockets: fly along -Z with an emissive trail.
- Invader shots: fly along +Z toward the player.
- Bombs: glowing spheres that fall from above and toward the player; shoot them for +500, they explode on contact with the ship.
- Collisions: simple spheres/boxes (raylib `CheckCollisionSpheres`, `CheckCollisionBoxes`). No physics engine needed.

### Visual style
Neon low-poly on a dark space background: unlit or simply lit colourful meshes, an additive glow on projectiles
and explosions, a starfield of points streaming past the camera (replaces the scrolling white dots), 3D particle
bursts that fade out (replaces the 2D fading circles). Bloom/glow via a post-process shader is a later learning step.

### UI
Score (top left), level banner, Start screen, Game Over screen with restart. Drawn with raylib's 2D functions on top of the 3D scene.

## 4. Difficulty and levels

Same 3 levels and thresholds as the 2D game. Level raises invader shot speed, bomb speed, bomb size and bomb colour.
Ideas for later: boss wave, shields, power-ups, high-score table saved to disk (and local storage on web).

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

## 6. Open questions

- Final 3D models: make in Blender, use free CC0 packs, or generate from primitives in code first?
- Mobile controls: on-screen buttons vs tilt vs touch-drag.
- Name and license of the project.
