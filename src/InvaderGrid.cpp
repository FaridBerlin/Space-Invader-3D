// InvaderGrid.cpp

#include "InvaderGrid.h"

#include <algorithm>   // std::min, std::max, std::clamp
#include <cmath>       // std::sin, std::cos
#include <cstddef>     // std::size_t
#include <limits>

#include "Config.h"
#include "Random.h"
#include "raymath.h"   // Vector3Add, Vector3Lerp, ...

InvaderGrid::InvaderGrid() {
    Respawn();
}

void InvaderGrid::Respawn() {
    invaders_.clear();
    direction_   = 1.0f;
    waveOffset_  = Vector3{0.0f, 0.0f, 0.0f};
    diveTimer_   = config::kDiveInterval[0];   // a short calm start for every wave

    // GetRandomValue(a, b) gives a random whole number from a to b, both included.
    const int columns = GetRandomValue(config::kInvaderMinColumns, config::kInvaderMaxColumns);
    const int rows    = GetRandomValue(config::kInvaderMinRows, config::kInvaderMaxRows);

    // Centre the wave on X = 0. The leftmost column starts at -(half the total width).
    const float startX = -0.5f * static_cast<float>(columns - 1) * config::kInvaderSpacingX;

    // Commanders sit in the middle of the back row: one, or two when the wave
    // is wide enough. For 7 columns, two commanders go to columns 2 and 4.
    const int commanders = (columns >= 6) ? config::kCommandersPerWave : 1;
    const int middle = columns / 2;
    auto isCommanderColumn = [&](int col) {
        if (commanders == 1) return col == middle;
        return col == middle - 1 || col == middle + 1;
    };

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            const float x = startX + static_cast<float>(col) * config::kInvaderSpacingX;
            // Row 0 is the front row. Further rows are further away (more negative Z).
            const float z = config::kInvaderStartZ - static_cast<float>(row) * config::kInvaderSpacingZ;

            // Which kind of alien goes here? Front half: Scarabs. Back row: Drones
            // (and the commanders). In between: Reapers. With 5 rows that gives
            // Scarab, Scarab, Reaper, Reaper, Drone (front to back).
            const bool backRow = (row == rows - 1);
            AlienType type = AlienType::Reaper;
            if (backRow) {
                type = isCommanderColumn(col) ? AlienType::Commander : AlienType::Drone;
            } else if (row < rows / 2) {
                type = AlienType::Scarab;
            }
            // emplace_back builds the Invader directly inside the vector.
            invaders_.emplace_back(Vector3{x, config::kInvaderHeight, z}, type);
        }
    }
}

void InvaderGrid::Update(float dt, int level, float playerX) {
    time_ += dt;
    if (invaders_.empty()) return;

    for (auto& invader : invaders_) {
        invader.Update(dt);   // the hit flash fades
    }
    MoveFormation(dt);
    UpdateDivers(dt, level, playerX);
}

void InvaderGrid::MoveFormation(float dt) {
    // 1. Move everybody in formation sideways. "auto&" = a reference to each
    //    element, so we change the real invader and not a copy. We also keep
    //    track of the whole wave's movement in waveOffset_: a diver needs it to
    //    find its way back to its place, which has moved on in the meantime.
    const float dx = direction_ * config::kInvaderSpeed * dt;
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
    if (diveTimer_ <= 0.0f && divers < config::kMaxDivers[level]) {
        // Collect who may dive (aliens in formation, but no commanders) and pick one.
        std::vector<Invader*> candidates;
        for (auto& invader : invaders_) {
            if (invader.dive == DiveState::InFormation && invader.type != AlienType::Commander) {
                candidates.push_back(&invader);   // a pointer: "where that invader lives"
            }
        }
        if (!candidates.empty()) {
            const int pick = GetRandomValue(0, static_cast<int>(candidates.size()) - 1);
            Invader& diver = *candidates[static_cast<std::size_t>(pick)];
            diver.dive        = DiveState::Diving;
            diver.diveTime    = 0.0f;
            diver.diveStart   = diver.position;
            diver.diveTargetX = playerX;                        // aim at the ship
            diver.diveSide    = (GetRandomValue(0, 1) == 0) ? -1.0f : 1.0f;
            diver.slot        = Vector3Subtract(diver.position, waveOffset_);   // its place
        }
        // Not like a clock: a bit random, so the player cannot count along.
        diveTimer_ = config::kDiveInterval[level] * RandomFloat(0.7f, 1.3f);
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
        invader.Draw(time_, models.Commander());
    }
}

std::optional<Vector3> InvaderGrid::PickShooterPosition() const {
    if (invaders_.empty()) {
        return std::nullopt;   // "no value"
    }
    // A weighted random choice: every alien gets "tickets", Reapers get more.
    // We draw one ticket number and walk through the aliens until we reach it.
    auto tickets = [](const Invader& invader) {
        return invader.type == AlienType::Reaper ? config::kReaperShotWeight : 1;
    };
    int total = 0;
    for (const auto& invader : invaders_) total += tickets(invader);

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
        if (it->dive == DiveState::Diving &&
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
