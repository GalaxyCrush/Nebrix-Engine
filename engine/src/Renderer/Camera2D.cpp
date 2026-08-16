#include "Nebrix/Renderer/Camera2D.h"

#include <algorithm>
#include <cmath>

namespace nbx {

Camera2D::Camera2D(uint32_t viewportWidth, uint32_t viewportHeight)
    : m_viewportWidth(viewportWidth), m_viewportHeight(viewportHeight) {}

void Camera2D::setViewportSize(uint32_t width, uint32_t height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
}

void Camera2D::setPosition(const math::vec2& position) { m_position = position; }

void Camera2D::setZoom(float zoom) { m_zoom = std::max(zoom, 0.01f); }

void Camera2D::follow(const math::vec2& target, float smoothing, double dt) {
    const float factor = 1.0f - std::exp(-smoothing * static_cast<float>(dt));
    m_position = math::lerp(m_position, target, factor);
}

float Camera2D::viewWidth() const {
    return static_cast<float>(m_viewportWidth) / m_zoom;
}

float Camera2D::viewHeight() const {
    return static_cast<float>(m_viewportHeight) / m_zoom;
}

math::mat4 Camera2D::viewProjection() const {
    const float halfWidth = viewWidth() * 0.5f;
    const float halfHeight = viewHeight() * 0.5f;
    // Y-down world: top of the screen is at position.y - halfHeight.
    return math::ortho(m_position.x - halfWidth, m_position.x + halfWidth,
                       m_position.y + halfHeight, m_position.y - halfHeight, -1.0f, 1.0f);
}

math::vec2 Camera2D::screenToWorld(const math::vec2& screen) const {
    const float dx = (screen.x - static_cast<float>(m_viewportWidth) * 0.5f) / m_zoom;
    const float dy = (screen.y - static_cast<float>(m_viewportHeight) * 0.5f) / m_zoom;
    return {m_position.x + dx, m_position.y + dy};
}

bool Camera2D::isVisible(const math::vec2& min, const math::vec2& max) const {
    const float halfWidth = viewWidth() * 0.5f + m_cullMargin;
    const float halfHeight = viewHeight() * 0.5f + m_cullMargin;
    if (max.x < m_position.x - halfWidth || min.x > m_position.x + halfWidth)
        return false;
    if (max.y < m_position.y - halfHeight || min.y > m_position.y + halfHeight)
        return false;
    return true;
}

} // namespace nbx