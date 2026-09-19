#pragma once

#include "Nebrix/Math/Math.h"

namespace nbx
{

    // Marker for the playable character; keeps interpolation state.
    struct Player
    {
        math::vec2 previous; // position at the start of the current fixed step
        float speed = 320.0f;
    };

} // namespace nbx
