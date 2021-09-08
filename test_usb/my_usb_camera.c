/*
 * Copyright (C) 2019 Rockchip Electronics Co., Ltd.
 * author: Zhihua Wang, hogan.wang@rock-chips.com
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL), available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <stdbool.h>
#include <pthread.h>

#include "my_usb_camera.h"
#include "vpu_decode.h"
#include "rga_control.h"
//#include "rockface_control.h"
//#include "video_common.h"
//#include "rkfacial.h"
#include "rkRgaApi.h"
#include "ringqueue.h"
#include "drmDsp.h"
#include "myutils.h"

t_rq *g_disp_queue;
t_rq *g_disp_freequeue; // 空闲数据队列

#define CAMERA_NUM 34
// #define BUFFER_COUNT 4
#define BUFFER_COUNT 8

struct map_buffer {
    void *start;
    size_t length;
};

struct video_cb_param
{
	video_cb cb;
};

static int g_fd = -1;
static struct map_buffer g_map_buf[BUFFER_COUNT] = {0};
static int g_width, g_height;
static bool g_run;
static pthread_t g_th;
static struct vpu_decode g_decode;
static bo_t g_dec_bo;
static int g_dec_fd = -1;

static bool g_usb_en;
static int g_usb_width;
static int g_usb_height;
static display_callback g_display_cb = NULL;
static pthread_mutex_t g_display_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_rotation = HAL_TRANSFORM_ROT_90;


// 定义显示内存数据队列
typedef struct s_disp_mem {
	bo_t bo;
	int fd;
}t_disp_mem;

void init_queue(int width, int height) {
	int w,h;
	w = ((width-1)/16 + 1)*16;
	h = ((height-1)/16 + 1)*16;
	
	int ret;
	int size = 5;
	rqing_init(&g_disp_queue, size);
	rqing_init(&g_disp_freequeue, size);
	// 分批数据放到空闲队列中
	for(int i=0; i< size; i++) {
		t_disp_mem *dmem = (t_disp_mem *)malloc(sizeof(t_disp_mem));
		if (rga_control_buffer_init(&dmem->bo, &dmem->fd, w, h, 32)) {
			printf("%s: %d exit!\n", __func__, __LINE__);
			assert(0);
			return ;
		}
		printf("-> dmem:%p, ptr:%p, fd:%d\n", dmem, dmem->bo.ptr, dmem->fd);
		//在操作完成前，队列不可能满
		assert(!rqing_isfull(g_disp_freequeue));
		rqing_push(g_disp_freequeue, (void*)dmem);
	}
	
}
void deinit_queue() {
	int ret;
	// 清空队列
	while(!rqing_isempty(g_disp_queue)) {
		t_disp_mem *dmem = NULL;
		ret = rqing_pop(g_disp_queue, (void **)&dmem);
		assert(ret == 0);

		rga_control_buffer_deinit(&dmem->bo, dmem->fd);
		free(dmem);
	}
	while(!rqing_isempty(g_disp_freequeue)) {
		t_disp_mem *dmem = NULL;
		ret = rqing_pop(g_disp_freequeue, (void **)&dmem);
		assert(ret == 0);

		rga_control_buffer_deinit(&dmem->bo, dmem->fd);
		free(dmem);
	}

	//释放占用内存
	rqing_deinit(&g_disp_queue);
	g_disp_queue = NULL;
	rqing_deinit(&g_disp_freequeue);
	g_disp_freequeue = NULL;
}

void set_usb_rotation(int angle)
{
    if (angle == 90)
        g_rotation = HAL_TRANSFORM_ROT_90;
    else if (angle == 270)
        g_rotation = HAL_TRANSFORM_ROT_270;
}

void set_usb_display(display_callback cb)
{
    pthread_mutex_lock(&g_display_lock);
    g_display_cb = cb;
    pthread_mutex_unlock(&g_display_lock);
}

void set_usb_param(int width, int height, display_callback cb)
{
    g_usb_en = true;
    g_usb_width = width;
    g_usb_height = height;
    set_usb_display(cb);
}

static int qbuf(int fd, struct v4l2_buffer *buf)
{
    int ret;

    ret = ioctl(fd, VIDIOC_QBUF, buf);
    if (ret < 0) {
        perror("VIDIOC_QBUF");
        return -1;
    }
    return 0;
}

static int dqbuf(int fd, struct v4l2_buffer *buf)
{
    int ret;

    ret = ioctl(fd, VIDIOC_DQBUF, buf);
    if (ret < 0) {
        perror("VIDIOC_DQBUF");
        return -1;
    }
    return 0;
}

static int stream_off(int fd)
{
    int ret;
    enum v4l2_buf_type type;

    memset(&type, 0, sizeof(type));
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        perror("VIDIOC_STREAMOFF");
        return -1;
    }
    return 0;
}

static int stream_on(int fd)
{
    int i;
    int ret;
    enum v4l2_buf_type type;

    memset(&type, 0, sizeof(type));
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        perror("VIDIOC_STREAMON");
        return -1;
    }
    return 0;
}

static void free_bufs(struct map_buffer *map_buf)
{
    int i;

    for (i = 0; i < BUFFER_COUNT; i++) {
        if (map_buf[i].start) {
            munmap(map_buf[i].start, map_buf[i].length);
            map_buf[i].start = NULL;
        }
    }
}

static int req_bufs(int fd, struct map_buffer *map_buf)
{
    int i;
    int ret;
    struct v4l2_requestbuffers reqbuf;

    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.count = BUFFER_COUNT;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
    if (ret < 0) {
        perror("VIDIOC_REQBUFS");
        return -1;
    }
    for (i = 0; i < reqbuf.count; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) {
            perror("VIDIOC_QUERYBUF");
            return -1;
        }
        map_buf[i].length = buf.length;
        map_buf[i].start = (char*)mmap(0, buf.length, PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, buf.m.offset);
        if (map_buf[i].start == MAP_FAILED) {
            perror("mmap");
            return -1;
        }
        ret = ioctl(fd, VIDIOC_QBUF, &buf);
        if (ret < 0) {
            perror("VIDIOC_QBUF");
            return -1;
        }
    }
    return 0;
}

static int set_fmt(int fd, int *width, int *height)
{
    int ret;
    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = *width;
    fmt.fmt.pix.height = *height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0) {
        perror("VIDIOC_S_FMT");
        return -1;
    }

    ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
    if (ret < 0) {
        perror("VIDIOC_G_FMT");
        return -1;
    }
	printf("type:%d\n", fmt.type);
    *width = fmt.fmt.pix.width;
    *height = fmt.fmt.pix.height;
    printf("%s: %dx%d\n", __func__, *width, *height);
    return 0;
}

static int querycap(int fd, char *type)
{
    struct v4l2_capability cap;
    memset(&cap, 0, sizeof(cap));
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        perror("VIDIOC_QUERYCAP");
        return -1;
    }
    if (!strstr((char *)cap.bus_info, type))
        return -1;
    return 0;
}

static int open_usb_camera(void)
{
    int i;
    struct stat st;
    char name[32];
    int fd;

    for (i = 0; i < CAMERA_NUM; i++) {
        snprintf(name, sizeof(name), "/dev/video%d", i);
        if (stat(name, &st) == -1)
            continue;
        fd = open(name, O_RDWR, 0);
        if (fd < 0)
            continue;
        if (!querycap(fd, "usb")) {
            printf("%s: %s\n", __func__, name);
            return fd;
        }
        close(fd);
    }
    return -1;
}

static void save_img(char *buf, unsigned int size) {
	static int seq = 0;
	char pOutPath[100];
	sprintf(pOutPath, "enc%d.jpg", seq);
	FILE *f = fopen(pOutPath, "w");
	fwrite(buf, 1, size, f);
	fclose(f);
	seq++;
}

void rga_copy(void *src, void *dst, int w, int h) {
  struct rkRgaCfg src_cfg, dst_cfg;

  src_cfg.addr = src;
  src_cfg.fmt = RK_FORMAT_YCrCb_422_SP;
  src_cfg.width = w;
  src_cfg.height = h;

  dst_cfg.addr = dst;
  dst_cfg.fmt = RK_FORMAT_YCrCb_422_SP;
  dst_cfg.width = w;
  dst_cfg.height = h;
  rkRgaBlit(&src_cfg, &dst_cfg);
}


// 显示线程，提高显示帧率
static void *proc_display(void *arg) {
	struct video_cb_param *param = (struct video_cb_param *)arg;
	video_cb cb = param->cb;
	free(param);


	int w,h;
	w = ((g_width-1)/16 + 1)*16;
	h = ((g_height-1)/16 + 1)*16;

	t_disp_mem *dmem = NULL;
	printf("++++++++++++++++++++++++thread\n");
	while(1) {
		// ring中没有数据，则延时
		if(rqing_isempty(g_disp_queue)) {
			usleep(1000);
			continue;
		}
		rqing_pop(g_disp_queue, (void **)&dmem);
		print_fps("jpg decode fps:");
		//printf("=============================>out buf:%p\n", buf);



		//rga_copy(dmem->bo.ptr, _ptr, w, h);
		//arm_copy(dmem->bo.ptr, _ptr, 1920, 1080);
		// 将数据scale后放到队列中
		if(cb){
			// yuv 422
			cb(dmem->bo.ptr, 1);	
			//display_fush();
		}

		// 此情况下，空闲队列不可能满
		if(rqing_isfull(g_disp_freequeue)) {
				printf("error in push free queue %d\n", __LINE__);
		}

		// 用完后释放数据到空闲队列
		rqing_push(g_disp_freequeue, (void*)dmem);		
		dmem = NULL;

	}

}

static void *process(void *arg)
{
    rga_info_t src, dst;
    int id = 0;
    RgaSURF_FORMAT fmt;
	int ret;

    struct v4l2_buffer buf;

    while (g_run) {
        id++;

		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if (dqbuf(g_fd, &buf)) {
            break;
		}
		print_fps("get jpeg fps:");
#if 0
        vpu_decode_jpeg_doing(&g_decode, g_map_buf[buf.index].start, buf.bytesused,
                              g_dec_fd, g_dec_bo.ptr);
		if(qbuf(g_fd, &buf)){
			break;
		}
#else
		// 取解码数据
		if(!rqing_isempty(g_disp_freequeue)) {
			t_disp_mem *dmem = NULL;
			ret = rqing_pop(g_disp_freequeue, (void **)&dmem);
			if(ret!=0) {
				printf("error in pop %d\n", __LINE__);
			}

		//printf("==>ptr:%p, fd:%d\n", dmem->bo.ptr, dmem->fd);
			// 解码jpeg 
			vpu_decode_jpeg_doing(&g_decode, g_map_buf[buf.index].start, buf.bytesused,
			dmem->fd, dmem->bo.ptr);//
			
			// 发送到显示队列
			if(rqing_isfull(g_disp_queue)) {
				// 显示队列满了， 释放掉这个数据,  放到空闲队列
				rqing_push(g_disp_freequeue, (void*)dmem);		
			}else {
				// 实现队列有空间，放到显示队列
				rqing_push(g_disp_queue, (void*)dmem);		
			}
		}
		

			if(qbuf(g_fd, &buf)){
				break;
			}
#endif

		/*
        fmt = (g_decode.fmt == MPP_FMT_YUV422SP ? RK_FORMAT_YCbCr_422_SP : RK_FORMAT_YCbCr_420_SP);
        if (!rockface_control_convert_detect(g_dec_bo.ptr, g_width, g_height, fmt, g_rotation, id))
            rockface_control_convert_feature(g_dec_bo.ptr, g_width, g_height, fmt, g_rotation, id);
			*/

		/*
        pthread_mutex_lock(&g_display_lock);
        if (g_display_cb)
            g_display_cb(g_dec_bo.ptr, g_dec_fd, fmt,
                    g_width, g_height, g_rotation);
        pthread_mutex_unlock(&g_display_lock);
		*/
    }

    pthread_exit(NULL);
}

