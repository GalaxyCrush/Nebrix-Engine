#include "Nebrix/Core/GameLoop.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

namespace nbx
{

    namespace
    {

        double nowSeconds()
        {
            return std::chrono::duration<double>(
                       std::chrono::steady_clock::now().time_since_epoch())
                .count();
        }

    } // namespace

    void GameLoop::run()
    {
        m_running = true;
        double previous = nowSeconds();
        double accumulator = 0.0;
        uint32_t frameCount = 0;
        double fpsWindow = 0.0;

        while (m_running)
        {
            const double frameStart = nowSeconds();
            double frameTime = frameStart - previous;
            previous = frameStart;

            // Prevent the "spiral of death" after a long stall (debugger, alt-tab...).
            frameTime = std::min(frameTime, m_maxFrameTime);
            accumulator += frameTime;

            while (accumulator >= m_fixedTimestep)
            {
                if (m_fixedUpdate)
                    m_fixedUpdate(m_fixedTimestep);
                accumulator -= m_fixedTimestep;
            }

            FrameStats stats;
            stats.deltaTime = frameTime;
            stats.interpolation = accumulator / m_fixedTimestep;

            ++frameCount;
            fpsWindow += frameTime;
            if (fpsWindow >= 0.5)
            {
                m_lastFps = static_cast<uint32_t>(std::lround(frameCount / fpsWindow));
                frameCount = 0;
                fpsWindow = 0.0;
            }
            stats.fps = m_lastFps;

            if (m_update)
                m_update(frameTime);
            if (m_render)
                m_render(stats);

            if (m_frameCap > 0)
            {
                const double frameDuration = nowSeconds() - frameStart;
                const double target = 1.0 / static_cast<double>(m_frameCap);
                if (frameDuration < target)
                    std::this_thread::sleep_for(std::chrono::duration<double>(target - frameDuration));
            }
        }
    }

} // namespace nbx
