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
// The game is TALL ("portrait"), like most phone games: on a phone you hold it
// upright, on the desktop it is a tall window, on the web a tall picture.
// The window opens at a size chosen for your monitor (see Display.cpp), and you can
// resize it. This size is only the starting size before that, and the size of the
// web version's canvas.
constexpr int kScreenWidth  = 720;
constexpr int kScreenHeight = 1280;

// Window sizes you can pick with the keys 1 .. 5, small to big (all 9:16).
constexpr int kWindowPresetCount = 5;
constexpr int kWindowPresets[kWindowPresetCount][2] = {
    {450,  800},
    {540,  960},
    {630, 1120},
    {720, 1280},
    {900, 1600},
};
constexpr float kWindowStartShare = 0.85f;   // at start: the biggest size using <= 85% of the monitor
constexpr float kWindowMaxShare   = 0.97f;   // keys 1-5: sizes up to 97% of the monitor are allowed

// The "play column": the part of the screen the game uses. On a phone it is the
// whole screen. On a wider screen (a landscape monitor in fullscreen, a tablet) it
// is a tall strip in the middle, at most this wide compared to its height; the
// sides show only space. Everything (field and HUD) is laid out inside it.
constexpr float kColumnMaxAspect = 0.62f;    // width / height; 9:16 is 0.5625

// The 2D text is designed for a column 540 x 960 pixels big. In bigger or smaller
// columns it is scaled with it, see display::Px().
constexpr float kUiReferenceWidth  = 540.0f;
constexpr float kUiReferenceHeight = 960.0f;

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
constexpr float kFieldHalfWidth = 9.5f;    // field spans X from -9.5 to +9.5 (narrow: the screen is tall)
constexpr float kFieldLength    = 50.0f;   // field spans Z from 0 down to -50

// ---- Camera -------------------------------------------------------------------
// The camera is above and behind the player and looks DOWN on the field, so the
// field stands upright on the tall screen: the aliens at the top, the ship at the
// bottom. 40 degrees keeps a real 3D look (higher angles look flat, like 2D).
// The camera places itself (see Game::FitCamera): it moves as close as it can
// while this part of the field still fits on the screen:
//   sideways: the whole field width (kFieldHalfWidth) plus kViewSideMargin
//   in depth: from kViewNearZ (just behind the ship) to kViewFarZ (the back rows)
// So it works on every screen shape, from a long phone to a square-ish tablet.
constexpr float kCameraPitchDegrees = 40.0f;   // 90 = straight down, 0 = flat behind the ship
constexpr float kCameraFovDegrees   = 34.0f;   // vertical field of view: lower = less perspective
constexpr float kViewNearZ     = 1.8f;         // the bottom edge of the picture ...
constexpr float kViewFarZ      = -36.0f;       // ... and the top edge (the back rows of a big wave)
constexpr float kViewSideMargin = 0.4f;        // a little space left and right of the field
constexpr int   kViewTopPx     = 112;          // room for the HUD at the top (UI pixels, see Px())
constexpr int   kViewBottomPx  = 8;            // ... and at the bottom

// ---- Ground grid ------------------------------------------------------------------------
// The floor grid covers only the play field (the area inside the blue border), see
// DrawGround() in Game.cpp. Outside the field you see open space. Lines must stay IN FRONT
// of the camera (it is high above and behind the ship): lines that cross the camera's
// position are clipped by the GPU, and some browsers get that wrong. The field ends at
// Z = 0, so we are safe.
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
constexpr float kPlayerSpeedZ         = 11.0f;  // top speed up / down the screen
// The ship may fly up and down in the lower half of the screen. The limits depend on
// the screen shape, so Game::FitCamera works them out from the camera:
//   up:   until the ship's centre is this far down the screen (0.5 = the middle)
//   down: until its tail touches the bottom edge of the screen
constexpr float kPlayerUpLimitShare   = 0.5f;
constexpr float kPlayerTailLength     = 1.3f;   // ship centre to the back of its hull (the flames may go off-screen)
constexpr float kPlayerBossMinZ       = -5.0f;  // in a boss fight: not further up than this (the boss is big and low)
constexpr float kPlayerMaxPitchDegrees = 12.0f; // the nose dips this much when flying forward at full speed
constexpr float kPlayerResponsiveness = 12.0f;  // higher = reaches top speed / stops faster
constexpr float kPlayerMaxBankDegrees = 35.0f;  // roll angle at top speed
constexpr float kPlayerHalfWidth      = 1.5f;   // wing tip to centre, used for the wall limit
constexpr float kPlayerHeight         = 0.6f;   // ship centre above the ground
constexpr float kPlayerHitRadius      = 1.1f;   // collision sphere of the ship (a bit smaller than the wings)
constexpr float kHeroHitRadius        = 1.5f;   // the same for the wider Claw (see "Ship looks")
constexpr float kHelpShowTime         = 6.0f;   // seconds the key help stays in the HUD after a start

