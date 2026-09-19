#pragma once

#include "Nebrix/Scene/Scene.h"

#include <memory>
#include <utility>

namespace nbx
{

    // Owns the current scene and forwards lifecycle + per-frame calls.
    // switchTo destroys the old scene (onExit runs first) and enters the new
    // one, so restarting is just switching to a fresh instance.
    class SceneManager
    {
    public:
        template <typename TScene, typename... Args>
        void switchTo(Args &&...args)
        {
            if (m_current)
                m_current->onExit();
            m_current = std::make_unique<TScene>(std::forward<Args>(args)...);
            m_current->onEnter();
        }

        Scene *current() { return m_current.get(); }
        const Scene *current() const { return m_current.get(); }

        void fixedUpdate(float dt);
        void update(double dt);
        void render(const FrameStats &stats);
        void onResize(uint32_t width, uint32_t height);

    private:
        std::unique_ptr<Scene> m_current;
    };

} // namespace nbx
