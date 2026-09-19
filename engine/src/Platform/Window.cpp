#include "Nebrix/Platform/Window.h"

#include "Nebrix/Core/Input.h"
#include "Nebrix/Core/Log.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <string_view>

namespace nbx
{

    namespace
    {

        // Tracks whether this process successfully ran glfwInit, so shutdown can
        // be idempotent and safe even when init failed early or was never called.
        bool s_glfwInitialized = false;

        Window *windowFrom(GLFWwindow *window)
        {
            return static_cast<Window *>(glfwGetWindowUserPointer(window));
        }

        void framebufferSizeCallback(GLFWwindow *window, int width, int height)
        {
            if (Window *self = windowFrom(window))
            {
                Event event;
                event.type = Event::Type::WindowResized;
                event.data1 = width;
                event.data2 = height;
                self->emit(event);
            }
        }

        void windowCloseCallback(GLFWwindow *window)
        {
            if (Window *self = windowFrom(window))
            {
                Event event;
                event.type = Event::Type::Quit;
                self->emit(event);
            }
        }

        void keyCallback(GLFWwindow *window, int key, int /*scancode*/, int action, int mods)
        {
            (void)mods;
            if (Window *self = windowFrom(window))
            {
                Event event;
                event.type = action == GLFW_RELEASE ? Event::Type::KeyReleased
                                                    : Event::Type::KeyPressed;
                event.data1 = static_cast<int>(Input::translateKey(key));
                event.data2 = action == GLFW_REPEAT ? 1 : 0;
                self->emit(event);
            }
        }

        void windowFocusCallback(GLFWwindow *window, int focused)
        {
            if (Window *self = windowFrom(window))
            {
                Event event;
                event.type = focused ? Event::Type::FocusGained : Event::Type::FocusLost;
                self->emit(event);
            }
        }

        const char *platformName()
        {
            switch (glfwGetPlatform())
            {
            case GLFW_PLATFORM_WAYLAND:
                return "wayland";
            case GLFW_PLATFORM_X11:
                return "x11";
            default:
                return "other";
            }
        }

    } // namespace

    Window::Window(const WindowProps &props) : m_props(props) {}

    Window::~Window() { shutdown(); }

    bool Window::init()
    {
        if (const char *platformEnv = std::getenv("NBX_PLATFORM"))
        {
            if (std::string_view(platformEnv) == "x11")
                m_props.platform = WindowProps::Platform::X11;
            else if (std::string_view(platformEnv) == "wayland")
                m_props.platform = WindowProps::Platform::Wayland;
            else
                NBX_LOG_WARN("Unknown NBX_PLATFORM '{}' (expected 'x11' or 'wayland')", platformEnv);
        }
        switch (m_props.platform)
        {
        case WindowProps::Platform::X11:
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
            break;
        case WindowProps::Platform::Wayland:
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
            break;
        case WindowProps::Platform::Auto:
            break;
        }

        if (!glfwInit())
        {
            NBX_LOG_ERROR("GLFW init failed");
            return false;
        }
        s_glfwInitialized = true;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        // EGL instead of GLX: GLX content does not present on Xwayland (NVIDIA), EGL does.
        // On Wayland GLFW uses EGL regardless; on X11 this selects the EGL-on-X11 path.
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
#ifdef NEBRIX_DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);

        m_window = glfwCreateWindow(static_cast<int>(m_props.width),
                                    static_cast<int>(m_props.height), m_props.title.c_str(),
                                    nullptr, nullptr);
        if (!m_window)
        {
            NBX_LOG_ERROR("Failed to create window");
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(m_props.vsync ? 1 : 0);

        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
        glfwSetWindowCloseCallback(m_window, windowCloseCallback);
        glfwSetKeyCallback(m_window, keyCallback);
        glfwSetWindowFocusCallback(m_window, windowFocusCallback);
        Input::bind(m_window);

        int windowWidth = 0;
        int windowHeight = 0;
        int drawableWidth = 0;
        int drawableHeight = 0;
        int windowX = 0;
        int windowY = 0;
        glfwGetWindowSize(m_window, &windowWidth, &windowHeight);
        glfwGetFramebufferSize(m_window, &drawableWidth, &drawableHeight);
        glfwGetWindowPos(m_window, &windowX, &windowY);
        NBX_LOG_INFO("Window created: {}x{} '{}' (logical {}x{}, drawable {}x{}, position {},{} "
                     "via GLFW {} on '{}')",
                     m_props.width, m_props.height, m_props.title, windowWidth, windowHeight,
                     drawableWidth, drawableHeight, windowX, windowY, glfwGetVersionString(),
                     platformName());
        return true;
    }

    void Window::shutdown()
    {
        if (m_window)
        {
            Input::unbind();
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        if (s_glfwInitialized)
        {
            glfwTerminate();
            s_glfwInitialized = false;
        }
    }

    void Window::pollEvents()
    {
        glfwPollEvents();
        Input::beginFrame();
    }

    void Window::swapBuffers() { glfwSwapBuffers(m_window); }

    uint32_t Window::width() const
    {
        if (!m_window)
            return 0;
        int width = 0;
        glfwGetFramebufferSize(m_window, &width, nullptr);
        return static_cast<uint32_t>(width);
    }

    uint32_t Window::height() const
    {
        if (!m_window)
            return 0;
        int height = 0;
        glfwGetFramebufferSize(m_window, nullptr, &height);
        return static_cast<uint32_t>(height);
    }

    void Window::setEventCallback(EventCallback callback) { m_callback = std::move(callback); }

    void Window::emit(const Event &event)
    {
        if (m_callback)
            m_callback(event);
    }

} // namespace nbx