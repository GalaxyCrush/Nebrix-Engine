#pragma once

#include "Nebrix/Core/GameLoop.h"

#include <cstdint>

namespace nbx
{

    // A self-contained game state (menu, gameplay, pause...). Scenes own
    // everything they need (world, camera, entities) and are driven by the
    // SceneManager: onEnter once when they become current, onExit once when
    // replaced, onResize on window changes.
    class Scene
    {
    public:
        virtual ~Scene() = default;

        virtual void onEnter() {}
        virtual void onExit() {}
        virtual void onResize(uint32_t width, uint32_t height)
        {
            (void)width;
            (void)height;
        }

        virtual void fixedUpdate(float dt) { (void)dt; }
        virtual void update(double dt) { (void)dt; }
        virtual void render(const FrameStats &stats) { (void)stats; }
    };

} // namespace nbx
