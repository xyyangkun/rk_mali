/******************************************************************************
 *
 *       Filename:  test_usb.c
 *
 *    Description:  test usb  interface ,recv usb data
 *
 *        Version:  1.0
 *        Created:  2021年08月19日 10时00分03秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include "my_usb_camera.h"
#include "myutils.h"

#define  DRMDISPLAY

#ifdef  DRMDISPLAY
#include "drmDsp.h"
#include "drm_fourcc.h"
#include "rkRgaApi.h"
#include "rga_control.h"
#include <rga/RgaApi.h> 
#endif

static void _display(void *ptr, int fd, int fmt, int w, int h, int rotation) {

	// printf("display:\n");
	print_fps("usb recv");
	//drmDspFrame(w, h, w, h, ptr, DRM_FORMAT_NV12);
	//display_fush();
}

static void copy_buffer(void *src_p, int src_w, int src_h, int src_fmt,
                         void *dst_p, int dst_w, int dst_h, int dst_fmt,
                         int rotation) {
  rga_info_t src, dst;
  memset(&src, 0, sizeof(rga_info_t));
  src.fd = -1;
  src.virAddr = src_p;
  src.mmuFlag = 1;
  src.rotation = rotation;
  rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, src_fmt);
  memset(&dst, 0, sizeof(rga_info_t));
  dst.fd = -1;
  dst.virAddr = dst_p;
  dst.mmuFlag = 1;
  rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dst_fmt);
  if (c_RkRgaBlit(&src, &dst, NULL))
    printf("%s: rga fail\n", __func__);
}

static void _video_cb(void *ptr, int buf_type){
	print_fps("1virtual display fps");
#ifdef  DRMDISPLAY
	int w = 1920;
	int h = 1088;
	int _fd; 
	void *_ptr;

	display_get_mem(&_ptr, &_fd); 
	// 竖屏需要旋转
	copy_buffer(ptr, 1920, 1088, RK_FORMAT_YCbCr_422_SP, 
				_ptr, 1080, 1920, RK_FORMAT_BGRA_8888, HAL_TRANSFORM_ROT_90); 
#endif
}

// 这个程序打开两个图层一个Primary，一个overlay, 
// 将primary 设置为顶层时，开启alpha,并设置为0, 顶层将透过去
// 次程序的primary 层的数据只设置了一次
// initDrmDsp->initialize_screens初始化argb数据
//
int main() {
	printf("ok\n");
	
	int width = 1920;
	int height = 1080;

#ifdef  DRMDISPLAY
	// mipi
	int out_type = 1;

	initDrmDsp(out_type);

	// mipi 是竖屏1080x1920
	init_display_mem(height, width, out_type, 1);
#endif
	// 设置usb参数
	set_usb_param(width, height, _display);


	init_queue(width, height);

	// 初始化usb  
	usb_camera_init(_video_cb);

	while(1) sleep(1);
}
