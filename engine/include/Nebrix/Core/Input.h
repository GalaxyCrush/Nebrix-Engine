#pragma once

#include "Nebrix/Math/Math.h"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct GLFWwindow;

namespace nbx
{

    // Key codes mirror GLFW's numeric layout (ASCII for letters/digits), so the
    // values are an implementation detail — always use the enum names.
    enum class Key : uint16_t
    {
        Unknown = 0,
        Space = 32,
        Apostrophe = 39,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,
        D0 = 48,
        D1 = 49,
        D2 = 50,
        D3 = 51,
        D4 = 52,
        D5 = 53,
        D6 = 54,
        D7 = 55,
        D8 = 56,
        D9 = 57,
        Semicolon = 59,
        Equal = 61,
        A = 65,
        B = 66,
        C = 67,
        D = 68,
        E = 69,
        F = 70,
        G = 71,
        H = 72,
        I = 73,
        J = 74,
        K = 75,
        L = 76,
        M = 77,
        N = 78,
        O = 79,
        P = 80,
        Q = 81,
        R = 82,
        S = 83,
        T = 84,
        U = 85,
        V = 86,
        W = 87,
        X = 88,
        Y = 89,
        Z = 90,
        LeftBracket = 91,
        Backslash = 92,
        RightBracket = 93,
        GraveAccent = 96,
        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Insert = 260,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        PageUp = 266,
        PageDown = 267,
        Home = 268,
        End = 269,
        CapsLock = 280,
        ScrollLock = 281,
        NumLock = 282,
        PrintScreen = 283,
        Pause = 284,
        F1 = 290,
        F2 = 291,
        F3 = 292,
        F4 = 293,
        F5 = 294,
        F6 = 295,
        F7 = 296,
        F8 = 297,
        F9 = 298,
        F10 = 299,
        F11 = 300,
        F12 = 301,
        KeyPad0 = 320,
        KeyPad1 = 321,
        KeyPad2 = 322,
        KeyPad3 = 323,
        KeyPad4 = 324,
        KeyPad5 = 325,
        KeyPad6 = 326,
        KeyPad7 = 327,
        KeyPad8 = 328,
        KeyPad9 = 329,
        KeyPadDecimal = 330,
        KeyPadDivide = 331,
        KeyPadMultiply = 332,
        KeyPadSubtract = 333,
        KeyPadAdd = 334,
        KeyPadEnter = 335,
        KeyPadEqual = 336,
        LeftShift = 340,
        LeftControl = 341,
        LeftAlt = 342,
        LeftSuper = 343,
        RightShift = 344,
        RightControl = 345,
        RightAlt = 346,
        RightSuper = 347,
        Menu = 348,
    };

    enum class MouseButton : uint8_t
    {
        Left = 0,
        Right = 1,
        Middle = 2,
        X1 = 3,
        X2 = 4,
    };

    // Polling-based input layer, independent of the windowing backend.
    // Bound automatically by Window (init/shutdown); apps only use the queries.
    class Input
    {
    public:
        static void bind(GLFWwindow *window);
        static void unbind();
        static void beginFrame();

        static bool isKeyDown(Key key);
        static bool isKeyDownAny(std::initializer_list<Key> keys);
        static bool isMouseDown(MouseButton button);

        // Cursor in framebuffer pixels, y-down (same space as Camera2D).
        static math::vec2 mousePosition();
        static math::vec2 mouseDelta();

        static void setMouseCapture(bool capture); // hides + locks the cursor
        static bool isMouseCaptured();

        // Named actions bound to one or more keys (e.g. "MoveLeft" -> {A, Left}).
        // Mapping an existing name replaces its keys.
        static void mapAction(std::string_view name, std::initializer_list<Key> keys);
        static void unmapAction(std::string_view name);
        static bool isActionDown(std::string_view name);

        // Translates a backend key code to a Key (used by the event layer).
        static Key translateKey(int backendKeyCode);

    private:
        static GLFWwindow *s_window;
        static math::vec2 s_mousePosition;
        static math::vec2 s_mouseDelta;
        static bool s_mouseCaptured;
        static std::unordered_map<std::string, std::vector<Key>> s_actions;
    };

} // namespace nbx