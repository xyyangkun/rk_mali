#include "drm_log.h"
#include <stdio.h>
#include <stdlib.h>


struct mp_log *mp_log_new(void *talloc_ctx, struct mp_log *parent,
                          const char *name) {
    (void)talloc_ctx;
    (void)parent;

    return calloc(1, sizeof(struct mp_log));
}

void mp_msg_va(struct mp_log *log, int lev, const char *format, va_list va)
{
    (void)log;
    (void)lev;
    vfprintf(stdout, format, va);
}

// Thread-safety: fully thread-safe, but keep in mind that the lifetime of
//                log must be guaranteed during the call.
//                Never call this from signal handlers.
void mp_msg(struct mp_log *log, int lev, const char *format, ...)
{
    va_list va;
    va_start(va, format);
    mp_msg_va(log, lev, format, va);
    va_end(va);
}

const char *mp_strerror(int errno) {
    (void)errno;
    return "";
}
