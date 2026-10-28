// InvaderGrid.cpp

#include "InvaderGrid.h"

#include <algorithm>          // std::min, std::max, std::clamp, std::stable_sort
#include <cmath>              // std::sin, std::cos, std::fabs, std::lround
#include <cstddef>            // std::size_t
#include <initializer_list>   // loops over a list like {AlienType::Scarab, ...}
#include <limits>

#include "Config.h"
#include "GlowRenderer.h"
#include "Random.h"
#include "raymath.h"   // Vector3Add, Vector3Lerp, ...

InvaderGrid::InvaderGrid() {
    Respawn(0);
}

namespace {

// The shapes a wave can have. The aliens sit on a grid of rows and columns; a
// shape decides which places of that grid get an alien.
enum class Shape { Block, Arrow, Diamond, Pincer, Checker, Chevron };

// Does the place (row, col) belong to the shape? To make the rules easy to read,
// we measure the place as two numbers from 0 to 1:
//   side  0 = the middle column, 1 = the outer columns
//   depth 0 = the front row (nearest to the player), 1 = the back row
bool InShape(Shape shape, int row, int col, int rows, int columns) {
    const float half  = 0.5f * static_cast<float>(columns - 1);
    const float side  = half > 0.0f ? std::fabs(static_cast<float>(col) - half) / half : 0.0f;
    const float depth = rows > 1 ? static_cast<float>(row) / static_cast<float>(rows - 1) : 0.0f;
    switch (shape) {
        case Shape::Block:   return true;
        case Shape::Arrow:   return side <= 0.35f + 0.7f * depth;            // a point aimed at the player
        case Shape::Diamond: return side + 1.3f * std::fabs(depth - 0.5f) <= 1.01f;
        case Shape::Pincer:  return side >= 0.3f;                            // two blocks, a gap in the middle
        case Shape::Checker: return (row + col) % 2 == 0;
        case Shape::Chevron: return side >= 0.7f * depth - 0.01f;           // a "V" opening toward the back
    }
    return true;
}

// The ways a wave can fly in (see FlyIn below).
enum class Entry { SideSwoop, TopRain, CrossLoop };

}  // namespace

void InvaderGrid::Respawn(int level, int wave) {
    invaders_.clear();
    direction_   = 1.0f;
    waveOffset_  = Vector3{0.0f, 0.0f, 0.0f};
    level_       = level;
    speed_       = config::kLevelInvaderSpeed[level];
    diveTimer_   = config::kLevelDiveInterval[level];   // a calm start for every wave

    // GetRandomValue(a, b) gives a random whole number from a to b, both included.
    const int columns = GetRandomValue(config::kInvaderMinColumns, config::kInvaderMaxColumns);
    const int rows    = GetRandomValue(config::kLevelMinRows[level], config::kLevelMaxRows[level]);

    // The shape changes from wave to wave. The very first wave of the game is a
    // plain block, so new players see the classic picture first. A shape that would
    // leave too few aliens (a small grid) falls back to the block.
    constexpr Shape kShapes[] = {Shape::Block, Shape::Arrow, Shape::Diamond, Shape::Pincer, Shape::Checker,
                                 Shape::Chevron};
    Shape shape = kShapes[(level * config::kWavesPerLevel + wave) % 6];
    int kept = 0;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) kept += InShape(shape, row, col, rows, columns) ? 1 : 0;
    }
    if (kept < 8) shape = Shape::Block;

    // Centre the wave on X = 0. The leftmost column starts at -(half the total width).
    const float startX = -0.5f * static_cast<float>(columns - 1) * config::kInvaderSpacingX;

    // Commanders sit in the middle of the back row: one, or two when the wave
    // is wide enough. For 6 columns, two commanders go to columns 2 and 4.
    const int commanders = !config::kLevelHasCommanders[level] ? 0
                         : (columns >= 6) ? config::kCommandersPerWave : 1;
    const int middle = columns / 2;
    auto isCommanderColumn = [&](int col) {
        if (commanders == 0) return false;
        if (commanders == 1) return col == middle;
        return col == middle - 1 || col == middle + 1;
    };

    // Which alien type each row gets, front to back. The back row is special
    // (Drones and Commanders). For the other rows we take the types this level
    // has, in this order of importance: Mite, Scarab, Reaper, Wasp, Moth ... as many
    // as there are rows, and sort them front to back as Mite, Scarab, Wasp, Reaper,
    // Moth. If the level has fewer types than rows, the extra rows are Scarabs.
    std::vector<AlienType> rowTypes;
    for (const AlienType t : {AlienType::Mite, AlienType::Scarab, AlienType::Reaper, AlienType::Wasp,
                              AlienType::Moth}) {
        const bool has = t == AlienType::Scarab || (t == AlienType::Reaper && config::kLevelHasReapers[level]) ||
                         (t == AlienType::Wasp && config::kLevelHasWasps[level]) ||
                         (t == AlienType::Moth && config::kLevelHasMoths[level]) ||
                         (t == AlienType::Mite && config::kLevelHasMites[level]);
        if (has && static_cast<int>(rowTypes.size()) < rows - 1) rowTypes.push_back(t);
    }
    while (static_cast<int>(rowTypes.size()) < rows - 1) rowTypes.insert(rowTypes.begin(), AlienType::Scarab);
    auto depthOrder = [](AlienType t) {
        switch (t) {
            case AlienType::Mite:   return 0;
            case AlienType::Scarab: return 1;
            case AlienType::Wasp:   return 2;
            case AlienType::Reaper: return 3;
            default:                return 4;   // Moth
        }
    };
    std::stable_sort(rowTypes.begin(), rowTypes.end(),
                     [&](AlienType a, AlienType b) { return depthOrder(a) < depthOrder(b); });

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            if (!InShape(shape, row, col, rows, columns)) continue;
            const float x = startX + static_cast<float>(col) * config::kInvaderSpacingX;
            // Row 0 is the front row. Further rows are further away (more negative Z).
            const float z = config::kInvaderStartZ - static_cast<float>(row) * config::kInvaderSpacingZ;

            AlienType type = AlienType::Scarab;
            if (row == rows - 1) {
                type = isCommanderColumn(col) ? AlienType::Commander
                     : config::kLevelHasDrones[level] ? AlienType::Drone : AlienType::Scarab;
            } else {
                type = rowTypes[static_cast<std::size_t>(row)];
            }
            // emplace_back builds the Invader directly inside the vector.
            invaders_.emplace_back(Vector3{x, config::kInvaderHeight, z}, type);
        }
    }

    constexpr Entry kEntries[] = {Entry::SideSwoop, Entry::TopRain, Entry::CrossLoop};
    FlyIn(static_cast<int>(kEntries[(level + wave) % 3]));
}

