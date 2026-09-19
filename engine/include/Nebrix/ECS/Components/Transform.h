#pragma once

#include "Nebrix/Math/Math.h"

namespace nbx
{

    struct Transform
    {
        math::vec2 position;
        float rotation = 0.0f;
        math::vec2 scale = {1.0f, 1.0f};
    };

} // namespace nbx
