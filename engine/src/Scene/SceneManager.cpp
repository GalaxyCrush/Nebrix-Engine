#include "Nebrix/Scene/SceneManager.h"

namespace nbx
{

    void SceneManager::fixedUpdate(float dt)
    {
        if (m_current)
            m_current->fixedUpdate(dt);
    }

    void SceneManager::update(double dt)
    {
        if (m_current)
            m_current->update(dt);
    }

    void SceneManager::render(const FrameStats &stats)
    {
        if (m_current)
            m_current->render(stats);
    }

    void SceneManager::onResize(uint32_t width, uint32_t height)
    {
        if (m_current)
            m_current->onResize(width, height);
    }

} // namespace nbx
