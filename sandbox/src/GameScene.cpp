#include "GameScene.h"

#include "Maze.h"
#include "MenuScene.h"

#include <Nebrix/Assets/AssetManager.h>
#include <Nebrix/Core/Input.h>
#include <Nebrix/Core/Log.h>
#include <Nebrix/ECS/Components.h>
#include <Nebrix/ECS/World.h>
#include <Nebrix/Platform/Window.h>
#include <Nebrix/Renderer/Font.h>
#include <Nebrix/Renderer/Renderer.h>
#include <Nebrix/Renderer/SpriteSheet.h>
#include <Nebrix/Scene/SceneManager.h>
#include <Nebrix/UI/UI.h>

#include <algorithm>
#include <cmath>
#include <format>

namespace nbx
{

    namespace
    {

        constexpr uint32_t kWorldTiles = 100; // 100x100 grid = 10,000 tiles
        constexpr float kTileSize = 64.0f;
        constexpr uint32_t kAtlasCells = 4; // 4x4 sprite sheet
        constexpr float kWorldSize = kWorldTiles * kTileSize;

    } // namespace

    GameScene::GameScene(SceneManager &scenes, Window &window, std::function<void()> onQuit)
        : m_scenes(scenes), m_window(window), m_onQuit(std::move(onQuit)),
          m_camera(window.width(), window.height())
    {
    }

    void GameScene::onEnter()
    {
        m_font = AssetManager::getFont("mono32", "fonts/NotoSansMono-Regular.ttf", 32.0f);
        m_camera.setPosition({kWorldTiles * kTileSize * 0.5f, kWorldTiles * kTileSize * 0.5f});
        m_culling = true;
        if (const char *culling = std::getenv("NEBRIX_CULLING"))
            m_culling = std::string_view(culling) != "0";
        m_targetZoom = 1.0f;
        m_physics.setCellSize(128.0f); // broadphase cell = two tile widths
        buildWorld();
        NBX_LOG_INFO("GameScene ready: {} entities (1 player + 1 tilemap {}x{})",
                     m_world.entityCount(), kWorldTiles, kWorldTiles);
    }

    void GameScene::buildWorld()
    {
        // Real PNG assets from disk, cached by the AssetManager.
        const SpriteSheet tileset(AssetManager::getTexture("textures/tileset.png").id(),
                                  kAtlasCells, kAtlasCells, kTileSize, kTileSize);
        for (uint32_t i = 0; i < m_tileSprites.size(); ++i)
            m_tileSprites[i] = tileset.sprite(i);

        m_player = m_world.createEntity();
        m_world.add<Transform>(m_player,
                               {.position = {96.0f, 96.0f}, .scale = {48.0f, 48.0f}});
        m_world.add<Sprite>(m_player,
                            Sprite::fullTexture(
                                AssetManager::getTexture("textures/player.png").id(), 48.0f,
                                48.0f));
        m_world.add<Velocity>(m_player, {});
        m_world.add<Collider>(m_player, Collider::fromScale({48.0f, 48.0f}));
        m_world.add<Player>(m_player, {.previous = {96.0f, 96.0f}});

        // Walk cycle: 4 frames, frame 0 is the idle pose.
        const SpriteSheet walkSheet(AssetManager::getTexture("textures/player_walk.png").id(),
                                    4, 1, 48.0f, 48.0f);
        std::vector<Sprite> walkFrames;
        for (uint32_t i = 0; i < walkSheet.cellCount(); ++i)
            walkFrames.push_back(walkSheet.sprite(i));
        m_world.add<Animation>(m_player, {.frames = walkFrames, .fps = 8.0f});

        Tilemap tilemap = buildMaze(kWorldTiles, kTileSize);
        const Entity tilemapEntity = m_world.createEntity();
        m_world.add<Transform>(tilemapEntity, {});
        m_world.add<Tilemap>(tilemapEntity, std::move(tilemap));
    }

    void GameScene::onResize(uint32_t width, uint32_t height)
    {
        m_camera.setViewportSize(width, height);
    }

    void GameScene::fixedUpdate(float dt)
    {
        float dx = (Input::isActionDown("MoveRight") ? 1.0f : 0.0f) -
                   (Input::isActionDown("MoveLeft") ? 1.0f : 0.0f);
        float dy = (Input::isActionDown("MoveDown") ? 1.0f : 0.0f) -
                   (Input::isActionDown("MoveUp") ? 1.0f : 0.0f);
        math::vec2 direction(dx, dy);
        const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0.0f)
            direction = direction * (1.0f / length);

        for (auto [entity, player, transform, velocity, animation] :
             m_world.view<Player, Transform, Velocity, Animation>())
        {
            (void)entity;
            player.previous = transform.position;
            velocity.value = length > 0.0f ? direction * player.speed : math::vec2{};
            if (length > 0.0f)
                transform.rotation = std::atan2(direction.y, direction.x);
            // Walk cycle plays while moving; stopping resets to frame 0 (idle).
            animation.playing = length > 0.0f;
            if (!animation.playing)
            {
                animation.timer = 0.0f;
                animation.index = 0;
            }
        }

