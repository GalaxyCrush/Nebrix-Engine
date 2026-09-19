#include "Nebrix/Renderer/Font.h"

#include "Nebrix/Core/Log.h"

#include <stb_truetype.h>

#include <algorithm>
#include <fstream>
#include <vector>

namespace nbx
{

    namespace
    {

        constexpr int kAtlasWidth = 512;
        constexpr int kAtlasHeight = 512;

    } // namespace

    bool Font::loadFromFile(const std::string &path, float pixelHeight)
    {
        if (pixelHeight <= 0.0f)
        {
            NBX_LOG_ERROR("Font '{}': pixel height must be > 0", path);
            return false;
        }

        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        if (!stream)
        {
            NBX_LOG_ERROR("Font '{}': cannot open file", path);
            return false;
        }
        std::vector<unsigned char> data(static_cast<size_t>(stream.tellg()));
        stream.seekg(0);
        stream.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!stream)
        {
            NBX_LOG_ERROR("Font '{}': cannot read file", path);
            return false;
        }

        stbtt_fontinfo font;
        if (!stbtt_InitFont(&font, data.data(), 0))
        {
            NBX_LOG_ERROR("Font '{}': not a valid TTF/OTF", path);
            return false;
        }

        // Bake ASCII 32..126 into a 1-channel bitmap, then expand to RGBA
        // (white RGB, coverage in alpha) for the batch shader's texture*tint.
        std::vector<unsigned char> bitmap(static_cast<size_t>(kAtlasWidth * kAtlasHeight), 0);
        stbtt_bakedchar baked[kCharCount] = {};
        const int bakeResult = stbtt_BakeFontBitmap(data.data(), 0, pixelHeight, bitmap.data(),
                                                    kAtlasWidth, kAtlasHeight, kFirstChar,
                                                    kCharCount, baked);
        if (bakeResult <= 0)
        {
            NBX_LOG_ERROR("Font '{}': glyph atlas overflow at {}px (need bigger atlas)", path,
                          pixelHeight);
            return false;
        }

        std::vector<uint8_t> rgba(static_cast<size_t>(kAtlasWidth * kAtlasHeight * 4));
        for (size_t i = 0; i < bitmap.size(); ++i)
        {
            rgba[i * 4 + 0] = 255;
            rgba[i * 4 + 1] = 255;
            rgba[i * 4 + 2] = 255;
            rgba[i * 4 + 3] = bitmap[i];
        }
        if (!m_atlas.create(static_cast<uint32_t>(kAtlasWidth),
                            static_cast<uint32_t>(kAtlasHeight), rgba.data()))
            return false;

        // The bake is top-down (row 0 = glyph tops); with no texture flip this
        // matches the engine's uv.y-is-top convention directly.
        const float atlasW = static_cast<float>(kAtlasWidth);
        const float atlasH = static_cast<float>(kAtlasHeight);
        for (int i = 0; i < kCharCount; ++i)
        {
            const stbtt_bakedchar &b = baked[i];
            const float w = static_cast<float>(b.x1 - b.x0);
            const float h = static_cast<float>(b.y1 - b.y0);
            Glyph &g = m_glyphs[i];
            g.sprite = {m_atlas.id(),
                        {static_cast<float>(b.x0) / atlasW, static_cast<float>(b.y0) / atlasH,
                         static_cast<float>(b.x1) / atlasW, static_cast<float>(b.y1) / atlasH},
                        w,
                        h};
            g.advanceX = b.xadvance;
            g.offsetX = b.xoff;
            g.offsetY = b.yoff;
        }

        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
        const float scale = stbtt_ScaleForPixelHeight(&font, pixelHeight);
        m_ascent = static_cast<float>(ascent) * scale;
        m_lineHeight = static_cast<float>(ascent - descent + lineGap) * scale;
        m_pixelHeight = pixelHeight;

        NBX_LOG_INFO("Font loaded: '{}' ({}px, {}/{} atlas rows used)", path, pixelHeight,
                     bakeResult, kAtlasHeight);
        return true;
    }

    const Font::Glyph &Font::glyph(char c) const
    {
        const int code = static_cast<unsigned char>(c);
        if (code < kFirstChar || code >= kFirstChar + kCharCount)
            return m_glyphs['?' - kFirstChar];
        return m_glyphs[code - kFirstChar];
    }

    math::vec2 Font::measure(std::string_view text, float scale) const
    {
        float width = 0.0f;
        float lineWidth = 0.0f;
        uint32_t lines = 1;
        for (char ch : text)
        {
            if (ch == '\n')
            {
                width = std::max(width, lineWidth);
                lineWidth = 0.0f;
                ++lines;
                continue;
            }
            lineWidth += glyph(ch).advanceX * scale;
        }
        width = std::max(width, lineWidth);
        return {width, static_cast<float>(lines) * m_lineHeight * scale};
    }

} // namespace nbx
