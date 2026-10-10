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

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            const float x = startX + static_cast<float>(col) * config::kInvaderSpacingX;
            // Row 0 is the front row. Further rows are further away (more negative Z).
            const float z = config::kInvaderStartZ - static_cast<float>(row) * config::kInvaderSpacingZ;

            // emplace_back builds the Invader directly inside the vector.
            invaders_.emplace_back(Vector3{x, config::kInvaderHeight, z});
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

void InvaderGrid::Draw() const {
    for (const auto& invader : invaders_) {
        invader.Draw(time_);
    }
}

std::optional<Vector3> InvaderGrid::PickShooterPosition() const {
    if (invaders_.empty()) {
        return std::nullopt;   // "no value"
    }
    const int index = GetRandomValue(0, static_cast<int>(invaders_.size()) - 1);
    return invaders_[static_cast<std::size_t>(index)].position;
}

float InvaderGrid::FrontZ() const {
    float front = std::numeric_limits<float>::lowest();
    for (const auto& invader : invaders_) {
        front = std::max(front, invader.position.z);
    }
    return front;
}

std::optional<Vector3> InvaderGrid::TryHit(Vector3 center, float radius) {
    // We use an iterator (it) so we can erase the element we found.
    for (auto it = invaders_.begin(); it != invaders_.end(); ++it) {
        if (CheckCollisionSpheres(it->position, config::kInvaderRadius, center, radius)) {
            const Vector3 where = it->position;   // remember it BEFORE erasing
            invaders_.erase(it);
            return where;   // we leave the function right away, so the erased
                            // element can never be used again: this is safe.
        }
    }
    return std::nullopt;
}
