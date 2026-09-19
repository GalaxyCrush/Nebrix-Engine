#pragma once

#include "Nebrix/Core/Log.h"

#include <cstdlib>
#include <format>
#include <source_location>
#include <string>
#include <string_view>

namespace nbx::detail
{

    [[noreturn]] inline void assertFail(std::string_view expression, std::string_view message,
                                        const std::source_location &location)
    {
        Log::write(LogLevel::Error, location, "Assertion failed: {}", expression);
        if (!message.empty())
            Log::write(LogLevel::Error, location, "  Reason: {}", message);
        std::abort();
    }

    // Message builder for NBX_ASSERT: the zero-argument overload makes the
    // message optional (NBX_ASSERT(cond) is valid); any other call treats its
    // first argument as a std::format-style format string. Compile-time format
    // checking is traded for this flexibility.
    inline std::string assertMessage() { return {}; }

    template <typename... Args>
    std::string assertMessage(std::string_view fmt, const Args &...args)
    {
        return std::vformat(fmt, std::make_format_args(args...));
    }

} // namespace nbx::detail

#define NBX_ASSERT(condition, ...)                                               \
    do                                                                           \
    {                                                                            \
        if (!(condition))                                                        \
            ::nbx::detail::assertFail(#condition,                                \
                                      ::nbx::detail::assertMessage(__VA_ARGS__), \
                                      std::source_location::current());          \
    } while (false)

#define NBX_UNREACHABLE()                                                    \
    do                                                                       \
    {                                                                        \
        ::nbx::detail::assertFail("reached unreachable code", "Unreachable", \
                                  std::source_location::current());          \
    } while (false)
