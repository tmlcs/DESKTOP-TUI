#ifndef TUI_CORE_LOGGER_HPP
#define TUI_CORE_LOGGER_HPP

#include <cstdio>
#include <cstdarg>
#include <chrono>
#include <string>

namespace tui {

/// Log levels
enum class LogLevel : int {
    DEBUG = 0,
    INFO  = 1,
    WARN  = 2,
    ERROR = 3,
    OFF   = 4
};

/// Compile-time log level filter — set to OFF to strip all logging at compile time
#ifndef TUI_LOG_LEVEL
#define TUI_LOG_LEVEL LogLevel::DEBUG
#endif

/// Minimal header-only logger
/// Usage: TUI_LOG(LogLevel::INFO, "Window resized to %dx%d", cols, rows);
#define TUI_LOG(level, ...) \
    ::tui::Logger::log_impl(level, __FILE__, __LINE__, __VA_ARGS__)

class Logger {
public:
    /// Set the runtime log level (default: INFO)
    static void set_level(LogLevel level) { instance().level_ = level; }
    static LogLevel level() { return instance().level_; }

    /// Log a message with format string and variadic arguments
    static void log_impl(LogLevel level, const char* file, int line, const char* fmt, ...) {
        if (level < TUI_LOG_LEVEL) return;
        if (level < instance().level_) return;

        const char* prefix = level_prefix(level);
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count() % 1000;
        auto t = std::chrono::system_clock::to_time_t(now);

        // Extract filename from path
        const char* short_file = file;
        for (const char* p = file; *p; p++) {
            if (*p == '/' || *p == '\\') short_file = p + 1;
        }

        char time_buf[32];
#ifndef _WIN32
        struct tm tm_buf;
        localtime_r(&t, &tm_buf);
        std::snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d.%03ld",
                      tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, (long)ms % 1000);
#else
        struct tm* tm_buf = localtime(&t);
        std::snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d.%03ld",
                      tm_buf->tm_hour, tm_buf->tm_min, tm_buf->tm_sec, (long)ms % 1000);
#endif

        std::fprintf(stderr, "[%s] [%s] %s:%d: ", time_buf, prefix, short_file, line);

        va_list args;
        va_start(args, fmt);
        std::vfprintf(stderr, fmt, args);
        va_end(args);

        std::fputc('\n', stderr);
        std::fflush(stderr);
    }

private:
    static const char* level_prefix(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default:              return "???";
        }
    }

    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    LogLevel level_ = LogLevel::INFO;
};

} // namespace tui

#endif // TUI_CORE_LOGGER_HPP