void InvaderGrid::FlyIn(int entryKind) {
    // Every alien gets a curve from outside the screen into its place: a "cubic
    // Bezier curve" from the start point to the place, pulled first toward pull1 and
    // then toward pull2 (like bending a wire with two fingers). The curve itself is
    // followed in UpdateEntering.
    //
    // The aliens come in "streams": one after the other, entryDelay apart. Each
    // stream counts its own aliens, so two streams can fly at the same time.
    const Entry entry = static_cast<Entry>(entryKind);
    int streamCount[2] = {0, 0};
    // Fly in the front rows last, so the back rows are not flown through: the
    // vector goes front row first, so we walk it backwards.
    for (auto it = invaders_.rbegin(); it != invaders_.rend(); ++it) {
        Invader& alien = *it;
        const Vector3 place = alien.position;
        const float y = place.y;
        // Which side does it come from? Left half from the left, right half from the
        // right (the middle column alternates by row).
        const int row = static_cast<int>(std::lround((config::kInvaderStartZ - place.z) / config::kInvaderSpacingZ));
        float side = place.x < -0.01f ? -1.0f : place.x > 0.01f ? 1.0f : (row % 2 == 0 ? -1.0f : 1.0f);
        if (entry == Entry::CrossLoop) side = (row % 2 == 0) ? -1.0f : 1.0f;   // whole rows from one side
        const int stream = side < 0.0f ? 0 : 1;

        switch (entry) {
            case Entry::SideSwoop:
                // In from the top corner, a swoop down toward the middle, and up into place.
                alien.diveStart  = Vector3{side * 20.0f, y, -30.0f};
                alien.entryPull1 = Vector3{side * 3.0f, y, -3.0f};
                alien.entryPull2 = Vector3{-side * 7.0f, y, -12.0f};
                break;
            case Entry::TopRain:
                // Down from far above its own column, past its place, and back up.
                alien.diveStart  = Vector3{place.x * 0.4f, y, -58.0f};
                alien.entryPull1 = Vector3{place.x + side * 6.0f, y, -30.0f};
                alien.entryPull2 = Vector3{place.x - side * 3.0f, y, -5.0f};
                break;
            case Entry::CrossLoop:
                // In low from one side, across the field, and a wide loop back up.
                alien.diveStart  = Vector3{side * 21.0f, y, -8.0f};
                alien.entryPull1 = Vector3{-side * 12.0f, y, -1.0f};
                alien.entryPull2 = Vector3{side * 10.0f, y, -36.0f};
                break;
        }
        alien.dive       = DiveState::Entering;
        alien.diveTime   = 0.0f;
        alien.slot       = place;   // the wave has not moved yet, so its place = where it was put
        alien.position   = alien.diveStart;
        alien.entryDelay = config::kEntryStagger * static_cast<float>(streamCount[stream]++);
    }
}

