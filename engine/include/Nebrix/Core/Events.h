#pragma once

#include <cstdint>

namespace nbx
{

    // Minimal engine event abstraction, independent of the windowing backend.
    struct Event
    {
        enum class Type : uint8_t
        {
            None = 0,
            Quit,          // window close requested
            WindowResized, // data1 = framebuffer width, data2 = framebuffer height
            KeyPressed,    // data1 = Key, data2 = repeat count
            KeyReleased,   // data1 = Key
            FocusGained,
            FocusLost,
        };

        Type type = Type::None;
        int data1 = 0;
        int data2 = 0;
    };

} // namespace nbx