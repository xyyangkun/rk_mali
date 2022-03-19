/******************************************************************************
 *
 *       Filename:  drm_out.c
 *
 *    Description:  test
 *
 *        Version:  1.0
 *        Created:  2021年09月07日 16时19分50秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <stdio.h>
#include "display.h"

//#include "drmDsp.h"
#include "drm_fourcc.h"
#include "rkRgaApi.h"
#include "rga_control.h"
#include <rga/RgaApi.h> 

static bo_t g_bo;
static int g_fd = -1;

// 改为argb输出
int main() 
{

#if 0
	int w= 1080;
	int h = 1920;
#else
	int w= 1920;
	int h = 1080;
#endif

    //if (rga_control_buffer_init(&g_bo, &g_fd, w, h, 16)) {
    if (rga_control_buffer_init(&g_bo, &g_fd, w, h, 32)) {
        printf("%s: %d exit!\n", __func__, __LINE__);
        return -1;
    }

	display_init(w, h);

	while(1)
	{
#if 0
		memset(g_bo.ptr + 0 * 1920*1080, 0xff, 1920*1080);
		memset(g_bo.ptr + 1 * 1920*1080, 0xff, 1920*1080);
		memset(g_bo.ptr + 2 * 1920*1080, 0xff, 1920*1080);
		memset(g_bo.ptr + 3 * 1920*1080, 0x00, 1920*1080);
#else
		unsigned char b = 255;
		unsigned char g = 255;
		unsigned char r = 255;
		unsigned char a = 0;
		// 构造rgba内存
		for(int _h=0; _h< h; _h++)
		{
			for(int _w = 0; _w < w; _w++)
			{
				*((unsigned char *)g_bo.ptr + 4*w*_h + 4*_w + 0) = b;
				*((unsigned char *)g_bo.ptr + 4*w*_h + 4*_w + 1) = g;
				*((unsigned char *)g_bo.ptr + 4*w*_h + 4*_w + 2) = r;
				*((unsigned char *)g_bo.ptr + 4*w*_h + 4*_w + 3) = a;
			}
		}

		//memset(g_bo.ptr + 0 * 1920*1080, 0xff, 1920*1080);
			
#endif

		//display_commit(g_bo.ptr, g_fd, RK_FORMAT_YCbCr_420_SP, 
		display_commit1(g_bo.ptr, g_fd, RK_FORMAT_BGRA_8888, 
				w, h, 0);
		printf("out===========>\n");
		usleep(100*1000);
	}

	while(1)
	{
		sleep(1);
		
	}

    rga_control_buffer_deinit(&g_bo, g_fd);
}
