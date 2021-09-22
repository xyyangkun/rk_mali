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

int main() 
{

#if 0
	int w= 1080;
	int h = 1920;
#else
	int w= 1920;
	int h = 1080;
#endif

    if (rga_control_buffer_init(&g_bo, &g_fd, w, h, 16)) {
        printf("%s: %d exit!\n", __func__, __LINE__);
        return -1;
    }

	display_init(w, h);

	while(1)
	{
		memset(g_bo.ptr, 0xff, 1920*1080);
		display_commit(g_bo.ptr, g_fd, RK_FORMAT_YCbCr_420_SP, 
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
