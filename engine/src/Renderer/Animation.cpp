#include "Nebrix/Renderer/Animation.h"

#include "Nebrix/ECS/Components.h"
#include "Nebrix/ECS/World.h"

#include <cmath>
#include <cstddef>

namespace nbx
{

    void AnimationSystem::update(World &world, float dt)
    {
        for (auto [entity, animation, sprite] : world.view<Animation, Sprite>())
        {
            (void)entity;
            if (animation.frames.empty() || animation.fps <= 0.0f)
                continue;

            const size_t count = animation.frames.size();
            if (animation.playing)
            {
                animation.timer += dt;
                if (animation.loop)
                {
                    // Keep the timer bounded so float precision never degrades
                    // over long sessions.
                    const float cycle = static_cast<float>(count) / animation.fps;
                    if (cycle > 0.0f)
                        animation.timer = std::fmod(animation.timer, cycle);
                }
                size_t next = static_cast<size_t>(animation.timer * animation.fps);
                if (next >= count)
                    next = count - 1;
                animation.index = next;
            }
            else if (animation.index >= count)
            {
                animation.index = count - 1;
            }

            sprite = animation.frames[animation.index];
        }
    }

} // namespace nbx
