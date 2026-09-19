#pragma once

#include "Nebrix/Math/Math.h"

#include <algorithm>

namespace nbx {

// Axis-aligned bounding box in world space.
struct AABB {
    math::vec2 min;
    math::vec2 max;

    static AABB fromCenterHalf(const math::vec2& center, const math::vec2& half) {
        return {center - half, center + half};
    }

    bool overlaps(const AABB& other) const {
        return min.x < other.max.x && max.x > other.min.x && min.y < other.max.y &&
               max.y > other.min.y;
    }

    // Positive while overlapping: how much the boxes interpenetrate.
    float overlapX(const AABB& other) const {
        return std::min(max.x, other.max.x) - std::max(min.x, other.min.x);
    }

    float overlapY(const AABB& other) const {
        return std::min(max.y, other.max.y) - std::max(min.y, other.min.y);
    }
};

} // namespace nbx