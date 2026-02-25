#pragma once
#include <cstdarg>
#include <cstdint>

// --- Build-time options (override via -D) ---
#ifndef LOG_LEVEL
// 0=OFF, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=TRACE
#define LOG_LEVEL 5
#endif
#ifndef LOG_BUF_SIZE
#define LOG_BUF_SIZE 256
#endif
#ifndef LOG_USE_COLOR
#define LOG_USE_COLOR 0
#endif

// Weak hook: override this in your project to return ms tick etc.
extern "C" uint32_t    log_timestamp_ms();
extern "C" const char *log_cpu_tag();

class Logger
{
public:
    enum Level : uint8_t
    {
        OFF   = 0,
        ERROR = 1,
        WARN  = 2,
        INFO  = 3,
        DEBUG = 4,
        TRACE = 5
    };

    static Logger &get();

    void  setLevel(Level lvl) { level_ = lvl; }
    Level level() const { return level_; }

    // printf-style logging with source info
    void log(Level lvl, const char *file, int line, const char *func, const char *fmt, ...)
        __attribute__((format(printf, 6, 7)));

    // RAII scope tracer: prints on enter/leave
    class Scope
    {
    public:
        Scope(const char *file, int line, const char *func, const char *name = nullptr);
        ~Scope();

    private:
        const char *file_;
        const char *func_;
        int         line_;
    };

private:
    Logger() = default;
    void               vprint(Level lvl, const char *file, int line, const char *func, const char *fmt, va_list ap);
    static const char *lvlTag(Level lvl);
    static const char *lvlColor(Level lvl);

private:
    Level level_ = (Level)LOG_LEVEL;
};

// ---- Convenience macros ----
#define LOGE(fmt, ...)                                                                                                 \
    do {                                                                                                               \
        if (LOG_LEVEL >= 1)                                                                                            \
            Logger::get().log(Logger::ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);                        \
    } while (0)
#define LOGW(fmt, ...)                                                                                                 \
    do {                                                                                                               \
        if (LOG_LEVEL >= 2)                                                                                            \
            Logger::get().log(Logger::WARN, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);                         \
    } while (0)
#define LOGI(fmt, ...)                                                                                                 \
    do {                                                                                                               \
        if (LOG_LEVEL >= 3)                                                                                            \
            Logger::get().log(Logger::INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);                         \
    } while (0)
#define LOGD(fmt, ...)                                                                                                 \
    do {                                                                                                               \
        if (LOG_LEVEL >= 4)                                                                                            \
            Logger::get().log(Logger::DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);                        \
    } while (0)
#define LOGT(fmt, ...)                                                                                                 \
    do {                                                                                                               \
        if (LOG_LEVEL >= 5)                                                                                            \
            Logger::get().log(Logger::TRACE, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);                        \
    } while (0)

// Scope trace macros
#define LOG_SCOPE()       Logger::Scope _log_scope(__FILE__, __LINE__, __func__, nullptr)
#define LOG_SCOPE_N(name) Logger::Scope _log_scope(__FILE__, __LINE__, __func__, name)