bool InvaderGrid::Entering() const {
    for (const auto& invader : invaders_) {
        if (invader.dive == DiveState::Entering) return true;
    }
    return false;
}

void InvaderGrid::UpdateEntering(float dt) {
    for (auto& alien : invaders_) {
        if (alien.dive != DiveState::Entering) continue;
        alien.diveTime += dt;
        const float t = std::clamp((alien.diveTime - alien.entryDelay) / config::kEntryDuration, 0.0f, 1.0f);
        const Vector3 place = Vector3Add(alien.slot, waveOffset_);
        // The cubic Bezier curve: a weighted mix of the four points. The weights
        // (u*u*u, 3*u*u*t, 3*u*t*t, t*t*t) always add up to 1 and slide from the
        // first point to the last as t goes from 0 to 1.
        auto curve = [&](float s) {
            const float u = 1.0f - s;
            const Vector3 a = Vector3Scale(alien.diveStart, u * u * u);
            const Vector3 b = Vector3Scale(alien.entryPull1, 3.0f * u * u * s);
            const Vector3 c = Vector3Scale(alien.entryPull2, 3.0f * u * s * s);
            const Vector3 d = Vector3Scale(place, s * s * s);
            return Vector3Add(Vector3Add(a, b), Vector3Add(c, d));
        };
        // Ease in and out a little, so it does not stop with a jolt at its place.
        const float eased = t * t * (3.0f - 2.0f * t) * 0.35f + t * 0.65f;
        const Vector3 before = alien.position;
        alien.position = curve(eased);
        // Tilt into the curve like a plane: the faster it moves sideways, the more.
        const float sideways = dt > 0.0f ? (alien.position.x - before.x) / dt : 0.0f;
        alien.roll = std::clamp(-sideways * 2.0f, -config::kEntryMaxRoll, config::kEntryMaxRoll);
        if (t >= 1.0f) {
            alien.position = place;
            alien.roll = 0.0f;
            alien.dive = DiveState::InFormation;
        }
    }
}

void InvaderGrid::Update(float dt, int level, float playerX) {
    time_ += dt;
    if (invaders_.empty()) return;

    for (auto& invader : invaders_) {
        invader.Update(dt);   // the hit flash fades
    }
    UpdateEntering(dt);
    // The formation only starts to move when everybody has arrived in it.
    if (!Entering()) MoveFormation(dt);
    UpdateDivers(dt, level, playerX);
}

void InvaderGrid::MoveFormation(float dt) {
    // 1. Move everybody in formation sideways. "auto&" = a reference to each
    //    element, so we change the real invader and not a copy. We also keep
    //    track of the whole wave's movement in waveOffset_: a diver needs it to
    //    find its way back to its place, which has moved on in the meantime.
    const float dx = direction_ * speed_ * dt;
    waveOffset_.x += dx;
    bool anyInFormation = false;
    for (auto& invader : invaders_) {
        if (invader.dive != DiveState::InFormation) continue;
        invader.position.x += dx;
        anyInFormation = true;
    }
    if (!anyInFormation) return;

    // 2. Where is the wave now? Measuring the invaders that are left (and not the
    //    original rectangle) means the wave gets wider room as the edge columns die.
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    for (const auto& invader : invaders_) {
        if (invader.dive != DiveState::InFormation) continue;
        minX = std::min(minX, invader.position.x);
        maxX = std::max(maxX, invader.position.x);
    }

    // 3. Did we cross the field edge? If so, push the wave back inside by the
    //    amount we overshot, turn around and step toward the player.
    const float limit = config::kFieldHalfWidth - config::kInvaderHalfWidth;
    float correction = 0.0f;
    if (maxX > limit) {
        correction = limit - maxX;       // negative: shift left
    } else if (minX < -limit) {
        correction = -limit - minX;      // positive: shift right
    }

    if (correction != 0.0f) {
        direction_ = -direction_;

        // Move back inside the field and step one notch toward the player.
        // Game checks FrontZ() and ends the game when we get too close.
        waveOffset_.x += correction;
        waveOffset_.z += config::kInvaderStepZ;
        for (auto& invader : invaders_) {
            if (invader.dive != DiveState::InFormation) continue;
            invader.position.x += correction;
            invader.position.z += config::kInvaderStepZ;
        }
    }
}

