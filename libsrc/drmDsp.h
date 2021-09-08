#ifndef __DRM_DSP_H__
#define __DRM_DSP_H__
#ifdef __cplusplus

extern "C"
{
#endif

#include <drm_fourcc.h>

int initDrmDsp(int out_type);
int drmDspFrame(int srcWidth, int srcHeight, int dispWidth, int dispHeight,
		void* dmaFd, int fmt);
int drmDspFrame_rgb(int srcWidth, int srcHeight, int dispWidth, int dispHeight,
		void* dmaFd, int fmt);
void deInitDrmDsp();

int display_get_mem(void **ptr, int *fd);
void display_fush();
int init_display_mem(int dispWidth, int dispHeight, int out_type, int buf_type);

#ifdef __cplusplus
}
#endif

#endif

