#ifndef EGL_GBM_EGL_H
#define EGL_GBM_EGL_H
#include "common.h"

bool init_egl(struct global_ctx_s *ctx);
int egl_prepare(struct global_ctx_s *gctx);
void egl_draw(struct global_ctx_s *gctx);
#endif
