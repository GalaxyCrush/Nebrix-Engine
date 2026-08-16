#include <Nebrix/Core/Assert.h>
#include <Nebrix/Core/Events.h>
#include <Nebrix/Core/GameLoop.h>
#include <Nebrix/Core/Input.h>
#include <Nebrix/Core/Log.h>
#include <Nebrix/Math/Math.h>
#include <Nebrix/Platform/Window.h>
#include <Nebrix/Renderer/Camera2D.h>
#include <Nebrix/Renderer/Renderer.h>
#include <Nebrix/Renderer/Shader.h>
#include <Nebrix/Renderer/TextureAtlas.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <vector>

using namespace nbx;

namespace
{

    constexpr uint32_t kWorldTiles = 100; // 100x100 grid = 10,000 tiles
    constexpr float kTileSize = 64.0f;
    constexpr uint32_t kAtlasCells = 4; // 4x4 sprite sheet

    constexpr std::array<uint32_t, 16> kTileColors = {
        0xFF40342E,
        0xFF52423B,
        0xFF5E4C43,
        0xFF6A564C,
        0xFFBBBC8F,
        0xFFD0C088,
        0xFFC1A181,
        0xFFAC815E,
        0xFF6A61BF,
        0xFF7087D0,
        0xFF8BCBEB,
        0xFF8CBEA3,
        0xFFAD8EB4,
        0xFF80726B,
        0xFFAFA39C,
        0xFFDBD5D1,
    };

    // Procedurally builds a 4x4 tile sheet with borders so cells are visible.
    Texture makeTileAtlasTexture()
    {
        constexpr uint32_t cell = 64;
        constexpr uint32_t size = cell * kAtlasCells;
        std::vector<uint32_t> pixels(size * size);
        for (uint32_t cy = 0; cy < kAtlasCells; ++cy)
        {
            for (uint32_t cx = 0; cx < kAtlasCells; ++cx)
            {
                const uint32_t color = kTileColors[cy * kAtlasCells + cx];
                for (uint32_t y = 0; y < cell; ++y)
                {
                    for (uint32_t x = 0; x < cell; ++x)
                    {
                        uint32_t pixel = color;
                        if (x < 4 || y < 4 || x >= cell - 4 || y >= cell - 4)
                            pixel = 0xFF1C1511;
                        pixels[(cy * cell + y) * size + cx * cell + x] = pixel;
                    }
                }
            }
        }
        Texture texture;
        texture.create(size, size, pixels.data());
        return texture;
    }

