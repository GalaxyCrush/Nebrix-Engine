#pragma once

namespace nbx
{

    class World;

    // Advances every Animation component and writes the current frame into the
    // entity's Sprite. Call once per fixed step, after gameplay sets `playing`
    // and before rendering. Entities without a Sprite are skipped.
    class AnimationSystem
    {
    public:
        void update(World &world, float dt);
    };

} // namespace nbx
