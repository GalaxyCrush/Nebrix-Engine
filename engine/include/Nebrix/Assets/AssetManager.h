#pragma once

#include "Nebrix/Renderer/Shader.h"
#include "Nebrix/Renderer/Texture.h"

#include <filesystem>
#include <string>
#include <unordered_map>

namespace nbx
{

    // Central asset cache. Resolves paths against a project root derived from
    // /proc/self/exe (the executable lives at <repo>/build*/sandbox/, so the
    // root is <repo>/sandbox/assets), regardless of the current working
    // directory. Textures are cached per relative name and never unloaded
    // until shutdown(); shaders are cached per logical name and keep hot
    // reload alive.
    //
    // A missing texture file logs an error once and returns a 1x1 magenta
    // fallback texture (cached under the requested name), so rendering keeps
    // working while the problem stays visible in the log.
    class AssetManager
    {
    public:
        // Resolves the project root. Safe to call more than once; later calls
        // are no-ops. Must be called after Window::init (GL context exists)
        // and before the first get* query.
        static bool init();

        // Releases every cached GL resource. Call before the GL context dies.
        static void shutdown();

        // Absolute path for a relative asset name ("textures/tileset.png").
        static std::string assetPath(const std::string &relative);

        // Loads (or returns the cached) texture. Never returns null: missing
        // files yield the cached magenta fallback.
        static const Texture &getTexture(const std::string &relative);

        // Loads (or returns the cached) shader pair. Returns nullptr when
        // compilation fails (also cached, so retries don't spam the log);
        // callers are expected to fall back to the renderer's default shader.
        static Shader *getShader(const std::string &name, const std::string &vertexRelative,
                                 const std::string &fragmentRelative);

    private:
        static const Texture &magentaFallback();

        static std::filesystem::path s_root;
        static bool s_initialized;
        static std::unordered_map<std::string, Texture> s_textures;
        static std::unordered_map<std::string, std::unique_ptr<Shader>> s_shaders;
    };

} // namespace nbx
