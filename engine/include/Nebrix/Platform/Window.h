#pragma once

#include "Nebrix/Core/Events.h"

#include <cstdint>
#include <functional>
#include <string>

struct GLFWwindow;

namespace nbx
{

    struct WindowProps
    {
        enum class Platform : uint8_t
        {
            Auto = 0,
            Wayland,
            X11
        };

        std::string title = "Nebrix";
        uint32_t width = 1280;
        uint32_t height = 720;
        bool vsync = true;
        Platform platform = Platform::Auto; // NBX_PLATFORM=x11|wayland env var overrides
    };

    class Window
    {
    public:
        using EventCallback = std::function<void(const Event &)>;

        explicit Window(const WindowProps &props = {});
        ~Window();

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        bool init();
        void shutdown();

        void pollEvents();
        void swapBuffers();

        // Framebuffer size in pixels (respects HiDPI/retina scaling).
        uint32_t width() const;
        uint32_t height() const;

        void setEventCallback(EventCallback callback);

        // Delivers an event to the registered callback (used by internal callbacks).
        void emit(const Event &event);

        GLFWwindow *handle() const { return m_window; }

    private:
        WindowProps m_props;
        GLFWwindow *m_window = nullptr;
        EventCallback m_callback;
    };

} // namespace nbx