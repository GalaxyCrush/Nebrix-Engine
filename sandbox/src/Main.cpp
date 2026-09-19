#include <Nebrix/Assets/AssetManager.h>
#include <Nebrix/Core/Events.h>
#include <Nebrix/Core/GameLoop.h>
#include <Nebrix/Core/Input.h>
#include <Nebrix/Core/Log.h>
#include <Nebrix/Platform/Window.h>
#include <Nebrix/Renderer/Renderer.h>
#include <Nebrix/Scene/SceneManager.h>
#include <Nebrix/UI/UI.h>

#include "GameScene.h"
#include "MenuScene.h"

using namespace nbx;

int main()
{
    Log::setLevel(LogLevel::Info);

    // TODO(nvidia-wayland): force X11 until the NVIDIA EGL Wayland presentation bug
    // (no buffer commits on this driver/Hyprland combo) is fixed system-side. The
    // engine itself defaults to Auto; run with NBX_PLATFORM=wayland to override.
    Window window({.title = "Nebrix Sandbox — 10k sprites",
                   .width = 1280,
                   .height = 720,
                   .vsync = true,
                   .platform = WindowProps::Platform::X11});
    if (!window.init())
        return 1;
    if (!Renderer::init())
        return 1;
    AssetManager::init();

    // Custom batch shader with hot reload (edit the files while running),
    // cached by the AssetManager. Falls back to the engine's internal shader
    // if the files are missing.
    Shader *shader =
        AssetManager::getShader("batch", "shaders/batch.vert", "shaders/batch.frag");
    if (shader)
        Renderer::setShader(shader);
    else
        NBX_LOG_WARN("Using engine's default batch shader");

    Renderer::setViewport(static_cast<int>(window.width()), static_cast<int>(window.height()));

    // Action mapping: the game logic refers to semantic actions, not raw keys.
    Input::mapAction("MoveLeft", {Key::A, Key::Left});
    Input::mapAction("MoveRight", {Key::D, Key::Right});
    Input::mapAction("MoveUp", {Key::W, Key::Up});
    Input::mapAction("MoveDown", {Key::S, Key::Down});
    Input::mapAction("ToggleCulling", {Key::C});
    Input::mapAction("ZoomIn", {Key::Equal, Key::KeyPadAdd});
    Input::mapAction("ZoomOut", {Key::Minus, Key::KeyPadSubtract});

    GameLoop loop;
    loop.setFixedTimestep(1.0 / 30.0);
    loop.setFrameCap(60);

    SceneManager scenes;
    window.setEventCallback([&](const Event &event)
                            {
        if (event.type == Event::Type::Quit) {
            loop.stop();
        } else if (event.type == Event::Type::WindowResized) {
            Renderer::setViewport(event.data1, event.data2);
            scenes.onResize(static_cast<uint32_t>(event.data1),
                            static_cast<uint32_t>(event.data2));
        } });

    // Boot into the menu when text is available, straight into the game without it.
    const auto quitApp = [&]() { loop.stop(); };
    if (AssetManager::getFont("mono32", "fonts/NotoSansMono-Regular.ttf", 32.0f))
        scenes.switchTo<MenuScene>(scenes, window, quitApp);
    else
        scenes.switchTo<GameScene>(scenes, window, quitApp);

    loop.setFixedUpdateFn([&](double fixedDt)
                          { scenes.fixedUpdate(static_cast<float>(fixedDt)); });

    uint64_t frameCounter = 0;
    loop.setUpdateFn([&](double dt)
                     {
        window.pollEvents();
        ui::beginFrame();
        if (shader && shader->update(dt)) {
            // Shader reloaded; Renderer keeps using the same Shader object.
            NBX_LOG_INFO("Shader hot-reloaded");
        }
        scenes.update(dt); });

    loop.setRenderFn([&](const FrameStats &stats)
                     {
        Renderer::clear({0.09f, 0.10f, 0.13f, 1.0f});
        scenes.render(stats);
        window.swapBuffers();

        if (++frameCounter % 60 == 0) {
            const Renderer::Stats& renderStats = Renderer::stats();
            NBX_LOG_INFO("fps={} quads={} drawCalls={}", stats.fps,
                         renderStats.quadCount, renderStats.drawCalls);
        } });

    NBX_LOG_INFO("Starting game loop (menu: click START, game: WASD, R restart, ESC menu)");
    loop.run();
    NBX_LOG_INFO("Game loop ended");

    Renderer::shutdown();
    // Cached GL resources (textures, shaders) must die while the context lives.
    AssetManager::shutdown();
    window.shutdown();
    return 0;
}
