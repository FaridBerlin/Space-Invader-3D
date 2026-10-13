// InvaderGrid.cpp

#include "InvaderGrid.h"

#include <algorithm>   // std::min, std::max
#include <cstddef>     // std::size_t
#include <limits>

#include "Config.h"

InvaderGrid::InvaderGrid() {
    Respawn();
}

void InvaderGrid::Respawn() {
    invaders_.clear();
    direction_ = 1.0f;

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

void InvaderGrid::Update(float dt) {
    time_ += dt;
    if (invaders_.empty()) return;

    // 1. Move everybody sideways. "auto&" = a reference to each element, so we
    //    change the real invader and not a copy.
    const float dx = direction_ * config::kInvaderSpeed * dt;
    for (auto& invader : invaders_) {
        invader.position.x += dx;
        invader.Update(dt);   // the hit flash fades
    }

    // 2. Where is the wave now? Measuring the invaders that are left (and not the
    //    original rectangle) means the wave gets wider room as the edge columns die.
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    for (const auto& invader : invaders_) {
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
        for (auto& invader : invaders_) {
            invader.position.x += correction;
            invader.position.z += config::kInvaderStepZ;
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
        front = std::max(front, invader.position.z);
    }
    return front;
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
