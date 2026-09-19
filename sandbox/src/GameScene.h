#pragma once

#include <Nebrix/ECS/Components.h>
#include <Nebrix/ECS/Entity.h>
#include <Nebrix/ECS/World.h>
#include <Nebrix/Physics/PhysicsSystem.h>
#include <Nebrix/Renderer/Animation.h>
#include <Nebrix/Renderer/Camera2D.h>
#include <Nebrix/Renderer/Sprite.h>
#include <Nebrix/Scene/Scene.h>

#include <array>
#include <cstdint>
#include <functional>

namespace nbx
{

    class SceneManager;
    class Window;
    class Font;

    // The maze game: player, physics, animations, HUD. R restarts (fresh
    // instance via the manager), ESC returns to the menu.
    class GameScene : public Scene
    {
    public:
        GameScene(SceneManager &scenes, Window &window, std::function<void()> onQuit);

        void onEnter() override;
        void onResize(uint32_t width, uint32_t height) override;
        void fixedUpdate(float dt) override;
        void update(double dt) override;
        void render(const FrameStats &stats) override;

    private:
        void buildWorld();

        SceneManager &m_scenes;
        Window &m_window;
        std::function<void()> m_onQuit;

        World m_world;
        Camera2D m_camera;
        PhysicsSystem m_physics;
        AnimationSystem m_animations;
        Entity m_player;
        std::array<Sprite, 16> m_tileSprites;
        Font *m_font = nullptr;
        bool m_culling = true;
        float m_targetZoom = 1.0f;
    };

} // namespace nbx