// ---- Touch / mouse steering -----------------------------------------------------------
// Drag anywhere: the ship moves as far (in screen pixels) as the finger does, in
// both directions, and fires by itself while the finger is down.
constexpr float kTouchDragGain    = 1.3f;    // 1.0 = the ship follows the finger 1:1 on screen
constexpr float kTouchFollowRate  = 14.0f;   // how quickly the ship catches up with the finger
constexpr float kTouchSpeedFactor = 1.8f;    // top speed with touch, times kPlayerSpeed

// ---- Rockets ------------------------------------------------------------------
constexpr float kRocketSpeed        = 35.0f;   // units per second, flying toward -Z
constexpr float kRocketFireInterval = 0.2f;    // seconds between shots (same as the 2D game)
constexpr float kRocketRadius       = 0.3f;    // size of the rocket's collision sphere

// ---- Invaders -----------------------------------------------------------------
constexpr int   kInvaderMinColumns = 4;        // a wave gets a random size, like the 2D game
constexpr int   kInvaderMaxColumns = 6;         // (6 columns fill the narrow field)
constexpr float kInvaderSpacingX   = 2.5f;     // distance between neighbours sideways
constexpr float kInvaderSpacingZ   = 3.0f;     // distance between rows in depth
constexpr float kInvaderHalfWidth  = 0.95f;    // used to stop at the field edge
constexpr float kInvaderRadius     = 0.9f;     // size of the collision sphere
constexpr float kInvaderHeight     = 0.9f;     // centre above the ground
constexpr float kInvaderStepZ      = 0.75f;    // how far the wave steps toward you at an edge
constexpr float kInvaderStartZ     = -22.0f;   // Z of the front row when a wave spawns
constexpr float kInvaderReachZ     = -2.5f;    // an invader this close to the ship = game over

// Every alien is drawn this much bigger than its design (and its hit area grows with
// it), so the aliens are easy to see on a phone. And every alien turns its face up
// toward the high camera by this angle, so you still see its front, not its back.
constexpr float kAlienScale    = 1.35f;
constexpr float kAlienFaceTilt = 30.0f;   // degrees

// ---- Alien types -----------------------------------------------------------------
// The rows of a wave, front to back: Scarabs, Wasps, Reapers, Moths, and Drones with
// the Commanders in the back row (only the types the level has, see "Levels").
//   Wasp   a striped insect with fast wings; it likes to dive (kWaspDiveWeight)
//   Moth   a big soft insect with eye-spots on its wings; 2 hits, often drops an orb
// Most aliens are Scarabs (one hit). Commanders sit in the back row: bigger,
// three hits, more points. A commander is drawn with a 3D model file when
// assets/models/commander.glb exists, otherwise with a built-in look.
constexpr int   kScorePerReaper      = 200;
constexpr int   kScorePerDrone       = 150;
constexpr int   kScorePerCommander   = 500;
constexpr int   kScorePerWasp        = 150;
constexpr int   kScorePerMoth        = 250;
constexpr int   kMothHitPoints       = 2;
constexpr float kWaspRadius          = 0.8f;    // collision spheres (before kAlienScale)
constexpr float kMothRadius          = 1.05f;
constexpr int   kWaspDiveWeight      = 3;       // a Wasp is 3x as likely to be the next diver
constexpr float kMothOrbChance       = 0.30f;   // a Moth drops an orb 30% of the time
constexpr float kReaperScale         = 0.95f;   // drawing size of each type (1.0 = as designed)
constexpr float kDroneScale          = 1.05f;
constexpr float kWaspScale           = 0.9f;
constexpr float kMothScale           = 0.78f;   // its wings are wide: smaller, so neighbours do not overlap
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
constexpr float kInvaderShotDespawnZ = 14.0f;  // shots that fly past the ship are removed here (off-screen)