void InvaderGrid::UpdateDivers(float dt, int level, float playerX) {
    // ---- 1. Is it time for a new diver? ---------------------------------------------
    int divers = 0;
    for (const auto& invader : invaders_) {
        if (invader.dive != DiveState::InFormation) ++divers;
    }
    diveTimer_ -= dt;
    if (diveTimer_ <= 0.0f && divers < config::kLevelMaxDivers[level]) {
        // Collect who may dive (aliens in formation, but no commanders) and pick one.
        std::vector<Invader*> candidates;
        for (auto& invader : invaders_) {
            if (invader.dive == DiveState::InFormation && invader.type != AlienType::Commander) {
                candidates.push_back(&invader);   // a pointer: "where that invader lives"
            }
        }
        if (!candidates.empty()) {
            // A weighted pick ("tickets", like for the shooters): Wasps get more.
            auto tickets = [](const Invader* invader) {
                return invader->type == AlienType::Wasp ? config::kWaspDiveWeight : 1;
            };
            int total = 0;
            for (const Invader* candidate : candidates) total += tickets(candidate);
            int ticket = GetRandomValue(0, total - 1);
            std::size_t pick = 0;
            while (ticket >= tickets(candidates[pick])) {
                ticket -= tickets(candidates[pick]);
                ++pick;
            }
            Invader& diver = *candidates[pick];
            diver.dive        = DiveState::Diving;
            diver.diveTime    = 0.0f;
            diver.diveStart   = diver.position;
            diver.diveTargetX = playerX;                        // aim at the ship
            diver.diveSide    = (GetRandomValue(0, 1) == 0) ? -1.0f : 1.0f;
            diver.slot        = Vector3Subtract(diver.position, waveOffset_);   // its place
        }
        // Not like a clock: a bit random, so the player cannot count along.
        diveTimer_ = config::kLevelDiveInterval[level] * RandomFloat(0.7f, 1.3f);
    }

    // ---- 2. Move the divers along their paths -----------------------------------------
    const float limit = config::kFieldHalfWidth - config::kInvaderHalfWidth;
    for (auto& invader : invaders_) {
        if (invader.dive == DiveState::Diving) {
            invader.diveTime += dt;
            const float t = std::min(1.0f, invader.diveTime / config::kDiveDuration);   // 0 .. 1

            // Forward: straight from the start to behind the player.
            // Sideways: from the start toward the player's X ("smoothstep" makes the
            // turn soft: slow, faster, slow), plus an S-shaped swing: sin() goes
            // up and down once while t goes from 0 to 1.
            const float smooth = t * t * (3.0f - 2.0f * t);
            const float swing  = invader.diveSide * config::kDiveSwing * std::sin(2.0f * PI * t);
            const float x = invader.diveStart.x + (invader.diveTargetX - invader.diveStart.x) * smooth + swing;
            invader.position.x = std::clamp(x, -limit, limit);
            invader.position.z = invader.diveStart.z + (config::kDiveEndZ - invader.diveStart.z) * t;
            invader.position.y = invader.diveStart.y + (config::kPlayerHeight - invader.diveStart.y) * smooth;

            // Tilt into the curve, like a plane: most where the swing changes fastest.
            invader.roll = -invader.diveSide * config::kDiveMaxRoll * std::cos(2.0f * PI * t);

            if (t >= 1.0f) {
                // Missed: jump far behind the wave and fly back into its place.
                invader.dive      = DiveState::Returning;
                invader.diveTime  = 0.0f;
                invader.position  = Vector3Add(invader.slot, waveOffset_);
                invader.position.z = config::kReturnStartZ;
                invader.diveStart = invader.position;
                invader.roll      = 0.0f;
            }
        } else if (invader.dive == DiveState::Returning) {
            invader.diveTime += dt;
            const float t = std::min(1.0f, invader.diveTime / config::kReturnDuration);
            const float smooth = t * t * (3.0f - 2.0f * t);
            // The place keeps moving with the wave, so we aim at where it is NOW.
            const Vector3 place = Vector3Add(invader.slot, waveOffset_);
            invader.position = Vector3Lerp(invader.diveStart, place, smooth);
            if (t >= 1.0f) {
                invader.position = place;
                invader.dive = DiveState::InFormation;
            }
        }
    }
}

void InvaderGrid::Draw(const AlienModels& models) const {
    for (const auto& invader : invaders_) {
        invader.Draw(time_, models.Commander(), level_);
    }
}

