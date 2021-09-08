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

	print_fps("usb recv");
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
#if 1
	copy_buffer(ptr, 1920, 1088, RK_FORMAT_YCbCr_422_SP, 
				_ptr, 1920, 1080, RK_FORMAT_BGRA_8888, 0); 
				//_ptr, 1920, 1080, RK_FORMAT_RGBX_8888, 0); 
				//_ptr, 1920, 1080, RK_FORMAT_BGR_888, 0); 
#else
	memset(_ptr, 0xff, 1920*1080);
	memset(_ptr+1920*1080, 0x1f, 1920*1080);
	memset(_ptr+1920*1080*2, 0xff, 1920*1080);
	memset(_ptr+1920*1080*3, 0xff, 1920*1080);
#endif
#endif
}

int main() {
	printf("ok\n");
	
	int width = 1920;
	int height = 1080;

#ifdef  DRMDISPLAY
	// hdmi
	int out_type = 0;

	initDrmDsp(out_type);

	// hdmi 是横屏 正常的1920x1080
	init_display_mem(width, height, out_type, 1);
#endif
	// 设置usb参数
	set_usb_param(width, height, _display);


	init_queue(width, height);

	// 初始化usb  
	usb_camera_init(_video_cb);

	while(1) sleep(1);
}