// ---- Levels --------------------------------------------------------------------------
// Seven levels of three waves each. A level ends when its third wave is destroyed;
// after level 7 the game goes on with endless bonus waves. Every level adds ONE new
// thing, so the player keeps discovering something:
//
//   level 1  Scarabs only, slow, no divers          (learn the controls)
//   level 2  Drones and Wasps join, first divers
//   level 3  Reapers join (they shoot more), faster bombs
//   level 4  Commanders join, ship look 2
//   level 5  two divers at once, new ship design (the "Claw")
//   level 6  faster waves, giant Moths, ship look 3
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
    "Scarabs incoming. Catch the purple orbs for more firepower!",
    "Wasps and Drones join - and aliens start to dive!",
    "Reapers join: they shoot more often",
    "Commanders arrive. New ship look!",
    "Two divers at once. New ship: the Claw!",
    "Giant Moths and faster waves. New ship look!",
    "Final level: everything at full speed",
};

// Which alien types may appear (Scarabs always do).
constexpr bool  kLevelHasDrones[]     = {false, true,  true,  true,  true,  true,  true};
constexpr bool  kLevelHasReapers[]    = {false, false, true,  true,  true,  true,  true};
constexpr bool  kLevelHasCommanders[] = {false, false, false, true,  true,  true,  true};
constexpr bool  kLevelHasWasps[]      = {false, true,  true,  true,  true,  true,  true};
constexpr bool  kLevelHasMoths[]      = {false, false, false, false, false, true,  true};

// The size of a wave: rows of aliens, from ... to (picked at random).
constexpr int   kLevelMinRows[] = {2, 2, 3, 3, 3, 4, 4};
constexpr int   kLevelMaxRows[] = {3, 3, 4, 4, 5, 5, 5};

constexpr float kLevelInvaderSpeed[]  = {4.0f, 4.5f, 5.0f, 5.5f, 6.0f, 6.8f, 7.5f};   // sideways, units/s
constexpr float kLevelShotInterval[]  = {2.4f, 2.2f, 2.0f, 1.8f, 1.6f, 1.4f, 1.2f};   // seconds between shots
constexpr float kLevelShotSpeed[]     = {11.0f, 12.0f, 13.0f, 15.0f, 16.0f, 18.0f, 19.0f};
constexpr float kLevelBombSpeed[]     = {9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f};
constexpr float kLevelBombRadius[]    = {0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f};     // 2D: 30 .. 50 px
// Every level has its own bomb: a new colour, and one more scary effect on top of
// the previous ones (see Bomb.cpp): pulse, energy ring, spikes, sparks, flicker,
// and in level 7 a cracked dark core with lightning.
constexpr Color kLevelBombColor[] = {
    {253, 249,   0, 255},   // 1 yellow
    { 90, 255,  90, 255},   // 2 green
    { 60, 220, 255, 255},   // 3 cyan
    {190,  90, 255, 255},   // 4 violet
    {255, 140,  30, 255},   // 5 orange
    {255,  50,  40, 255},   // 6 red
    {255,  30,  70, 255},   // 7 crimson
};
constexpr float kLevelDiveInterval[]  = {0.0f, 5.0f, 4.5f, 4.0f, 3.5f, 3.0f, 2.5f};   // 0 = no divers
constexpr int   kLevelMaxDivers[]     = {0, 1, 1, 1, 2, 2, 3};                         // at the same time
constexpr int   kLevelShipTier[]      = {0, 0, 0, 1, 1, 2, 2};                         // see "Ship looks"
constexpr bool  kLevelHeroShip[]      = {false, false, false, false, true, true, true};  // the copper "Claw"

// The colour mood of each level: the stars are tinted, from cool blue to hot red.
constexpr Color kLevelStarColor[] = {
    {200, 220, 255, 255}, {150, 200, 255, 255}, {170, 170, 255, 255}, {200, 150, 255, 255},
    {240, 140, 230, 255}, {255, 150, 140, 255}, {255, 110,  90, 255},
};