    // Resolves asset paths relative to the repository root, regardless of the CWD.
    // The executable lives at <repo>/build/sandbox/ (or build-release/sandbox/).
    std::filesystem::path assetPath(const std::string &relative)
    {
        std::error_code error;
        const auto exeDir = std::filesystem::read_symlink("/proc/self/exe", error).parent_path();
        if (error)
            return relative;
        const auto repoRoot = exeDir.parent_path().parent_path();
        return repoRoot / "sandbox" / "assets" / relative;
    }

} // namespace

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

    // Custom batch shader with hot reload (edit the files while running).
    // Falls back to the engine's internal shader if the files are missing.
    Shader shader;
    const bool shaderLoaded =
        shader.loadFromFile(assetPath("shaders/batch.vert"), assetPath("shaders/batch.frag"));
    if (shaderLoaded)
        Renderer::setShader(&shader);
    else
        NBX_LOG_WARN("Using engine's default batch shader");

    Renderer::setViewport(static_cast<int>(window.width()), static_cast<int>(window.height()));

    Camera2D camera(window.width(), window.height());
    camera.setPosition({kWorldTiles * kTileSize * 0.5f, kWorldTiles * kTileSize * 0.5f});

    // Do not move the atlas after this point (sprites store texture ids, but keep it simple).
    TextureAtlas atlas;
    atlas.create(makeTileAtlasTexture(), kAtlasCells, kAtlasCells);

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

    window.setEventCallback([&](const Event &event)
                            {
        if (event.type == Event::Type::Quit) {
            loop.stop();
        } else if (event.type == Event::Type::WindowResized) {
            Renderer::setViewport(event.data1, event.data2);
            camera.setViewportSize(static_cast<uint32_t>(event.data1),
                                   static_cast<uint32_t>(event.data2));
        } });

    math::vec2 playerPosition(64.0f, 64.0f);
    math::vec2 playerPrevious = playerPosition;
    float playerRotation = 0.0f;
    constexpr float kPlayerSpeed = 320.0f;
    constexpr float kWorldSize = kWorldTiles * kTileSize;

    loop.setFixedUpdateFn([&](double fixedDt)
                          {
        const float dt = static_cast<float>(fixedDt);
        playerPrevious = playerPosition;

        float dx = (Input::isActionDown("MoveRight") ? 1.0f : 0.0f) -
                   (Input::isActionDown("MoveLeft") ? 1.0f : 0.0f);
        float dy = (Input::isActionDown("MoveDown") ? 1.0f : 0.0f) -
                   (Input::isActionDown("MoveUp") ? 1.0f : 0.0f);

        math::vec2 direction(dx, dy);
        const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0.0f) {
            direction = direction * (1.0f / length);
            playerRotation = std::atan2(direction.y, direction.x);
        }
        playerPosition += direction * kPlayerSpeed * dt;
        playerPosition.x = std::clamp(playerPosition.x, 32.0f, kWorldSize - 32.0f);
        playerPosition.y = std::clamp(playerPosition.y, 32.0f, kWorldSize - 32.0f); });

    bool cullingEnabled = true;
    if (const char *culling = std::getenv("NEBRIX_CULLING"))
        cullingEnabled = std::string_view(culling) != "0";
    float targetZoom = 1.0f;
    uint64_t frameCounter = 0;

    loop.setUpdateFn([&](double dt)
                     {
        window.pollEvents();
        if (shader.update(dt)) {
            // Shader reloaded; Renderer keeps using the same Shader object.
            NBX_LOG_INFO("Shader hot-reloaded");
        }

        static bool cWasDown = false;
        const bool cDown = Input::isActionDown("ToggleCulling");
        if (cDown && !cWasDown)
            cullingEnabled = !cullingEnabled;
        cWasDown = cDown;

        if (Input::isActionDown("ZoomIn"))
            targetZoom = std::clamp(targetZoom * 1.02f, 0.25f, 4.0f);
        if (Input::isActionDown("ZoomOut"))
            targetZoom = std::clamp(targetZoom / 1.02f, 0.25f, 4.0f);

        camera.setZoom(camera.zoom() + (targetZoom - camera.zoom()) * 0.1f);
        camera.follow(playerPosition, 8.0f, dt); });

    loop.setRenderFn([&](const FrameStats &stats)
                     {
        Renderer::clear({0.09f, 0.10f, 0.13f, 1.0f});
        Renderer::beginFrame(camera.viewProjection());

        const math::vec2 renderPlayer =
            math::lerp(playerPrevious, playerPosition, static_cast<float>(stats.interpolation));

        for (uint32_t ty = 0; ty < kWorldTiles; ++ty) {
            for (uint32_t tx = 0; tx < kWorldTiles; ++tx) {
                const float x = tx * kTileSize;
                const float y = ty * kTileSize;
                if (cullingEnabled && !camera.isVisible({x, y}, {x + kTileSize, y + kTileSize}))
                    continue;

                const Sprite sprite = atlas.cell((tx + ty) % (kAtlasCells * kAtlasCells));
                Renderer::drawQuad({.position = {x + kTileSize * 0.5f, y + kTileSize * 0.5f},
                                    .scale = {kTileSize, kTileSize}},
                                   sprite);
            }
        }

        const Sprite playerSprite = atlas.cell(12);
        Renderer::drawQuad({.position = renderPlayer,
                            .rotation = playerRotation,
                            .scale = {48.0f, 48.0f}},
                           playerSprite);

        Renderer::endFrame();
        window.swapBuffers();

        if (++frameCounter % 60 == 0) {
            const Renderer::Stats& renderStats = Renderer::stats();
            NBX_LOG_INFO("fps={} quads={} drawCalls={} culling={}", stats.fps,
                         renderStats.quadCount, renderStats.drawCalls, cullingEnabled);
        } });

    NBX_LOG_INFO("Starting game loop (move: WASD, zoom: +/- , culling: C)");
    loop.run();
    NBX_LOG_INFO("Game loop ended");

    Renderer::shutdown();
    shader = Shader{};
    atlas = TextureAtlas{};
    window.shutdown();
    return 0;
}