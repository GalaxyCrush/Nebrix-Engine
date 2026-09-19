#include "Nebrix/Core/Input.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>

namespace nbx
{

    GLFWwindow *Input::s_window = nullptr;
    math::vec2 Input::s_mousePosition;
    math::vec2 Input::s_mouseDelta;
    bool Input::s_mouseCaptured = false;
    std::unordered_map<std::string, std::vector<Key>, Input::ActionHash, std::equal_to<>>
        Input::s_actions;

    namespace
    {

        // Cursor in framebuffer pixels (logical window coords scaled by the
        // HiDPI ratio), y-down - the same space Camera2D works in.
        math::vec2 scaledCursor(GLFWwindow *window)
        {
            double x = 0.0;
            double y = 0.0;
            glfwGetCursorPos(window, &x, &y);

            int windowWidth = 0;
            int windowHeight = 0;
            int drawableWidth = 0;
            int drawableHeight = 0;
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
            glfwGetFramebufferSize(window, &drawableWidth, &drawableHeight);

            const float scaleX = static_cast<float>(drawableWidth) /
                                 static_cast<float>(std::max(windowWidth, 1));
            const float scaleY = static_cast<float>(drawableHeight) /
                                 static_cast<float>(std::max(windowHeight, 1));
            return {static_cast<float>(x) * scaleX, static_cast<float>(y) * scaleY};
        }

    } // namespace

    void Input::bind(GLFWwindow *window)
    {
        s_window = window;
        // Initialize from the current cursor position so the first frame's
        // delta is not a jump from (0,0).
        s_mousePosition = s_window ? scaledCursor(s_window) : math::vec2{};
        s_mouseDelta = {};
    }

    void Input::unbind()
    {
        if (s_window && s_mouseCaptured)
            glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        s_window = nullptr;
        s_mousePosition = {};
        s_mouseDelta = {};
        s_mouseCaptured = false;
    }

    void Input::beginFrame()
    {
        if (!s_window)
            return;

        const math::vec2 position = scaledCursor(s_window);
        s_mouseDelta = position - s_mousePosition;
        s_mousePosition = position;
    }

    bool Input::isKeyDown(Key key)
    {
        if (!s_window)
            return false;
        return glfwGetKey(s_window, static_cast<int>(key)) == GLFW_PRESS;
    }

    bool Input::isKeyDownAny(std::initializer_list<Key> keys)
    {
        for (const Key key : keys)
            if (isKeyDown(key))
                return true;
        return false;
    }

    bool Input::isMouseDown(MouseButton button)
    {
        if (!s_window)
            return false;
        return glfwGetMouseButton(s_window, static_cast<int>(button)) == GLFW_PRESS;
    }

    math::vec2 Input::mousePosition() { return s_mousePosition; }

    math::vec2 Input::mouseDelta() { return s_mouseDelta; }

    void Input::setMouseCapture(bool capture)
    {
        if (!s_window)
            return;
        s_mouseCaptured = capture;
        glfwSetInputMode(s_window, GLFW_CURSOR,
                         capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    bool Input::isMouseCaptured() { return s_mouseCaptured; }

    void Input::mapAction(std::string_view name, std::initializer_list<Key> keys)
    {
        s_actions[std::string(name)] = std::vector<Key>(keys);
    }

    void Input::unmapAction(std::string_view name) { s_actions.erase(std::string(name)); }

    bool Input::isActionDown(std::string_view name)
    {
        // Heterogeneous lookup: no temporary std::string allocation per query.
        const auto it = s_actions.find(name);
        if (it == s_actions.end())
            return false;
        for (const Key key : it->second)
            if (isKeyDown(key))
                return true;
        return false;
    }

    Key Input::translateKey(int backendKeyCode)
    {
        // Valid codes are exactly the values enumerated in Key: printable ASCII
        // (with the gaps GLFW never emits) plus the 256..348 keypad/function ranges.
        if (backendKeyCode >= 32 && backendKeyCode <= 96)
        {
            if (backendKeyCode == 33 || backendKeyCode == 34 || backendKeyCode == 35 ||
                backendKeyCode == 36 || backendKeyCode == 37 || backendKeyCode == 38 ||
                backendKeyCode == 40 || backendKeyCode == 41 || backendKeyCode == 42 ||
                backendKeyCode == 43 || backendKeyCode == 58 || backendKeyCode == 60 ||
                backendKeyCode == 62 || backendKeyCode == 63 || backendKeyCode == 64 ||
                backendKeyCode == 94 || backendKeyCode == 95)
                return Key::Unknown;
            return static_cast<Key>(backendKeyCode);
        }
        if (backendKeyCode >= 256 && backendKeyCode <= 269)
            return static_cast<Key>(backendKeyCode);
        if (backendKeyCode >= 280 && backendKeyCode <= 284)
            return static_cast<Key>(backendKeyCode);
        if (backendKeyCode >= 290 && backendKeyCode <= 301)
            return static_cast<Key>(backendKeyCode);
        if (backendKeyCode >= 320 && backendKeyCode <= 336)
            return static_cast<Key>(backendKeyCode);
        if (backendKeyCode >= 340 && backendKeyCode <= 348)
            return static_cast<Key>(backendKeyCode);
        return Key::Unknown;
    }

} // namespace nbx