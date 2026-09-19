#pragma once

#include "Nebrix/Math/Math.h"
#include "Nebrix/Renderer/Sprite.h"
#include "Nebrix/Renderer/Texture.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace nbx
{

    // A baked bitmap font: ASCII 32..126 rasterized with stb_truetype into one
    // RGBA atlas texture (white glyphs, coverage in alpha), so text renders
    // through the normal batch pipeline (texture * tint) in a single draw call.
    // Coordinates are y-down pixels, matching the renderer's UV convention.
    class Font
    {
    public:
        static constexpr int kFirstChar = 32;
        static constexpr int kCharCount = 96;

        struct Glyph
        {
            Sprite sprite;   // UV rect into the atlas + glyph pixel size
            float advanceX;  // horizontal advance, pixels
            float offsetX;   // left bearing, pixels (may be negative)
            float offsetY;   // distance from the baseline to the glyph top, pixels
        };

        Font() = default;

        // Rasterizes the TTF at the given pixel height. Returns false when the
        // file is missing, unreadable, or the atlas overflows.
        bool loadFromFile(const std::string &path, float pixelHeight);

        // Out-of-range characters (and controls) fall back to '?'.
        const Glyph &glyph(char c) const;

        // Pixel size of a (possibly multi-line) string at the given scale.
        math::vec2 measure(std::string_view text, float scale) const;

        float lineHeight() const { return m_lineHeight; }
        // Distance from a text block's top to its first baseline, pixels.
        float ascent() const { return m_ascent; }
        float pixelHeight() const { return m_pixelHeight; }
        const Texture &texture() const { return m_atlas; }

    private:
        Texture m_atlas;
        Glyph m_glyphs[kCharCount] = {};
        float m_lineHeight = 0.0f;
        float m_ascent = 0.0f;
        float m_pixelHeight = 0.0f;
    };

} // namespace nbx
