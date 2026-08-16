#pragma once

#include <cstdint>
#include <format>
#include <source_location>
#include <string>
#include <string_view>

namespace nbx
{

    enum class LogLevel : uint8_t
    {
        Trace = 0,
        Info,
        Warn,
        Error,
    };

    class Log
    {
    public:
        static void setLevel(LogLevel level);
        static void write(LogLevel level, const std::source_location &location, std::string message);
    };

} // namespace nbx

#define NBX_LOG_TRACE(...)                                                     \
    ::nbx::Log::write(::nbx::LogLevel::Trace, std::source_location::current(), \
                      std::format(__VA_ARGS__))
#define NBX_LOG_INFO(...)                                                     \
    ::nbx::Log::write(::nbx::LogLevel::Info, std::source_location::current(), \
                      std::format(__VA_ARGS__))
#define NBX_LOG_WARN(...)                                                     \
    ::nbx::Log::write(::nbx::LogLevel::Warn, std::source_location::current(), \
                      std::format(__VA_ARGS__))
#define NBX_LOG_ERROR(...)                                                     \
    ::nbx::Log::write(::nbx::LogLevel::Error, std::source_location::current(), \
                      std::format(__VA_ARGS__))
