#pragma once

#include "Nebrix/Renderer/Sprite.h"

#include <cstddef>
#include <vector>

namespace nbx
{

    // A frame sequence played back onto the entity's Sprite. Frames are plain
    // Sprites resolved once at setup (no coupling to sprite sheets here); the
    // AnimationSystem advances `index` and copies the current frame into
    // Sprite every step, including when paused (index frozen = frozen pose).
    // Frame 0 is the idle pose by convention.
    struct Animation
    {
        std::vector<Sprite> frames;
        float fps = 8.0f;
        bool loop = true;
        bool playing = true;
        float timer = 0.0f;
        size_t index = 0;
    };

} // namespace nbx
