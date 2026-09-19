#include "MenuScene.h"

#include "GameScene.h"

#include <Nebrix/Assets/AssetManager.h>
#include <Nebrix/Core/Log.h>
#include <Nebrix/Platform/Window.h>
#include <Nebrix/Renderer/Font.h>
#include <Nebrix/Renderer/Renderer.h>
#include <Nebrix/Scene/SceneManager.h>
#include <Nebrix/UI/UI.h>

namespace nbx
{

    MenuScene::MenuScene(SceneManager &scenes, Window &window, std::function<void()> onQuit)
        : m_scenes(scenes), m_window(window), m_onQuit(std::move(onQuit))
    {
    }

    void MenuScene::onEnter()
    {
        m_font = AssetManager::getFont("mono32", "fonts/NotoSansMono-Regular.ttf", 32.0f);
        if (!m_font)
            NBX_LOG_WARN("Menu without text: font failed to load");
    }

    void MenuScene::render(const FrameStats &stats)
    {
        (void)stats;
        if (!m_font)
            return;

        const float screenW = static_cast<float>(m_window.width());
        const float screenH = static_cast<float>(m_window.height());
        Renderer::beginFrame(math::ortho(0.0f, screenW, screenH, 0.0f, -1.0f, 1.0f));

        const float cx = screenW * 0.5f;
        const float cy = screenH * 0.5f;
        ui::labelCentered(*m_font, "NEBRIX", {0.0f, cy - 180.0f, screenW, 96.0f}, 3.0f,
                          {0.85f, 0.89f, 0.95f, 1.0f});
        ui::labelCentered(*m_font, "a 2d engine sandbox - maze - player - 10k tiles",
                          {0.0f, cy - 84.0f, screenW, 32.0f}, 1.0f,
                          {0.55f, 0.60f, 0.68f, 1.0f});
        if (ui::button(*m_font, "start", {cx - 150.0f, cy - 20.0f, 300.0f, 64.0f}, "START",
                       1.25f, {0.16f, 0.19f, 0.24f, 1.0f}, {0.37f, 0.51f, 0.67f, 1.0f},
                       {1.0f, 1.0f, 1.0f, 1.0f}))
        {
            NBX_LOG_INFO("Starting game");
            // switchTo destroys this scene: end the pass and return immediately.
            m_scenes.switchTo<GameScene>(m_scenes, m_window, m_onQuit);
            Renderer::endFrame();
            return;
        }
        if (ui::button(*m_font, "quit", {cx - 150.0f, cy + 56.0f, 300.0f, 64.0f}, "QUIT",
                       1.25f, {0.16f, 0.19f, 0.24f, 1.0f}, {0.75f, 0.38f, 0.42f, 1.0f},
                       {1.0f, 1.0f, 1.0f, 1.0f}))
            m_onQuit();
        ui::labelCentered(*m_font, "WASD / arrows move - +/- zoom - C culling - ESC menu",
                          {0.0f, screenH - 48.0f, screenW, 32.0f}, 0.75f,
                          {0.55f, 0.60f, 0.68f, 1.0f});
        Renderer::endFrame();
    }

} // namespace nbx