// std::size(array) counts the entries. If someone adds an 8th level to kLevelCount
// but forgets a table, the build stops right here with a clear message.
static_assert(std::size(kLevelIntro) == kLevelCount, "one intro text per level");
static_assert(std::size(kLevelHasDrones) == kLevelCount && std::size(kLevelHasReapers) == kLevelCount &&
              std::size(kLevelHasCommanders) == kLevelCount && std::size(kLevelHasWasps) == kLevelCount &&
              std::size(kLevelHasMoths) == kLevelCount, "alien types: one value per level");
static_assert(std::size(kLevelMinRows) == kLevelCount && std::size(kLevelMaxRows) == kLevelCount,
              "wave size: one value per level");
static_assert(std::size(kLevelInvaderSpeed) == kLevelCount && std::size(kLevelShotInterval) == kLevelCount &&
              std::size(kLevelShotSpeed) == kLevelCount, "invader speeds: one value per level");
static_assert(std::size(kLevelBombSpeed) == kLevelCount && std::size(kLevelBombRadius) == kLevelCount &&
              std::size(kLevelBombColor) == kLevelCount, "bombs: one value per level");
static_assert(std::size(kLevelDiveInterval) == kLevelCount && std::size(kLevelMaxDivers) == kLevelCount,
              "divers: one value per level");
static_assert(std::size(kLevelShipTier) == kLevelCount && std::size(kLevelHeroShip) == kLevelCount &&
              std::size(kLevelStarColor) == kLevelCount, "ship and star colour: one value per level");

// ---- Ship looks ------------------------------------------------------------------------
// The ship has three looks ("tiers"); kLevelShipTier above says which level gets which.
// Each tier is a little bigger and fires a different colour. (The colours and parts
// of the ship itself are in Player.cpp.) HOW MANY shots it fires is not the look: that
// is the weapon level, see "Weapons" below.
constexpr int   kTierCount = 3;
constexpr float kTierScale[kTierCount] = {1.0f, 1.12f, 1.25f};   // 1.0 = original size
constexpr Color kTierShotColor[kTierCount] = {
    {150, 255,  70, 255},   // lime green
    { 80, 220, 255, 255},   // cyan
    {255,  80, 200, 255},   // hot pink
};

// The "Claw": a second ship design, flown from level 5 (kLevelHeroShip above). It is
// a little wider than the first ship.
constexpr float kHeroHalfWidth = 2.05f;   // outer claw to centre, before the tier scale
constexpr Color kHeroShotColor{255, 245, 205, 255};  // white-gold: clearly not an alien shot (yellow)

// ---- Weapons ---------------------------------------------------------------------------
// The ship has three weapons. It starts every game with the BLASTER; a green SPREAD
// orb or an orange LASER orb (see "Power-ups") switches to that weapon. Every weapon
// has levels 1 to 5: a purple WEAPON orb raises the level of the weapon you have, and
// so does an orb of the weapon you already carry. Losing a ship costs one level, but
// you keep the weapon.
//
//   BLASTER  straight rockets; more of them per level (the table below)
//   SPREAD   a fan of rockets; wider and fuller per level: good against big waves
//   LASER    a lightning beam that burns the first thing in its way, many times a
//            second; thicker and faster per level: good against commanders and bosses

// Blaster: each level is a list of shots fired together: where a shot leaves the ship
// (x, sideways from its centre) and in which direction it flies (angle in degrees:
// 0 = straight ahead, negative = to the left, positive = to the right).
struct WeaponShot {
    float x;
    float angle;
};
constexpr int kWeaponLevels   = 5;
constexpr int kMaxWeaponShots = 7;
constexpr int kWeaponShotCount[kWeaponLevels] = {1, 2, 3, 5, 7};
constexpr WeaponShot kWeaponShots[kWeaponLevels][kMaxWeaponShots] = {
    {{ 0.0f, 0.0f}},                                                       // 1: one shot
    {{-0.45f, 0.0f}, {0.45f, 0.0f}},                                       // 2: two side by side
    {{-0.8f, 0.0f}, {0.0f, 0.0f}, {0.8f, 0.0f}},                           // 3: three
    {{-0.8f, 0.0f}, {0.0f, 0.0f}, {0.8f, 0.0f},                            // 4: three, and two
     {-0.5f, -9.0f}, {0.5f, 9.0f}},                                        //    angled to the sides
    {{-0.8f, 0.0f}, {0.0f, 0.0f}, {0.8f, 0.0f},                            // 5: a fan of seven
     {-0.5f, -9.0f}, {0.5f, 9.0f}, {-0.3f, -18.0f}, {0.3f, 18.0f}},
};
constexpr float kWeaponMuzzleZ = -2.0f;   // the shots leave this far in front of the ship's centre (times the tier size)
constexpr int   kMaxPowerBonus = 500;     // a WEAPON orb at level 5: these points instead

