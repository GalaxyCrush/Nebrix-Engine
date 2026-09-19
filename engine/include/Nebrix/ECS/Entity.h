#pragma once

#include <cstdint>
#include <limits>

namespace nbx
{

    // Lightweight entity handle: index into the World's entity arrays + a generation
    // counter that invalidates stale handles after destroy/reuse.
    struct Entity
    {
        uint32_t index = std::numeric_limits<uint32_t>::max();
        uint32_t generation = 0;

        bool valid() const { return index != std::numeric_limits<uint32_t>::max(); }
        explicit operator bool() const { return valid(); }
        bool operator==(const Entity &) const = default;
    };

} // namespace nbx