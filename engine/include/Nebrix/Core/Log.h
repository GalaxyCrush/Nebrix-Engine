#pragma once

#include <atomic>
#include <cstdint>
#include <format>
#include <source_location>
#include <string>

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
        static LogLevel level();
        static void setLevel(LogLevel level);

        // Variadic entry point: the message is formatted only when the level
        // passes the filter, so filtered-out calls skip std::format entirely.
        // The level is read atomically (thread-safe with concurrent writers).
        template <typename... Args>
        static void write(LogLevel level, const std::source_location &location,
                          std::format_string<Args...> fmt, Args &&...args)
        {
            if (level < currentLevel())
                return;
            writeFormatted(level, location, std::format(fmt, std::forward<Args>(args)...));
        }

    private:
        static void writeFormatted(LogLevel level, const std::source_location &location,
                                   std::string &&message);
        static std::atomic<LogLevel> &levelStorage();
        static LogLevel currentLevel() { return levelStorage().load(std::memory_order_relaxed); }
    };

} // namespace nbx

#define NBX_LOG_TRACE(...)                                                     \
    ::nbx::Log::write(::nbx::LogLevel::Trace, std::source_location::current(), \
                      __VA_ARGS__)
#define NBX_LOG_INFO(...)                                                      \
    ::nbx::Log::write(::nbx::LogLevel::Info, std::source_location::current(),  \
                      __VA_ARGS__)
#define NBX_LOG_WARN(...)                                                      \
    ::nbx::Log::write(::nbx::LogLevel::Warn, std::source_location::current(),  \
                      __VA_ARGS__)
#define NBX_LOG_ERROR(...)                                                     \
    ::nbx::Log::write(::nbx::LogLevel::Error, std::source_location::current(), \
                      __VA_ARGS__)