// Spread: how many rockets per volley, fanned out evenly from -angle to +angle.
constexpr int   kSpreadShots[kWeaponLevels]    = {3, 4, 5, 6, 7};
constexpr float kSpreadAngle[kWeaponLevels]    = {12.0f, 17.0f, 22.0f, 27.0f, 32.0f};   // degrees, to each side
constexpr Color kSpreadShotColor{110, 255, 150, 255};   // green, like its orb

// Laser: while you hold fire, the beam "ticks": every tick hurts the first alien,
// bomb or boss in its way by 1 (like one rocket). Higher levels tick faster and
// the beam is thicker (easier to aim). Rapid fire halves the tick time.
constexpr float kLaserTick[kWeaponLevels]      = {0.13f, 0.11f, 0.09f, 0.075f, 0.06f};   // seconds per hit
constexpr float kLaserHalfWidth[kWeaponLevels] = {0.25f, 0.35f, 0.45f, 0.55f, 0.70f};    // hit width, to each side
constexpr Color kLaserColor{255, 170, 50, 255};       // orange-gold lightning
constexpr Color kLaserCoreColor{255, 250, 225, 255};  // the white-hot middle
constexpr float kLaserJitterTime = 0.04f;   // the lightning changes its zig-zag this often (seconds)
constexpr float kLaserSoundTime  = 0.22f;   // the laser sound repeats this often while firing

// ---- Demo ------------------------------------------------------------------------------
// The free demo (built with the CMake option SPACE_INVADER_DEMO=ON, see CMakeLists.txt)
// ends after the boss of level 3 with a screen that points to the full game.
// "#if defined(...)" is decided by the compiler, before the program exists: the
// demo and the full game are two different builds of the same code.
#if defined(SPACE_INVADER_DEMO)
constexpr bool kDemo = true;
#else
constexpr bool kDemo = false;
#endif
constexpr int kDemoLastLevel = 2;   // 0-based: the demo ends after level 3
// The Play Store page of the full game. Empty until the app is published: the demo
// then says "coming soon". Put the link here when it is live.
constexpr const char* kFullGameUrl = "";

// ---- Bosses --------------------------------------------------------------------------
// Levels 3, 5 and 7 end with a boss: after the third wave a huge alien flies in. It
// slides from side to side and attacks in patterns (see Boss.cpp). Every hit on its
// armour costs it 1 hit point; when its core opens (it glows), a hit there costs 3.
// Below half health it gets angry: it attacks faster and with more shots.
// kLevelBoss: which boss ends each level (0, 1, 2), or -1 for none.
constexpr int   kLevelBoss[]     = {-1, -1, 0, -1, 1, -1, 2};
constexpr int   kBossCount       = 3;
constexpr const char* kBossName[kBossCount]  = {"HIVE QUEEN", "WAR TITAN", "VOID EMPEROR"};
constexpr int   kBossHitPoints[kBossCount]   = {100, 200, 300};
constexpr int   kBossScore[kBossCount]       = {5000, 8000, 12000};
constexpr float kBossScale[kBossCount]       = {1.3f, 1.45f, 1.6f};   // size of the model
constexpr float kBossSway[kBossCount]        = {2.6f, 2.0f, 1.5f};    // how far it slides left/right (bigger bosses: less room)
constexpr float kBossSwaySpeed[kBossCount]   = {0.45f, 0.55f, 0.65f}; // how fast (radians per second)
constexpr float kBossAttackPause[kBossCount] = {2.2f, 1.8f, 1.5f};    // seconds between attacks
constexpr float kBossShotSpeed[kBossCount]   = {11.0f, 13.0f, 15.0f};
constexpr float kBossRageFactor  = 0.65f;   // angry: attack pauses get this much shorter
constexpr float kBossZ           = -17.0f;  // where it hovers while fighting
constexpr float kBossPitch       = -10.0f;  // degrees it leans; negative = its face turns up toward the high camera
constexpr float kBossY           = 1.4f;    // its shots leave at the aliens' height (kInvaderHeight)
constexpr float kBossEnterZ      = -80.0f;  // it flies in from here ...
constexpr float kBossEnterTime   = 3.0f;    // ... taking this long (it cannot be hurt yet)
constexpr float kBossCoreCycle   = 6.0f;    // the core opens once every 6 seconds ...
constexpr float kBossCoreOpen    = 2.5f;    // ... for this long
constexpr int   kBossCoreDamage  = 3;       // damage of a hit on the open core
constexpr float kBossHitFlashTime = 0.08f;  // a short, light flash per hit (see Boss::Draw)
constexpr float kBossDeathTime   = 2.5f;    // the explosions before it is gone
constexpr float kBossOutroTime   = 3.5f;    // pause after the boss before the next level
static_assert(std::size(kLevelBoss) == kLevelCount, "bosses: one value per level");

