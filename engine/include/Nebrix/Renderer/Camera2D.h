#pragma once

#include "Nebrix/Math/Math.h"

#include <cstdint>

namespace nbx {

// Orthographic 2D camera. World coordinates are y-down (screen-like), 1 world
// unit = 1 pixel at zoom 1.
class Camera2D {
public:
    Camera2D() = default;
    Camera2D(uint32_t viewportWidth, uint32_t viewportHeight);

    void setViewportSize(uint32_t width, uint32_t height);
    void setPosition(const math::vec2& position);
    void setZoom(float zoom);

    // Smoothly moves the camera towards the target (exponential smoothing).
    void follow(const math::vec2& target, float smoothing, double dt);

    math::vec2 position() const { return m_position; }
    float zoom() const { return m_zoom; }

    // World-space size of the visible area.
    float viewWidth() const;
    float viewHeight() const;

    // Projection matrix for Renderer::beginFrame.
    math::mat4 viewProjection() const;

    // Converts window pixel coordinates to world coordinates.
    math::vec2 screenToWorld(const math::vec2& screen) const;

    // AABB visibility test (with a margin) for 2D culling.
    bool isVisible(const math::vec2& min, const math::vec2& max) const;

    void setCullMargin(float margin) { m_cullMargin = margin; }

private:
    math::vec2 m_position = {0.0f, 0.0f};
    float m_zoom = 1.0f;
    uint32_t m_viewportWidth = 1280;
    uint32_t m_viewportHeight = 720;
    float m_cullMargin = 64.0f;
};

} // namespace nbx