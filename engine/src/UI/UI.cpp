#include "Nebrix/UI/UI.h"

#include "Nebrix/Core/Input.h"
#include "Nebrix/Renderer/Font.h"
#include "Nebrix/Renderer/Renderer.h"

#include <string>
#include <unordered_map>

namespace nbx::ui
{

    namespace
    {

        math::vec2 s_mouse;
        bool s_down = false;
        bool s_prevDown = false;
        std::unordered_map<std::string, bool> s_armed; // button id -> pressed-inside

    } // namespace

    void beginFrame()
    {
        s_mouse = Input::mousePosition();
        s_prevDown = s_down;
        s_down = Input::isMouseDown(MouseButton::Left);
    }

    void panel(const Rect &rect, const math::vec4 &color)
    {
        const math::Transform2D transform = {.position = {rect.x + rect.w * 0.5f,
                                                          rect.y + rect.h * 0.5f},
                                             .scale = {rect.w, rect.h}};
        Renderer::drawQuad(transform, color);
    }

    void label(const Font &font, std::string_view text, const math::vec2 &topLeft, float scale,
               const math::vec4 &color)
    {
        Renderer::drawText(font, text, topLeft, scale, color);
    }

    void labelCentered(const Font &font, std::string_view text, const Rect &rect, float scale,
                       const math::vec4 &color)
    {
        const math::vec2 size = font.measure(text, scale);
        label(font, text,
              {rect.x + (rect.w - size.x) * 0.5f, rect.y + (rect.h - size.y) * 0.5f}, scale,
              color);
    }

    bool button(const Font &font, std::string_view id, const Rect &rect, std::string_view label,
                float textScale, const math::vec4 &normal, const math::vec4 &hover,
                const math::vec4 &textColor)
    {
        const bool inside = contains(rect, s_mouse);
        const std::string key(id);
        const bool clicked = detail::clickEdge(s_armed[key], s_down, s_prevDown, inside);

        panel(rect, inside ? hover : normal);
        labelCentered(font, label, rect, textScale, textColor);
        return clicked;
    }

} // namespace nbx::ui
