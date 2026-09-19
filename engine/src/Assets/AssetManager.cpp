#include "Nebrix/Assets/AssetManager.h"

#include "Nebrix/Core/Log.h"

#include <array>
#include <system_error>

namespace nbx
{

    namespace
    {

        Texture makeMagentaTexture()
        {
            // 1x1 solid magenta: impossible to miss on screen.
            Texture texture;
            constexpr std::array<uint8_t, 4> magenta = {255, 0, 255, 255};
            texture.create(1, 1, magenta.data());
            return texture;
        }

    } // namespace

    std::filesystem::path AssetManager::s_root;
    bool AssetManager::s_initialized = false;
    std::unordered_map<std::string, Texture> AssetManager::s_textures;
    std::unordered_map<std::string, std::unique_ptr<Shader>> AssetManager::s_shaders;

    bool AssetManager::init()
    {
        if (s_initialized)
            return true;

        // The sandbox executable lives at <repo>/build*/sandbox/NebrixSandbox,
        // so the assets root is two directories up + sandbox/assets.
        std::error_code error;
        const auto exeDir = std::filesystem::read_symlink("/proc/self/exe", error).parent_path();
        if (error)
        {
            NBX_LOG_ERROR("AssetManager: cannot resolve /proc/self/exe: {}", error.message());
            return false;
        }
        s_root = exeDir.parent_path().parent_path() / "sandbox" / "assets";
        s_initialized = true;
        NBX_LOG_INFO("AssetManager ready (root: '{}')", s_root.string());
        return true;
    }

    void AssetManager::shutdown()
    {
        // GL resources must die while the context is alive, so this must run
        // before Window::shutdown.
        s_shaders.clear();
        s_textures.clear();
        s_initialized = false;
        NBX_LOG_INFO("AssetManager shut down");
    }

    std::string AssetManager::assetPath(const std::string &relative)
    {
        return (s_root / relative).string();
    }

    const Texture &AssetManager::magentaFallback()
    {
        // Cached under an impossible key so it is created exactly once and
        // dies with the manager.
        constexpr const char *kKey = "\1fallback-magenta";
        const auto it = s_textures.find(kKey);
        if (it != s_textures.end())
            return it->second;
        return s_textures.emplace(kKey, makeMagentaTexture()).first->second;
    }

    const Texture &AssetManager::getTexture(const std::string &relative)
    {
        if (!s_initialized)
        {
            NBX_LOG_ERROR("AssetManager::getTexture called before init()");
            return magentaFallback();
        }

        const auto it = s_textures.find(relative);
        if (it != s_textures.end())
            return it->second; // cache hit (also covers previous failures)

        Texture texture;
        const std::string path = assetPath(relative);
        if (!texture.loadFromFile(path))
        {
            NBX_LOG_ERROR("Texture '{}' missing or unreadable; using magenta fallback", path);
            return s_textures.emplace(relative, makeMagentaTexture()).first->second;
        }

        return s_textures.emplace(relative, std::move(texture)).first->second;
    }

    Shader *AssetManager::getShader(const std::string &name, const std::string &vertexRelative,
                                    const std::string &fragmentRelative)
    {
        if (!s_initialized)
        {
            NBX_LOG_ERROR("AssetManager::getShader called before init()");
            return nullptr;
        }

        const auto it = s_shaders.find(name);
        if (it != s_shaders.end())
            return it->second.get(); // may be null: a previously failed load

        auto shader = std::make_unique<Shader>();
        if (!shader->loadFromFile(assetPath(vertexRelative), assetPath(fragmentRelative)))
        {
            NBX_LOG_ERROR("Shader '{}' failed to compile ('{}' + '{}'); cached as failed", name,
                          vertexRelative, fragmentRelative);
            s_shaders.emplace(name, nullptr); // remember the failure
            return nullptr;
        }

        return s_shaders.emplace(name, std::move(shader)).first->second.get();
    }

} // namespace nbx
