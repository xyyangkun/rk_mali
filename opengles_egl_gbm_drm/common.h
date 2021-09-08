#ifndef EGL_GBM_COMMON_H
#define EGL_GBM_COMMON_H
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vt.h>
#include <unistd.h>

#include <gbm.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>
//#include <GLES3/gl3ext.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

struct framebuffer {
    int fd;
    uint32_t width, height;
    uint32_t id;
};

struct gbm {
    struct gbm_surface *surface;
    struct gbm_device *device;
    struct gbm_bo *bo;
    struct gbm_bo *next_bo;
};

struct egl {
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
};

struct global_ctx_s {
    struct mp_log *log;
    struct kms *kms;
    struct egl egl;
    struct gbm gbm;
    struct framebuffer *fb;

    GLuint programObject;
    uint32_t primary_plane_format;
    bool active;
    bool waiting_for_flip;

    drmEventContext ev;
    drmModeCrtc *old_crtc;
};

#endif
