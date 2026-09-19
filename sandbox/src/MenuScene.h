#pragma once

#include <Nebrix/Scene/Scene.h>

#include <functional>

namespace nbx
{

    class SceneManager;
    class Window;

    // Title screen: START enters a fresh game, QUIT exits the app.
    class MenuScene : public Scene
    {
    public:
        MenuScene(SceneManager &scenes, Window &window, std::function<void()> onQuit);

        void onEnter() override;
        void render(const FrameStats &stats) override;

    private:
        SceneManager &m_scenes;
        Window &m_window;
        std::function<void()> m_onQuit;
        class Font *m_font = nullptr;
    };

} // namespace nbx
