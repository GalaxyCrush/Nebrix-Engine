#pragma once

#include "Nebrix/Math/Math.h"
#include "Nebrix/Renderer/Sprite.h"

#include <cstdint>
#include <string_view>

namespace nbx
{

    class Shader;
    class Font;

    // Batched 2D renderer: all quads submitted between beginFrame/endFrame are packed
    // into one vertex buffer and drawn with a single draw call per texture.
    class Renderer
    {
    public:
        struct Stats
        {
            uint32_t quadCount = 0; // quads submitted this frame
            uint32_t drawCalls = 0; // draw calls issued this frame
        };

        // Must be called once, after the window/GL context exists.
        static bool init();
        static void shutdown();

        static void setViewport(int width, int height);

        // Resets per-frame state; call once per frame before drawing.
        static void beginFrame(const math::mat4 &projection);
        static void endFrame();

        static void clear(const math::vec4 &color);

        // Transform maps quad space (centered at origin, unit size) into world space.
        static void drawQuad(const math::Transform2D &transform, const Sprite &sprite,
                             const math::vec4 &tint = {1.0f, 1.0f, 1.0f, 1.0f});
        static void drawQuad(const math::Transform2D &transform, const math::vec4 &color);

        // Text block with its TOP-LEFT corner at topLeft (y-down), one quad per
        // glyph from the font atlas (batched like everything else). Supports '\n'.
        static void drawText(const Font &font, std::string_view text, const math::vec2 &topLeft,
                             float scale, const math::vec4 &color);

        // Uses a custom shader for the batch instead of the internal default.
        // The shader must declare u_Projection, u_Texture and use the same vertex layout.
        static void setShader(Shader *shader);

        static const Stats &stats();

    private:
        static void flush();
        static void submit(const math::Transform2D &transform, const math::vec4 &uv,
                           const math::vec4 &tint);

        static bool s_initialized;
        static uint32_t s_quadCount;
        static uint32_t s_currentTextureId;
        static math::mat4 s_projection;
        static Stats s_stats;
    };

} // namespace nbx