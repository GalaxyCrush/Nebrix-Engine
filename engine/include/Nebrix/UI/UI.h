#pragma once

#include "Nebrix/Math/Math.h"

#include <string_view>

namespace nbx
{

    class Font;

    // Minimal immediate-mode UI for menus and HUDs. Everything is screen space
    // (y-down pixels, top-left origin): draw the UI in its own pass with a
    // screen-space projection (ortho(0, W, H, 0)).
    //
    // Call beginFrame() once per frame before any widget; widgets are drawn
    // immediately and button() returns true on the frame the click completes
    // (press inside + release inside).
    namespace ui
    {

        struct Rect
        {
            float x = 0.0f; // left
            float y = 0.0f; // top
            float w = 0.0f;
            float h = 0.0f;
        };

        // Point in half-open rect (left/top inclusive, right/bottom exclusive).
        inline bool contains(const Rect &rect, const math::vec2 &point)
        {
            return point.x >= rect.x && point.x < rect.x + rect.w && point.y >= rect.y &&
                   point.y < rect.y + rect.h;
        }

        // Snapshots mouse state for this frame's widgets.
        void beginFrame();

        // Flat colored rectangle.
        void panel(const Rect &rect, const math::vec4 &color);

        // Left-aligned text block with its top-left corner at topLeft.
        void label(const Font &font, std::string_view text, const math::vec2 &topLeft,
                   float scale, const math::vec4 &color);

        // Text centered inside rect (single line).
        void labelCentered(const Font &font, std::string_view text, const Rect &rect, float scale,
                           const math::vec4 &color);

        // Labeled button: draws the panel (normal or hover color) with a
        // centered label. `id` must be unique per button per frame (the click
        // arm state is tracked per id). Returns true on click.
        bool button(const Font &font, std::string_view id, const Rect &rect,
                    std::string_view label, float textScale, const math::vec4 &normal,
                    const math::vec4 &hover, const math::vec4 &textColor);

        namespace detail
        {

            // Pure click-edge logic (unit-testable): `armed` latches on
            // press-inside and fires only on release-inside.
            inline bool clickEdge(bool &armed, bool down, bool prevDown, bool inside)
            {
                if (down && !prevDown && inside)
                    armed = true;
                bool clicked = false;
                if (!down && prevDown)
                {
                    clicked = armed && inside;
                    armed = false;
                }
                return clicked;
            }

        } // namespace detail

    } // namespace ui

} // namespace nbx
