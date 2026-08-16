#pragma once

#include "Nebrix/Math/Math.h"

#include <cstdint>

namespace nbx {

// A region of a texture, defined by normalized UVs (0..1).
// width/height are the region's pixel size (the sprite's natural size).
// Stores the texture id instead of a pointer so sprites stay valid if the
// owning texture is moved.
struct Sprite {
    uint32_t textureId = 0;
    math::vec4 uv = {0.0f, 0.0f, 1.0f, 1.0f};  // minX, minY, maxX, maxY
    float width = 0.0f;
    float height = 0.0f;

    static Sprite fullTexture(uint32_t textureId, float width, float height) {
        return {textureId, {0.0f, 0.0f, 1.0f, 1.0f}, width, height};
    }
};

} // namespace nbx