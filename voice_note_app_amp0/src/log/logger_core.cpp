#include "logger_core.h"
#include "xil_printf.h"
#include <cstdio>
#include <cstring>

extern "C" __attribute__((weak)) uint32_t    log_timestamp_ms() { return 0u; }
extern "C" __attribute__((weak)) const char *log_cpu_tag() { return "cpu0"; }

static void console_write(const char *s) { xil_printf("%s", s); }

Logger &Logger::get()
{
    static Logger inst;
    return inst;
}

const char *Logger::lvlTag(Level lvl)
{
    switch (lvl) {
        case ERROR:
            return "E";
        case WARN:
            return "W";
        case INFO:
            return "I";
        case DEBUG:
            return "D";
        case TRACE:
            return "T";
        default:
            return "?";
    }
}

const char *Logger::lvlColor(Level lvl)
{
#if LOG_USE_COLOR
    switch (lvl) {
        case ERROR:
            return "\x1b[31m";  // red
        case WARN:
            return "\x1b[33m";  // yellow
        case INFO:
            return "\x1b[32m";  // green
        case DEBUG:
            return "\x1b[36m";  // cyan
        case TRACE:
            return "\x1b[90m";  // gray
        default:
            return "\x1b[0m";
    }
#else
    (void)lvl;
    return "";
#endif
}

void Logger::vprint(Level lvl, const char *file, int line, const char *func, const char *fmt, va_list ap)
{
    if (lvl > level_) {
        return;
    }

    char buf[LOG_BUF_SIZE];
    int  n = 0;

#if LOG_USE_COLOR
    n += snprintf(buf + n, sizeof(buf) - n, "%s", lvlColor(lvl));
#endif
    n += snprintf(buf + n, sizeof(buf) - n, "[%s][%s][%s:%d %s] ", log_cpu_tag(), lvlTag(lvl), file, line, func);
    if (n < 0 || n >= (int)sizeof(buf)) {
        buf[sizeof(buf) - 1] = '\0';
        console_write(buf);
        return;
    }

    int m = vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
    if (m < 0) {
        m = 0;
    }
    n += (m >= (int)sizeof(buf) - n) ? (int)sizeof(buf) - n - 1 : m;

#if LOG_USE_COLOR
    n += snprintf(buf + n, sizeof(buf) - n, "\x1b[0m");
#endif
    if (n < (int)sizeof(buf) - 1 && buf[n - 1] != '\n') {
        buf[n++] = '\n';
        buf[n]   = '\0';
    }

    console_write(buf);
}

void Logger::log(Level lvl, const char *file, int line, const char *func, const char *fmt, ...)
{
    if (lvl > level_) {
        return;
    }

    // ファイル名抽出（パスは捨てる）
    const char *fname = std::strrchr(file, '/');
    if (!fname) {
        fname = std::strrchr(file, '\\');
    }
    fname = fname ? fname + 1 : file;

    va_list ap;
    va_start(ap, fmt);
    vprint(lvl, fname, line, func, fmt, ap);
    va_end(ap);
}

Logger::Scope::Scope(const char *file, int line, const char *func, const char *name)
    : file_(file), func_(func), line_(line)
{
#if LOG_LEVEL >= 5
    // ファイル名抽出（パスは捨てる）
    const char *fname = std::strrchr(file_, '/');
    if (!fname) {
        fname = std::strrchr(file_, '\\');
    }
    fname = fname ? fname + 1 : file_;

    Logger::get().log(Logger::TRACE, fname, line_, func_, "<enter>%s", name ? name : "");
#else
    (void)name;
#endif
}

Logger::Scope::~Scope()
{
#if LOG_LEVEL >= 5
    Logger::get().log(Logger::TRACE, file_, line_, func_, "<leave>");
#endif
}
