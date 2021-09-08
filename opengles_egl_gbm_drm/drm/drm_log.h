#ifndef DRM_LOG_H
#define DRM_LOG_H

#include <stdbool.h>
#include <stdarg.h>

struct mp_log {
    char name[32];
};


// Verbosity levels.
enum {
    MSGL_FATAL,     // will exit/abort (note: msg.c doesn't exit or abort)
    MSGL_ERR,       // continues
    MSGL_WARN,      // only warning
    MSGL_INFO,      // -quiet
    MSGL_STATUS,    // exclusively for the playback status line
    MSGL_V,         // -v
    MSGL_DEBUG,     // -v -v
    MSGL_TRACE,     // -v -v -v
    MSGL_STATS,     // dumping fine grained stats (--dump-stats)

    MSGL_MAX = MSGL_STATS,
};

struct mp_log *mp_log_new(void *talloc_ctx, struct mp_log *parent,
                          const char *name);

void mp_msg(struct mp_log *log, int lev, const char *format, ...);
void mp_msg_va(struct mp_log *log, int lev, const char *format, va_list va);

bool mp_msg_test(struct mp_log *log, int lev);
const char *mp_strerror(int errno);

// Convenience macros.
#define mp_fatal(log, ...)      mp_msg(log, MSGL_FATAL, __VA_ARGS__)
#define mp_err(log, ...)        mp_msg(log, MSGL_ERR, __VA_ARGS__)
#define mp_warn(log, ...)       mp_msg(log, MSGL_WARN, __VA_ARGS__)
#define mp_info(log, ...)       mp_msg(log, MSGL_INFO, __VA_ARGS__)
#define mp_verbose(log, ...)    mp_msg(log, MSGL_V, __VA_ARGS__)
#define mp_dbg(log, ...)        mp_msg(log, MSGL_DEBUG, __VA_ARGS__)
#define mp_trace(log, ...)      mp_msg(log, MSGL_TRACE, __VA_ARGS__)

// Convenience macros, typically called with a pointer to a context struct
// as first argument, which has a "struct mp_log *log;" member.

#define MP_MSG(obj, lev, ...)   mp_msg((obj)->log, lev, __VA_ARGS__)

#define MP_FATAL(obj, ...)      MP_MSG(obj, MSGL_FATAL, __VA_ARGS__)
#define MP_ERR(obj, ...)        MP_MSG(obj, MSGL_ERR, __VA_ARGS__)
#define MP_WARN(obj, ...)       MP_MSG(obj, MSGL_WARN, __VA_ARGS__)
#define MP_INFO(obj, ...)       MP_MSG(obj, MSGL_INFO, __VA_ARGS__)
#define MP_VERBOSE(obj, ...)    MP_MSG(obj, MSGL_V, __VA_ARGS__)
#define MP_DBG(obj, ...)        MP_MSG(obj, MSGL_DEBUG, __VA_ARGS__)
#define MP_TRACE(obj, ...)      MP_MSG(obj, MSGL_TRACE, __VA_ARGS__)

// This is a bit special. See TOOLS/stats-conv.py what rules text passed
// to these functions should follow. Also see --dump-stats.
#define mp_stats(obj, ...)      mp_msg(obj, MSGL_STATS, __VA_ARGS__)
#define MP_STATS(obj, ...)      MP_MSG(obj, MSGL_STATS, __VA_ARGS__)

#endif
