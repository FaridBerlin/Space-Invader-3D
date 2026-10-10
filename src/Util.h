// Util.h - small helpers shared by several files.

#pragma once

#include <algorithm>   // std::remove_if
#include <vector>

// A function TEMPLATE: one definition that works for any type T. The compiler
// writes a separate version for every T we call it with (Rocket, InvaderShot,
// Particle ...). T only needs an "alive" flag.
//
// Templates must live in a header: the compiler needs to see the full code in
// every file that uses it, so it can generate the version for that type.
//
// This is the erase-remove idiom: remove_if moves all items we want to keep to
// the front and returns where the leftover (dead) ones start; erase then cuts
// them off. Call it AFTER the loops that use the items, never during them,
// which avoids the classic bug of changing a list while walking through it.
template <typename T>
void RemoveDead(std::vector<T>& items) {
    items.erase(
        std::remove_if(items.begin(), items.end(),
                       [](const T& item) { return !item.alive; }),
        items.end());
}