int usb_camera_init(video_cb cb)
{
    int width = g_usb_width;
    int height = g_usb_height;
    if (!g_usb_en)
        return 0;

    g_fd = open_usb_camera();
    if (g_fd < 0) {
        printf("%s: %d exit!\n", __func__, __LINE__);
        return -1;
    }
	printf("set width:%d height:%d\n", width, height);
    if (set_fmt(g_fd, &width, &height)) {
        printf("%s: %d exit!\n", __func__, __LINE__);
        return -1;
    }
    if (req_bufs(g_fd, g_map_buf)) {
        printf("%s: %d exit!\n", __func__, __LINE__);
        return -1;
    }
    if (stream_on(g_fd)) {
        printf("%s: %d exit!\n", __func__, __LINE__);
        return -1;
    }

    if (rga_control_buffer_init(&g_dec_bo, &g_dec_fd, width, height, 16)) {
        printf("%s: %d exit!\n", __func__, __LINE__);
        return -1;
    }

    if (vpu_decode_jpeg_init(&g_decode, width, height)) {
        printf("%s: %d exit!\n", __func__, __LINE__);
        return -1;
    }

    g_width = width;
    g_height = height;

	{
		pthread_t id;
		struct video_cb_param *param = (struct video_cb_param *)malloc(sizeof(struct video_cb_param));
		param->cb = cb;
		pthread_create(&id, NULL, proc_display, (void*)param);
		pthread_detach(id);
	}

    g_run = true;
    if (pthread_create(&g_th, NULL, process, NULL)) {
        printf("%s: %d exit!\n", __func__, __LINE__);
        return -1;
    }

    return 0;
}

void usb_camera_exit(void)
{
    if (!g_usb_en)
        return;

    g_run = false;
    stream_off(g_fd);
    free_bufs(g_map_buf);
    close(g_fd);

    if (g_th) {
        pthread_join(g_th, NULL);
        g_th = 0;
    }
    vpu_decode_jpeg_done(&g_decode);
    rga_control_buffer_deinit(&g_dec_bo, g_dec_fd);
}
