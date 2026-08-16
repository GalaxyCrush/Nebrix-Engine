#include "Nebrix/Core/Log.h"

#include <cstdio>

namespace nbx {

namespace {

LogLevel s_level = LogLevel::Info;

const char* levelName(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "Trace";
        case LogLevel::Info: return "Info";
        case LogLevel::Warn: return "Warn";
        case LogLevel::Error: return "Error";
    }
    return "?";
}

const char* levelColor(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "\033[90m";
        case LogLevel::Info: return "\033[0m";
        case LogLevel::Warn: return "\033[33m";
        case LogLevel::Error: return "\033[31m";
    }
    return "\033[0m";
}

} // namespace

void Log::setLevel(LogLevel level) { s_level = level; }

void Log::write(LogLevel level, const std::source_location& location, std::string message) {
    if (level < s_level)
        return;

    std::string_view file = location.file_name();
    if (const auto pos = file.find_last_of('/'); pos != std::string_view::npos)
        file = file.substr(pos + 1);

    std::fprintf(stderr, "%s[Nebrix][%-5s] %s\033[0m (%s:%u)\n", levelColor(level),
                 levelName(level), message.c_str(), file.data(), location.line());
}

} // namespace nbx