        // Advance all animations, then move + resolve against solid walls
        // (axis-separated, grid broadphase).
        m_animations.update(m_world, dt);
        m_physics.step(m_world, dt);

        // Defensive world-bounds clamp (walls already cover most of the edge).
        for (auto [entity, player, transform] : m_world.view<Player, Transform>())
        {
            (void)entity;
            (void)player;
            transform.position.x = std::clamp(transform.position.x, 32.0f, kWorldSize - 32.0f);
            transform.position.y = std::clamp(transform.position.y, 32.0f, kWorldSize - 32.0f);
        }
    }

    void GameScene::update(double dt)
    {
        static bool cWasDown = false;
        const bool cDown = Input::isActionDown("ToggleCulling");
        if (cDown && !cWasDown)
            m_culling = !m_culling;
        cWasDown = cDown;

        static bool rWasDown = false;
        const bool rDown = Input::isKeyDown(Key::R);
        if (rDown && !rWasDown)
        {
            NBX_LOG_INFO("Restarting scene");
            m_scenes.switchTo<GameScene>(m_scenes, m_window, m_onQuit);
            return;
        }
        rWasDown = rDown;

        static bool escWasDown = false;
        const bool escDown = Input::isKeyDown(Key::Escape);
        if (escDown && !escWasDown)
        {
            // switchTo destroys this scene: return immediately, touch nothing.
            m_scenes.switchTo<MenuScene>(m_scenes, m_window, m_onQuit);
            return;
        }
        escWasDown = escDown;

        if (Input::isActionDown("ZoomIn"))
            m_targetZoom = std::clamp(m_targetZoom * 1.02f, 0.25f, 4.0f);
        if (Input::isActionDown("ZoomOut"))
            m_targetZoom = std::clamp(m_targetZoom / 1.02f, 0.25f, 4.0f);

        m_camera.setZoom(m_camera.zoom() + (m_targetZoom - m_camera.zoom()) * 0.1f);
        m_camera.follow(m_world.get<Transform>(m_player).position, 8.0f, dt);
    }

    void GameScene::render(const FrameStats &stats)
    {
        Renderer::beginFrame(m_camera.viewProjection());

        const float interpolation = static_cast<float>(stats.interpolation);

        // Tilemap: only cells inside the camera's view (all 10k when culling is off).
        for (auto [entity, tilemap, mapTransform] : m_world.view<Tilemap, Transform>())
        {
            (void)entity;
            const float originX = mapTransform.position.x;
            const float originY = mapTransform.position.y;
            const math::vec2 viewMin =
                m_camera.position() -
                math::vec2(m_camera.viewWidth() * 0.5f, m_camera.viewHeight() * 0.5f);
            const math::vec2 viewMax =
                m_camera.position() +
                math::vec2(m_camera.viewWidth() * 0.5f, m_camera.viewHeight() * 0.5f);
            int32_t minX = 0;
            int32_t minY = 0;
            int32_t maxX = static_cast<int32_t>(tilemap.width) - 1;
            int32_t maxY = static_cast<int32_t>(tilemap.height) - 1;
            const bool visible =
                !m_culling ||
                tilemap.cellRange(viewMin.x - originX, viewMin.y - originY, viewMax.x - originX,
                                  viewMax.y - originY, minX, minY, maxX, maxY);
            if (!visible)
                continue;
            for (int32_t cy = minY; cy <= maxY; ++cy)
            {
                for (int32_t cx = minX; cx <= maxX; ++cx)
                {
                    const math::vec2 center(
                        originX + (static_cast<float>(cx) + 0.5f) * tilemap.tileSize,
                        originY + (static_cast<float>(cy) + 0.5f) * tilemap.tileSize);
                    Renderer::drawQuad(
                        {.position = center, .scale = {tilemap.tileSize, tilemap.tileSize}},
                        m_tileSprites[tilemap.indexAt(static_cast<uint32_t>(cx),
                                                      static_cast<uint32_t>(cy))]);
                }
            }
        }

        // Player: interpolated between the previous and current fixed step.
        for (auto [entity, player, transform, sprite] : m_world.view<Player, Transform, Sprite>())
        {
            (void)entity;
            Renderer::drawQuad({.position = math::lerp(player.previous, transform.position,
                                                       interpolation),
                                .rotation = transform.rotation,
                                .scale = transform.scale},
                               sprite);
        }

        Renderer::endFrame();

        // HUD pass in screen space: fps + controls hint.
        const float screenW = static_cast<float>(m_window.width());
        const float screenH = static_cast<float>(m_window.height());
        Renderer::beginFrame(math::ortho(0.0f, screenW, screenH, 0.0f, -1.0f, 1.0f));
        ui::panel({10.0f, 10.0f, 340.0f, 88.0f}, {0.05f, 0.06f, 0.08f, 0.75f});
        if (m_font)
        {
            ui::label(*m_font, std::format("fps {}", stats.fps), {22.0f, 20.0f}, 0.75f,
                      {1.0f, 1.0f, 1.0f, 1.0f});
            ui::label(*m_font, "WASD move - R restart - ESC menu", {22.0f, 52.0f}, 0.75f,
                      {0.65f, 0.70f, 0.78f, 1.0f});
        }
        Renderer::endFrame();
    }

} // namespace nbx