std::optional<Vector3> InvaderGrid::PickShooterPosition() const {
    if (invaders_.empty()) {
        return std::nullopt;   // "no value"
    }
    // A weighted random choice: every alien gets "tickets", Reapers get more.
    // We draw one ticket number and walk through the aliens until we reach it.
    // Aliens still flying in do not shoot: they get no tickets.
    auto tickets = [](const Invader& invader) {
        if (invader.dive == DiveState::Entering) return 0;
        if (invader.type == AlienType::Reaper) return config::kReaperShotWeight;
        return invader.type == AlienType::Mite ? config::kMiteShotWeight : 1;
    };
    int total = 0;
    for (const auto& invader : invaders_) total += tickets(invader);
    if (total == 0) return std::nullopt;   // all of them are still on their way in

    int ticket = GetRandomValue(0, total - 1);
    for (const auto& invader : invaders_) {
        ticket -= tickets(invader);
        if (ticket < 0) return invader.position;
    }
    return invaders_.back().position;   // not reached; keeps the compiler happy
}

float InvaderGrid::FrontZ() const {
    float front = std::numeric_limits<float>::lowest();
    for (const auto& invader : invaders_) {
        if (invader.dive != DiveState::InFormation) continue;   // divers fly past on purpose
        front = std::max(front, invader.position.z);
    }
    return front;
}

std::optional<InvaderHit> InvaderGrid::TryRam(Vector3 center, float radius) {
    for (auto it = invaders_.begin(); it != invaders_.end(); ++it) {
        // Divers, and (now that the ship can fly up) aliens still in the formation.
        // A returning alien flies in from far away and is not a danger yet.
        if (it->dive != DiveState::Returning && it->dive != DiveState::Entering &&
            CheckCollisionSpheres(it->position, it->Radius(), center, radius)) {
            const InvaderHit hit{it->position, it->type, true};
            invaders_.erase(it);
            return hit;
        }
    }
    return std::nullopt;
}

std::optional<InvaderHit> InvaderGrid::TryHit(Vector3 center, float radius) {
    // We use an iterator (it) so we can erase the element we found.
    for (auto it = invaders_.begin(); it != invaders_.end(); ++it) {
        if (CheckCollisionSpheres(it->position, it->Radius(), center, radius)) {
            // Remember everything BEFORE a possible erase: afterwards "it" is invalid.
            const InvaderHit hit{it->position, it->type, it->hitPoints <= 1};
            if (hit.destroyed) {
                invaders_.erase(it);
            } else {
                it->hitPoints -= 1;
                it->flash = config::kHitFlashTime;   // light up to show the hit
            }
            return hit;   // we leave the function right away, so the erased
                          // element can never be used again: this is safe.
        }
    }
    return std::nullopt;
}

std::optional<Vector3> InvaderGrid::FirstInLine(float x, float halfWidth, float fromZ, float slope) const {
    std::optional<Vector3> nearest;
    for (const auto& invader : invaders_) {
        // Where is the line at this alien's height? Further up = more to the side.
        const float lineX  = x + slope * (fromZ - invader.position.z);
        const bool inLine  = std::fabs(invader.position.x - lineX) <= invader.Radius() + halfWidth;
        const bool inFront = invader.position.z < fromZ && invader.position.z > -config::kFieldLength;
        // "Nearest" to the ship = the biggest z (the ship is at the bottom, +z).
        if (inLine && inFront && (!nearest || invader.position.z > nearest->z)) {
            nearest = invader.position;
        }
    }
    return nearest;
}

std::optional<Vector3> InvaderGrid::NearestAlien(Vector3 from, float range,
                                                 const std::vector<Vector3>& skip) const {
    std::optional<Vector3> nearest;
    float best = range;
    for (const auto& invader : invaders_) {
        if (invader.position.z < -config::kFieldLength) continue;   // still far away, flying in
        bool skipped = false;
        for (const Vector3& s : skip) {
            if (Vector3Distance(s, invader.position) < 0.01f) skipped = true;
        }
        const float distance = Vector3Distance(from, invader.position);
        if (!skipped && distance <= best) {
            best = distance;
            nearest = invader.position;
        }
    }
    return nearest;
}

void InvaderGrid::DrawGlow(const GlowRenderer& glow, const Camera3D& camera) const {
    const Color color = Fade(AlienColorsForLevel(level_).glow, config::kAlienHaloAlpha);
    for (const auto& invader : invaders_) {
        if (invader.position.z < -config::kFieldLength) continue;   // still far away, flying in
        glow.Draw(camera, invader.position, invader.Radius() * config::kAlienHaloSize, color);
    }
}
