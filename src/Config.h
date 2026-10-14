// Config.h - every tuning number of the game in one place.
//
// A ".h" file is a "header": it is copied into every .cpp file that says
// #include "Config.h". Headers are how C++ files share declarations.

#pragma once   // "include me only once", even if several files ask for me

#include <iterator>   // std::size

#include "raylib.h"

// A namespace is a named box. Writing config::kFieldHalfWidth makes it obvious
// where a value comes from and avoids name clashes with other code.
namespace config {

// ---- Window ---------------------------------------------------------------
// The window opens at a size chosen for your monitor (see Display.cpp), and you can
// resize it. This size is only the starting size before that, and the size of the
// web version's canvas.
constexpr int kScreenWidth  = 1280;
constexpr int kScreenHeight = 720;

// Window sizes you can pick with the keys 1 .. 5, small to big (all 16:9).
constexpr int kWindowPresetCount = 5;
constexpr int kWindowPresets[kWindowPresetCount][2] = {
    {1280,  720},
    {1600,  900},
    {1920, 1080},
    {2560, 1440},
    {3200, 1800},
};
constexpr float kWindowStartShare = 0.80f;   // at start: the biggest size using <= 80% of the monitor
constexpr float kWindowMaxShare   = 0.97f;   // keys 1-5: sizes up to 97% of the monitor are allowed

// The 2D text is designed for a window 720 pixels high. At other heights it is
// scaled by (height / 720), see display::Px().
constexpr float kUiReferenceHeight = 720.0f;

// Longest time step we allow, in seconds. If the game stalls (window dragged,
// slow computer), dt can become huge and fast objects would jump straight
// through their targets. Capping dt keeps the simulation stable.
constexpr float kMaxFrameTime = 0.05f;

// ---- Play field -------------------------------------------------------------
// Coordinates (raylib is right-handed, Y is up):
//   X: left (-) / right (+)       the player moves along X
//   Y: down (-) / up (+)
//   Z: toward the camera (+) / away into the screen (-)   invaders come from -Z
// One unit is roughly "one metre". The player stands at Z = 0.
constexpr float kFieldHalfWidth = 12.0f;   // field spans X from -12 to +12
constexpr float kFieldLength    = 50.0f;   // field spans Z from 0 down to -50

// ---- Camera -------------------------------------------------------------------
// The camera sits behind and above the player, looking down the field.
constexpr Vector3 kCameraPosition{0.0f, 8.0f, 14.0f};
constexpr Vector3 kCameraTarget{0.0f, 0.0f, -12.0f};
constexpr float   kCameraFovDegrees = 60.0f;   // vertical field of view

// ---- Ground grid ------------------------------------------------------------------------
// The floor grid covers only the play field (the area inside the blue border), see
// DrawGround() in Game.cpp. Outside the field you see open space. Lines must stay IN FRONT
// of the camera (it sits at Z = 14): lines that cross the camera's position are clipped by
// the GPU, and some browsers get that wrong. The field ends at Z = 0, so we are safe.
//
// Colours are {red, green, blue, alpha}. The LAST number is the opacity: 255 = solid,
// 0 = invisible. Lower it to make a line fainter.
constexpr Color kGridLineColor   {130, 130, 155, 0};    // the thin grid lines
constexpr Color kGridCenterColor {255, 255, 255, 0};   // the white line down the middle
constexpr Color kFieldBorderColor{135, 206, 235, 0};   // the light-blue border (sky blue)

// ---- Glass floor --------------------------------------------------------------------------
// A see-through tinted pane over the play field. Stars below it shine through.
constexpr Color kGlassColor{40, 110, 190, 0};   // blue tint, alpha 55 of 255 (see-through)

// ---- Player ship --------------------------------------------------------------
constexpr float kPlayerSpeed          = 14.0f;  // top sideways speed, units per second
constexpr float kPlayerResponsiveness = 12.0f;  // higher = reaches top speed / stops faster
constexpr float kPlayerMaxBankDegrees = 35.0f;  // roll angle at top speed
constexpr float kPlayerHalfWidth      = 1.5f;   // wing tip to centre, used for the wall limit
constexpr float kPlayerHeight         = 0.6f;   // ship centre above the ground
constexpr float kPlayerHitRadius      = 1.1f;   // collision sphere of the ship (a bit smaller than the wings)

// ---- Touch / mouse steering -----------------------------------------------------------
// Drag anywhere: the ship moves as far (in screen pixels) as the finger does, and
// fires by itself while the finger is down.
constexpr float kTouchDragGain    = 1.3f;    // 1.0 = the ship follows the finger 1:1 on screen
constexpr float kTouchFollowRate  = 14.0f;   // how quickly the ship catches up with the finger
constexpr float kTouchSpeedFactor = 1.8f;    // top speed with touch, times kPlayerSpeed

// ---- Rockets ------------------------------------------------------------------
constexpr float kRocketSpeed        = 35.0f;   // units per second, flying toward -Z
constexpr float kRocketFireInterval = 0.2f;    // seconds between shots (same as the 2D game)
constexpr float kRocketRadius       = 0.3f;    // size of the rocket's collision sphere

// ---- Invaders -----------------------------------------------------------------
constexpr int   kInvaderMinColumns = 5;        // a wave gets a random size, like the 2D game
constexpr int   kInvaderMaxColumns = 10;
constexpr float kInvaderSpacingX   = 2.2f;     // distance between neighbours sideways
constexpr float kInvaderSpacingZ   = 3.2f;     // distance between rows in depth (room to see the back rows)
constexpr float kInvaderHalfWidth  = 0.7f;     // used to stop at the field edge
constexpr float kInvaderRadius     = 0.9f;     // size of the collision sphere
constexpr float kInvaderHeight     = 0.9f;     // centre above the ground
constexpr float kInvaderStepZ      = 1.0f;     // how far the wave steps toward you at an edge
constexpr float kInvaderStartZ     = -30.0f;   // Z of the front row when a wave spawns
constexpr float kInvaderReachZ     = -2.5f;    // an invader this close to the ship = game over

// ---- Alien types -----------------------------------------------------------------
// Most aliens are Scarabs (one hit). Commanders sit in the back row: bigger,
// three hits, more points. A commander is drawn with a 3D model file when
// assets/models/commander.glb exists, otherwise with a built-in look.
constexpr int   kScorePerReaper      = 200;
constexpr int   kScorePerDrone       = 150;
constexpr int   kScorePerCommander   = 500;
constexpr float kReaperScale         = 0.95f;   // drawing size of each type (1.0 = as designed)
constexpr float kDroneScale          = 1.05f;
constexpr float kDroneRadius         = 0.75f;   // collision sphere of a Drone
constexpr int   kReaperShotWeight    = 3;       // a Reaper is 3x as likely to be the next shooter
constexpr int   kCommanderHitPoints  = 3;
constexpr int   kCommandersPerWave   = 2;       // when the wave is wide enough, else 1
constexpr float kCommanderRadius     = 1.2f;    // collision sphere (Scarab: kInvaderRadius)
constexpr float kHitFlashTime        = 0.15f;   // seconds an alien lights up after a hit
constexpr const char* kCommanderModelFile = "models/commander.glb";
constexpr float kCommanderModelScale = 2.3f;    // the model is about 1 unit big
constexpr float kCommanderHover      = 0.6f;    // drawn this much higher, to peek over the rows in front

// ---- Invader shots --------------------------------------------------------------
// The 2D game fired one shot every ~8 seconds. In 3D you can see shots coming and
// dodge them more easily, so we fire more often. Tune to taste!
constexpr float kInvaderShotRadius   = 0.25f;  // collision sphere of a shot
constexpr float kInvaderShotDespawnZ = 6.0f;   // shots that fly past the ship are removed here

// ---- Levels --------------------------------------------------------------------------
// Seven levels of three waves each. A level ends when its third wave is destroyed;
// after level 7 the game goes on with endless bonus waves. Every level adds ONE new
// thing, so the player keeps discovering something:
//
//   level 1  Scarabs only, slow, no divers          (learn the controls)
//   level 2  Drones join, first divers
//   level 3  Reapers join (they shoot more), faster bombs
//   level 4  Commanders join, ship upgrade 2
//   level 5  two divers at once, red bombs
//   level 6  faster waves, ship upgrade 3
//   level 7  everything at full speed, three divers
//
// Every table below has one value per level. Arrays are indexed from 0: level 1 is
// index 0. The static_asserts at the end check that no table misses a level.
constexpr int   kLevelCount    = 7;
constexpr int   kWavesPerLevel = 3;
constexpr float kLevelBannerTime = 3.0f;   // seconds the "LEVEL 2" banner stays on screen
constexpr float kWaveBannerTime  = 1.4f;   // ... and the smaller "WAVE 2 / 3"
constexpr int   kAllLevelsBonus  = 10000;  // points for clearing level 7

constexpr const char* kLevelIntro[] = {
    "Scarabs incoming",
    "Drones join - and aliens start to dive!",
    "Reapers join: they shoot more often",
    "Commanders arrive. Ship upgraded!",
    "Two divers at once",
    "Faster waves. Ship upgraded!",
    "Final level: everything at full speed",
};

// Which alien types may appear (Scarabs always do).
constexpr bool  kLevelHasDrones[]     = {false, true,  true,  true,  true,  true,  true};
constexpr bool  kLevelHasReapers[]    = {false, false, true,  true,  true,  true,  true};
constexpr bool  kLevelHasCommanders[] = {false, false, false, true,  true,  true,  true};

// The size of a wave: rows of aliens, from ... to (picked at random).
constexpr int   kLevelMinRows[] = {2, 2, 3, 3, 3, 4, 4};
constexpr int   kLevelMaxRows[] = {3, 3, 4, 4, 5, 5, 5};

constexpr float kLevelInvaderSpeed[]  = {4.0f, 4.5f, 5.0f, 5.5f, 6.0f, 6.8f, 7.5f};   // sideways, units/s
constexpr float kLevelShotInterval[]  = {2.4f, 2.2f, 2.0f, 1.8f, 1.6f, 1.4f, 1.2f};   // seconds between shots
constexpr float kLevelShotSpeed[]     = {11.0f, 12.0f, 13.0f, 15.0f, 16.0f, 18.0f, 19.0f};
constexpr float kLevelBombSpeed[]     = {9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f};
constexpr float kLevelBombRadius[]    = {0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f};     // 2D: 30 .. 50 px
constexpr Color kLevelBombColor[] = {           // from yellow to deep red: "hotter" each level
    {253, 249,   0, 255}, {255, 220,   0, 255}, {255, 180,   0, 255}, {255, 140,  20, 255},
    {255, 100,  30, 255}, {240,  50,  40, 255}, {220,  20,  60, 255},
};
constexpr float kLevelDiveInterval[]  = {0.0f, 5.0f, 4.5f, 4.0f, 3.5f, 3.0f, 2.5f};   // 0 = no divers
constexpr int   kLevelMaxDivers[]     = {0, 1, 1, 1, 2, 2, 3};                         // at the same time
constexpr int   kLevelShipTier[]      = {0, 0, 0, 1, 1, 2, 2};                         // see "Ship upgrades"

// The colour mood of each level: the stars are tinted, from cool blue to hot red.
constexpr Color kLevelStarColor[] = {
    {200, 220, 255, 255}, {150, 200, 255, 255}, {170, 170, 255, 255}, {200, 150, 255, 255},
    {240, 140, 230, 255}, {255, 150, 140, 255}, {255, 110,  90, 255},
};

// std::size(array) counts the entries. If someone adds an 8th level to kLevelCount
// but forgets a table, the build stops right here with a clear message.
static_assert(std::size(kLevelIntro) == kLevelCount, "one intro text per level");
static_assert(std::size(kLevelHasDrones) == kLevelCount && std::size(kLevelHasReapers) == kLevelCount &&
              std::size(kLevelHasCommanders) == kLevelCount, "alien types: one value per level");
static_assert(std::size(kLevelMinRows) == kLevelCount && std::size(kLevelMaxRows) == kLevelCount,
              "wave size: one value per level");
static_assert(std::size(kLevelInvaderSpeed) == kLevelCount && std::size(kLevelShotInterval) == kLevelCount &&
              std::size(kLevelShotSpeed) == kLevelCount, "invader speeds: one value per level");
static_assert(std::size(kLevelBombSpeed) == kLevelCount && std::size(kLevelBombRadius) == kLevelCount &&
              std::size(kLevelBombColor) == kLevelCount, "bombs: one value per level");
static_assert(std::size(kLevelDiveInterval) == kLevelCount && std::size(kLevelMaxDivers) == kLevelCount,
              "divers: one value per level");
static_assert(std::size(kLevelShipTier) == kLevelCount && std::size(kLevelStarColor) == kLevelCount,
              "ship tier and star colour: one value per level");

// ---- Ship upgrades -------------------------------------------------------------------
// The ship has three looks ("tiers"); kLevelShipTier above says which level gets which.
// Each tier is bigger, fires more rockets at once, and fires a different colour.
// (The colours and parts of the ship itself are in Player.cpp.)
constexpr int   kTierCount = 3;
constexpr float kTierScale[kTierCount] = {1.0f, 1.12f, 1.25f};   // 1.0 = original size

// Where a rocket leaves the ship: x = sideways from the ship's centre, z = forward
// (negative = toward the invaders). Wing cannons sit at x = +-0.95 * scale.
struct ShotPoint {
    float x;
    float z;
};
constexpr int kTierShotCount[kTierCount] = {1, 2, 3};
constexpr ShotPoint kTierShots[kTierCount][3] = {
    {{ 0.00f, -2.7f}, { 0.00f,  0.0f}, { 0.00f,  0.0f}},   // tier 0: one rocket from the nose
    {{-1.06f, -1.3f}, { 1.06f, -1.3f}, { 0.00f,  0.0f}},   // tier 1: one from each wing cannon
    {{-1.19f, -1.4f}, { 0.00f, -3.4f}, { 1.19f, -1.4f}},   // tier 2: both cannons plus the nose
};
constexpr Color kTierShotColor[kTierCount] = {
    {150, 255,  70, 255},   // lime green
    { 80, 220, 255, 255},   // cyan
    {255,  80, 200, 255},   // hot pink
};

// ---- Divers ------------------------------------------------------------------------
// Now and then an alien leaves the formation and swoops at the player in an
// S-curve. If it misses, it comes back from the far end into its place.
// How often and how many: kLevelDiveInterval and kLevelMaxDivers (see "Levels").
constexpr float kDiveDuration   = 3.2f;    // seconds from the formation to past the player
constexpr float kDiveEndZ       = 9.0f;    // the dive ends here, behind the player
constexpr float kDiveSwing      = 4.0f;    // how far the S-curve swings sideways
constexpr float kDiveMaxRoll    = 35.0f;   // degrees the alien tilts in the curve
constexpr float kReturnStartZ   = -62.0f;  // a diver that missed re-enters from here ...
constexpr float kReturnDuration = 1.8f;    // ... and takes this long to reach its place

// ---- Lives ---------------------------------------------------------------------------
constexpr int   kStartLives         = 3;
constexpr int   kMaxLives           = 5;      // the extra-life power-up stops here
constexpr float kPlayerRespawnDelay = 1.5f;   // seconds between losing a ship and the next one
constexpr float kInvincibleTime     = 2.5f;   // the new ship blinks and cannot be hit

// ---- Power-ups -----------------------------------------------------------------------
// A destroyed alien sometimes drops a capsule that drifts toward the player.
constexpr float kPowerUpDropChance = 0.08f;   // 8% per alien; commanders always drop one
constexpr int   kExtraLifeChance   = 10;      // percent of drops that are an extra life
constexpr int   kMaxPowerUps       = 2;       // capsules on the field at the same time
constexpr float kPowerUpSpeed      = 7.0f;    // units per second toward the player
constexpr float kPowerUpRadius     = 0.7f;
constexpr float kPowerUpDespawnZ   = 6.0f;
constexpr float kShieldDuration    = 20.0f;   // the shield also ends after absorbing one hit
constexpr float kRapidFireDuration = 8.0f;
constexpr float kRapidFireInterval = 0.1f;    // normal: kRocketFireInterval (0.2)
constexpr float kPickupBannerTime  = 1.5f;    // the "SHIELD!" text after a pickup

// ---- Bombs ---------------------------------------------------------------------------
// One bomb every 5 s; its speed, size and colour per level are in "Levels".
constexpr float kBombInterval  = 5.0f;     // seconds between bombs
constexpr float kBombSpawnZ    = -42.0f;   // far end of the field, behind the invaders
constexpr float kBombSpawnY    = 9.0f;     // starts high up ...
constexpr float kBombLevelOutZ = -16.0f;   // ... and has sunk to cruise height by here
constexpr float kBombCruiseY   = 1.0f;     // the height at which rockets can hit it
constexpr float kBombDespawnZ  = 6.0f;     // flew past the ship
constexpr float kBombAimSpread = 3.0f;     // aimed at the ship's X, give or take this much
constexpr int   kScorePerBomb  = 500;
constexpr int   kBombExplosionCount = 130;
constexpr float kBombExplosionSpeed = 14.0f;
constexpr float kBombExplosionLife  = 1.3f;
constexpr float kBombExplosionSize  = 0.4f;
constexpr float kBombGlowFactor     = 4.5f;   // glow square width = radius * this

// ---- Starfield ------------------------------------------------------------------
constexpr int   kStarCount     = 520;
constexpr float kStarHalfWidth = 70.0f;    // stars spread from X = -70 to +70
constexpr float kStarMinY      = -35.0f;   // stars also float BELOW the floor (seen through the glass)
constexpr float kStarMaxY      = 45.0f;
constexpr float kStarGapMinY   = -0.5f;    // thin empty layer around the floor (y = 0) ...
constexpr float kStarGapMaxY   = 1.5f;     // ... up to the height of the ship, so stars never cut through it
constexpr float kStarFarZ      = -130.0f;  // stars are born here ...
constexpr float kStarNearZ     = 13.0f;    // ... and die here, just in front of the camera
constexpr float kStarSpeed     = 28.0f;    // base speed toward the camera, units per second
constexpr float kStarStreak    = 0.9f;     // length of a star's streak

// ---- Particles and explosions ------------------------------------------------------
constexpr int   kMaxParticles  = 1500;     // safety cap (phones will thank us later)
constexpr float kParticleGravity = 5.0f;   // downward pull, units per second squared
constexpr float kParticleDrag    = 1.6f;   // air resistance, higher = slows faster

constexpr int   kInvaderExplosionCount = 40;
constexpr float kInvaderExplosionSpeed = 9.0f;
constexpr float kInvaderExplosionLife  = 0.9f;
constexpr float kInvaderExplosionSize  = 0.5f;

constexpr int   kPlayerExplosionCount  = 110;
constexpr float kPlayerExplosionSpeed  = 12.0f;
constexpr float kPlayerExplosionLife   = 1.6f;
constexpr float kPlayerExplosionSize   = 0.26f;

// ---- Glow (the shader effect) --------------------------------------------------------
constexpr float kGlowPulseSpeed       = 18.0f;   // flicker speed in radians per second
constexpr float kRocketGlowSize       = 2.6f;    // width of the light square around a rocket
constexpr float kInvaderShotGlowSize  = 2.2f;    // ... around an invader shot

// ---- Score and waves ------------------------------------------------------------
constexpr int   kScorePerInvader   = 100;      // a Scarab; same as the 2D game (other types: see "Alien types")
constexpr float kWaveRespawnDelay  = 1.5f;     // pause before a new wave appears
constexpr float kGameOverDelay     = 1.6f;     // time to watch the explosion before the Game Over screen

}  // namespace config
