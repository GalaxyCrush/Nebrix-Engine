#pragma once

#include <cmath>

namespace nbx::math
{

    struct vec2
    {
        float x = 0.0f;
        float y = 0.0f;

        vec2() = default;
        vec2(float x, float y) : x(x), y(y) {}

        vec2 operator+(const vec2 &other) const { return {x + other.x, y + other.y}; }
        vec2 operator-(const vec2 &other) const { return {x - other.x, y - other.y}; }
        vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
        vec2 &operator+=(const vec2 &other)
        {
            x += other.x;
            y += other.y;
            return *this;
        }
        vec2 &operator-=(const vec2 &other)
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }
    };

    inline vec2 lerp(const vec2 &a, const vec2 &b, float t) { return a + (b - a) * t; }

    // 2D transform: translation + rotation (radians) + scale.
    // Position is the quad center; scale is the quad's world-space size.
    struct Transform2D
    {
        vec2 position = {0.0f, 0.0f};
        float rotation = 0.0f;
        vec2 scale = {1.0f, 1.0f};

        static Transform2D identity() { return {}; }
    };

    // Transforms a local-space point (relative to the quad center, half-extents [-0.5, 0.5])
    // into world space. Used by the batch renderer to expand quads into vertices.
    inline vec2 transformPoint(const Transform2D &transform, const vec2 &local)
    {
        const float cosA = std::cos(transform.rotation);
        const float sinA = std::sin(transform.rotation);
        const float x = local.x * transform.scale.x;
        const float y = local.y * transform.scale.y;
        return {x * cosA - y * sinA + transform.position.x,
                x * sinA + y * cosA + transform.position.y};
    }

    struct vec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        vec3() = default;
        vec3(float x, float y, float z) : x(x), y(y), z(z) {}
        explicit vec3(const vec2 &v, float z = 0.0f) : x(v.x), y(v.y), z(z) {}
    };

    struct vec4
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;

        vec4() = default;
        vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    };

    // Column-major 4x4 matrix (same layout as OpenGL/glm expects for uniforms).
    struct mat4
    {
        float m[16] = {};

        static mat4 identity()
        {
            mat4 result{};
            result.m[0] = 1.0f;
            result.m[5] = 1.0f;
            result.m[10] = 1.0f;
            result.m[15] = 1.0f;
            return result;
        }

        const float *data() const { return m; }
    };

    // Result = a * b (apply b first, then a).
    inline mat4 mul(const mat4 &a, const mat4 &b)
    {
        mat4 result{};
        for (int col = 0; col < 4; ++col)
        {
            for (int row = 0; row < 4; ++row)
            {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k)
                    sum += a.m[k * 4 + row] * b.m[col * 4 + k];
                result.m[col * 4 + row] = sum;
            }
        }
        return result;
    }

    inline mat4 translate(const mat4 &m, const vec3 &translation)
    {
        mat4 result = m;
        result.m[12] = m.m[0] * translation.x + m.m[4] * translation.y + m.m[8] * translation.z + m.m[12];
        result.m[13] = m.m[1] * translation.x + m.m[5] * translation.y + m.m[9] * translation.z + m.m[13];
        result.m[14] = m.m[2] * translation.x + m.m[6] * translation.y + m.m[10] * translation.z + m.m[14];
        result.m[15] = m.m[3] * translation.x + m.m[7] * translation.y + m.m[11] * translation.z + m.m[15];
        return result;
    }

    inline mat4 scale(const mat4 &m, const vec3 &factors)
    {
        mat4 result = m;
        result.m[0] *= factors.x;
        result.m[1] *= factors.x;
        result.m[2] *= factors.x;
        result.m[3] *= factors.x;
        result.m[4] *= factors.y;
        result.m[5] *= factors.y;
        result.m[6] *= factors.y;
        result.m[7] *= factors.y;
        result.m[8] *= factors.z;
        result.m[9] *= factors.z;
        result.m[10] *= factors.z;
        result.m[11] *= factors.z;
        return result;
    }

    // Standard OpenGL orthographic projection (column-major).
    inline mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        mat4 result{};
        result.m[0] = 2.0f / (right - left);
        result.m[5] = 2.0f / (top - bottom);
        result.m[10] = -2.0f / (zFar - zNear);
        result.m[12] = -(right + left) / (right - left);
        result.m[13] = -(top + bottom) / (top - bottom);
        result.m[14] = -(zFar + zNear) / (zFar - zNear);
        result.m[15] = 1.0f;
        return result;
    }

} // namespace nbx::math