// ---- Nebula ----------------------------------------------------------------------------
// Colour clouds behind the stars (see Nebula.cpp). Two colours per level; a new
// level fades to its colours. kNebulaStrength is how bright the clouds get: keep it
// low, the clouds are background and must not compete with the action.
constexpr Color kLevelNebulaA[] = {
    { 40,  60, 160, 255}, { 20, 110, 120, 255}, { 50,  50, 170, 255}, {110,  40, 160, 255},
    {170,  40, 110, 255}, {170,  50,  30, 255}, {160,  20,  50, 255},
};
constexpr Color kLevelNebulaB[] = {
    {100,  40, 140, 255}, { 30,  70, 150, 255}, { 20, 120, 160, 255}, {170,  40, 120, 255},
    {190,  90,  40, 255}, {140,  80,  20, 255}, { 70,  10,  90, 255},
};
static_assert(std::size(kLevelNebulaA) == kLevelCount && std::size(kLevelNebulaB) == kLevelCount,
              "nebula: two colours per level");
constexpr float kNebulaStrength  = 0.85f;   // 0 = no clouds, 1 = full colour
constexpr int   kNebulaDownscale = 4;       // the clouds are calculated at 1/4 width and height
constexpr float kNebulaDriftX    = 0.004f;  // drift speed, noise units per second
constexpr float kNebulaDriftY    = 0.03f;

// ---- Music ---------------------------------------------------------------------------
// Three looping tracks in assets/audio/music/ (see Audio.cpp and the README credits).
constexpr float kMusicVolume      = 0.45f;   // under the sound effects
constexpr float kMusicFadeTime    = 1.2f;    // seconds to fade from one track to another
constexpr float kMusicPausedShare = 0.35f;   // the music volume on the pause screen

// ---- Game feel ---------------------------------------------------------------------
// Screen shake: big moments add "trauma" (0 .. 1); it fades by itself. The camera
// moves by trauma squared, so small bumps stay subtle and big ones really shake.
constexpr float kShakeFade        = 1.6f;    // trauma lost per second
constexpr float kShakeMaxOffset   = 0.6f;    // camera offset at full trauma, world units
constexpr float kShakePlayerHit   = 0.75f;   // the ship is destroyed
constexpr float kShakeShieldBreak = 0.35f;   // the shield takes a hit
constexpr float kShakeBomb        = 0.3f;    // a bomb explodes (shot down)
constexpr float kShakeCommander   = 0.25f;   // a commander is destroyed
constexpr float kShakeBossBlast   = 0.15f;   // each blast while a boss dies ...
constexpr float kShakeBossDeath   = 1.0f;    // ... and the final one
// Combo: kills in quick succession raise a points multiplier.
constexpr float kComboWindow      = 1.5f;    // seconds to the next kill before the combo ends
constexpr int   kComboStep        = 5;       // kills per +1 multiplier: x2 from the 6th kill
constexpr int   kComboMaxMultiplier = 4;
// Wave-clear bonus: points for a fast wave, and extra for not losing a ship.
constexpr float kWaveParPerAlien  = 1.0f;    // "par time" of a wave: 1 second per alien
constexpr int   kSpeedBonusPerSecond = 25;   // points per second faster than par
constexpr int   kNoHitBonus       = 1000;    // no ship lost in this wave
constexpr float kBonusTextTime    = 1.6f;    // seconds the bonus text shows (the pause before the next wave)
// Bomb glow: smaller near the ship, so a big bomb never hides it.
constexpr float kBombGlowNearDistance = 7.0f;   // closer than this the glow shrinks ...
constexpr float kBombGlowNearFactor   = 0.5f;   // ... down to this share of its size

