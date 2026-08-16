#pragma once

#include "Nebrix/Core/Log.h"

#include <cstdlib>
#include <format>
#include <source_location>
#include <string_view>

namespace nbx::detail
{

    [[noreturn]] inline void assertFail(std::string_view expression, std::string_view message,
                                        const std::source_location &location)
    {
        Log::write(LogLevel::Error, location, std::format("Assertion failed: {}", expression));
        if (!message.empty())
            Log::write(LogLevel::Error, location, std::format("  Reason: {}", message));
        std::abort();
    }

} // namespace nbx::detail

#define NBX_ASSERT(condition, ...)                                          \
    do                                                                      \
    {                                                                       \
        if (!(condition))                                                   \
            ::nbx::detail::assertFail(#condition, std::format(__VA_ARGS__), \
                                      std::source_location::current());     \
    } while (false)

#define NBX_UNREACHABLE()                                                    \
    do                                                                       \
    {                                                                        \
        ::nbx::detail::assertFail("reached unreachable code", "Unreachable", \
                                  std::source_location::current());          \
    } while (false)
