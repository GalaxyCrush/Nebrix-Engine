#pragma once

#include "Nebrix/Core/Log.h"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <functional>

namespace nbx
{

    struct FrameStats
    {
        double deltaTime = 0.0;     // Real time of the last frame, in seconds.
        double interpolation = 0.0; // [0,1] fraction of the fixed timestep accumulated.
        uint32_t fps = 0;
    };

    // Fixed-timestep game loop with render interpolation.
    // The simulation runs at a constant rate (fixedUpdate); rendering runs as fast as
    // possible and receives an interpolation factor so movement stays smooth at any FPS.
    class GameLoop
    {
    public:
        using FixedUpdateFn = std::function<void(double fixedDt)>;
        using UpdateFn = std::function<void(double dt)>;
        using RenderFn = std::function<void(const FrameStats &stats)>;

        // At most this many fixed steps per frame; any further backlog is dumped
        // instead of spiraling (relevant when fixedTimestep << real frame time).
        static constexpr uint32_t kMaxStepsPerFrame = 8;

        GameLoop() = default;

        void setFixedTimestep(double seconds)
        {
            if (seconds <= 0.0 || !std::isfinite(seconds))
            {
                NBX_LOG_WARN("Ignoring invalid fixed timestep ({})", seconds);
                return;
            }
            m_fixedTimestep = seconds;
        }
        void setMaxFrameTime(double seconds)
        {
            if (seconds <= 0.0 || !std::isfinite(seconds))
            {
                NBX_LOG_WARN("Ignoring invalid max frame time ({})", seconds);
                return;
            }
            m_maxFrameTime = seconds;
        }
        // Caps render rate when vsync is unavailable (e.g. Wayland). 0 = uncapped.
        void setFrameCap(uint32_t fps) { m_frameCap = fps; }
        void setFixedUpdateFn(FixedUpdateFn fn) { m_fixedUpdate = std::move(fn); }
        void setUpdateFn(UpdateFn fn) { m_update = std::move(fn); }
        void setRenderFn(RenderFn fn) { m_render = std::move(fn); }

        void run();
        void stop() { m_running.store(false, std::memory_order_relaxed); }

    private:
        double m_fixedTimestep = 1.0 / 60.0;
        double m_maxFrameTime = 0.25;
        uint32_t m_frameCap = 0;
        uint32_t m_lastFps = 0;
        std::atomic<bool> m_running{false};
        FixedUpdateFn m_fixedUpdate;
        UpdateFn m_update;
        RenderFn m_render;
    };

} // namespace nbx