// ---- Divers ------------------------------------------------------------------------
// Now and then an alien leaves the formation and swoops at the player in an
// S-curve. If it misses, it comes back from the far end into its place.
// How often and how many: kLevelDiveInterval and kLevelMaxDivers (see "Levels").
constexpr float kDiveDuration   = 3.2f;    // seconds from the formation to past the player
constexpr float kDiveEndZ       = 14.0f;   // the dive ends here, below the bottom of the screen
constexpr float kDiveSwing      = 4.0f;    // how far the S-curve swings sideways
constexpr float kDiveMaxRoll    = 35.0f;   // degrees the alien tilts in the curve
constexpr float kReturnStartZ   = -62.0f;  // a diver that missed re-enters from here ...
constexpr float kReturnDuration = 1.8f;    // ... and takes this long to reach its place

// ---- Fly-in -------------------------------------------------------------------------
// A new wave does not just appear: its aliens fly in from outside the screen, one
// after the other in streams, along curved paths into their places (like Galaga).
// The formation waits until everybody has arrived; aliens on the way in do not shoot
// and do not ram, but you can shoot them. Which entry and which formation shape a
// wave gets changes from wave to wave (see InvaderGrid::Respawn).
constexpr float kEntryDuration = 2.2f;    // seconds for one alien's flight into its place
constexpr float kEntryStagger  = 0.11f;   // seconds between two aliens of the same stream
constexpr float kEntryMaxRoll  = 40.0f;   // degrees they tilt in the curves

// ---- Lives ---------------------------------------------------------------------------
constexpr int   kStartLives         = 3;
constexpr int   kMaxLives           = 5;      // the extra-life power-up stops here
constexpr float kPlayerRespawnDelay = 1.5f;   // seconds between losing a ship and the next one
constexpr float kInvincibleTime     = 2.5f;   // the new ship blinks and cannot be hit

// ---- Power-ups -----------------------------------------------------------------------
// A destroyed alien sometimes drops a glowing orb that drifts toward the player.
// Fly into it to collect it. Each kind has its own colour and symbol (PowerUp.cpp):
//   WEAPON      purple, three arrows   weapon level +1 (see "Weapons")
//   SPREAD      green, a fan           switch to the spread gun (or its level +1)
//   LASER       orange, a lightning    switch to the laser (or its level +1)
//   RAPID FIRE  yellow, "x2"           shoot twice as fast for a while
//   SHIELD      cyan, a shield         absorbs one hit
//   EXTRA LIFE  pink, "+1"             one more ship
constexpr float kPowerUpDropChance = 0.10f;   // 10% per alien; commanders always drop one
// Which kind a drop is, in percent (together 100):
constexpr int   kWeaponOrbChance   = 33;
constexpr int   kSpreadOrbChance   = 10;
constexpr int   kLaserOrbChance    = 10;
constexpr int   kShieldChance      = 20;
constexpr int   kRapidFireChance   = 17;
constexpr int   kExtraLifeChance   = 10;
static_assert(kWeaponOrbChance + kSpreadOrbChance + kLaserOrbChance + kShieldChance + kRapidFireChance +
              kExtraLifeChance == 100, "power-up chances must add up to 100");
constexpr int   kMaxPowerUps       = 3;       // orbs on the field at the same time
constexpr float kPowerUpSpeed      = 7.0f;    // units per second toward the player
constexpr float kPowerUpRadius     = 0.9f;    // what the ship must touch to collect it
constexpr float kPowerUpOrbSize    = 1.15f;   // how big the orb looks (radius, world units)
constexpr float kPowerUpDespawnZ   = 14.0f;
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
constexpr float kBombDespawnZ  = 14.0f;    // flew past the ship (off-screen)
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
