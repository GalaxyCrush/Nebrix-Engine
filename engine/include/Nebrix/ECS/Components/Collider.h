#pragma once

#include "Nebrix/Math/Math.h"

namespace nbx
{

    // Axis-aligned collision box (half extents in world units, centered on the
    // Transform position). Entities with Collider + Velocity are dynamic; with
    // Collider only they act as static obstacles.
    struct Collider
    {
        math::vec2 halfExtents;
        bool solid = true; // blocks other entities

        // Convenience for the common "collider matches the sprite size" case
        // (Transform.scale is the sprite's world-space size).
        static Collider fromScale(const math::vec2 &scale)
        {
            return {scale * 0.5f, true};
        }
    };

} // namespace nbx
