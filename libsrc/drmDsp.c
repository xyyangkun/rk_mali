#include <stdio.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <drm_fourcc.h>
#include <string.h>

#include "./drmDsp/dev.h"
#include "./drmDsp/bo.h"
#include "./drmDsp/modeset.h"
#include "xf86drm.h"
#include "xf86drmMode.h"
#include "drmDsp.h"
#include "rkRgaApi.h"
#include "rga/rga.h"

struct drmDsp {
  struct fb_var_screeninfo vinfo;
  unsigned long screensize;
  char* fbp;
  struct sp_dev* dev;
  struct sp_plane** plane;
  struct sp_crtc* test_crtc;
  struct sp_plane* test_plane;
  int num_test_planes;
  struct sp_bo* bo[2];
  struct sp_bo* nextbo;
} gDrmDsp;

int display_get_fd() {
	return gDrmDsp.dev->fd;
}


// out_type 0 hdmi  1mipi for rk3568
int initDrmDsp(int out_type) {
  int ret = 0, i = 0;
  struct drmDsp* pDrmDsp = &gDrmDsp;

  memset(pDrmDsp, 0, sizeof(struct drmDsp));

  printf("in init drmsdp\n");
  pDrmDsp->dev = create_sp_dev();
  if (!pDrmDsp->dev) {
    printf("Failed to create sp_dev\n");
    return -1;
  }

  ret = initialize_screens(pDrmDsp->dev);
  if (ret) {
    printf("Failed to initialize screens\n");
    return ret;
  }
  pDrmDsp->plane = (struct sp_plane**)calloc(pDrmDsp->dev->num_planes, sizeof(struct sp_plane*));
  if (!pDrmDsp->plane) {
    printf("Failed to allocate plane array\n");
    return -1;
  }

  printf("DRM_FORMAT_NV12 = %d\n", DRM_FORMAT_NV12);
  printf("DRM_FORMAT_NV16 = %d\n", DRM_FORMAT_NV16);

  if(out_type == 0){
    // hdmi out ok
    pDrmDsp->test_crtc = &pDrmDsp->dev->crtcs[0];
    pDrmDsp->num_test_planes = pDrmDsp->test_crtc->num_planes;
    printf("\n\nyk debug planes num:%d\n", pDrmDsp->test_crtc->num_planes);
    for (i = 0; i < pDrmDsp->test_crtc->num_planes; i++) {
      pDrmDsp->plane[i] = get_sp_plane(pDrmDsp->dev, pDrmDsp->test_crtc);
      printf("yk debug !!!!!i=%d\n", i);
      //if (is_supported_format(pDrmDsp->plane[i], DRM_FORMAT_NV12))
      //if (is_supported_format(pDrmDsp->plane[i], DRM_FORMAT_NV16))
      // yk debug test
      //if (is_supported_format(pDrmDsp->plane[i], DRM_FORMAT_BGR565))
      //if (is_supported_format(pDrmDsp->plane[i], DRM_FORMAT_RGB888))
        pDrmDsp->test_plane = pDrmDsp->plane[i];

      // yk debug
      if(i==0)break;
	  //
      //if(i==2)break;
      //if(i==3)break;
      //if(i==1)break;

    }
  }
  else {
	// mipi out
    pDrmDsp->test_crtc = &pDrmDsp->dev->crtcs[1];
    pDrmDsp->num_test_planes = pDrmDsp->test_crtc->num_planes;
    printf("\n\nyk debug planes num:%d\n", pDrmDsp->test_crtc->num_planes);
    for (i = 0; i < pDrmDsp->test_crtc->num_planes; i++) {
      pDrmDsp->plane[i] = get_sp_plane(pDrmDsp->dev, pDrmDsp->test_crtc);
      printf("yk debug !!!!!i=%d\n", i);
      //if (is_supported_format(pDrmDsp->plane[i], DRM_FORMAT_NV12))
      //if (is_supported_format(pDrmDsp->plane[i], DRM_FORMAT_NV16))
      // yk debug test
      //if (is_supported_format(pDrmDsp->plane[i], DRM_FORMAT_BGR565))
      //if (is_supported_format(pDrmDsp->plane[i], DRM_FORMAT_RGB888))
        pDrmDsp->test_plane = pDrmDsp->plane[i];

      // yk debug
      // if(i==6)break;
      if(i==2)break;
    }
  }

  if (!pDrmDsp->test_plane)
  {
	  printf("get test plane error\n");
    return -1;
  }

  rkRgaInit();
}

void deInitDrmDsp() {
  struct drmDsp* pDrmDsp = &gDrmDsp;
  if (pDrmDsp->bo[0])
    free_sp_bo(pDrmDsp->bo[0]);
  if (pDrmDsp->bo[1])
    free_sp_bo(pDrmDsp->bo[1]);
  destroy_sp_dev(pDrmDsp->dev);
  memset(pDrmDsp, 0, sizeof(struct drmDsp));
}

static int arm_camera_yuv420_scale_arm(char *srcbuf, char *dstbuf,int src_w, int src_h,int dst_w, int dst_h) {
	unsigned char *psY,*pdY,*psUV,*pdUV;
	unsigned char *src,*dst;
	int srcW,srcH,cropW,cropH,dstW,dstH;
	long zoomindstxIntInv,zoomindstyIntInv;
	long x,y;
	long yCoeff00,yCoeff01,xCoeff00,xCoeff01;
	long sX,sY;
	long r0,r1,a,b,c,d;
	int ret = 0;
	int nv21DstFmt = 0, mirror = 0;
	int ratio = 0;
	int top_offset=0,left_offset=0;

	//need crop ?
	if((src_w*100/src_h) != (dst_w*100/dst_h)){
		ratio = ((src_w*100/dst_w) >= (src_h*100/dst_h))?(src_h*100/dst_h):(src_w*100/dst_w);
		cropW = ratio*dst_w/100;
		cropH = ratio*dst_h/100;
		
		left_offset=((src_w-cropW)>>1) & (~0x01);
		top_offset=((src_h-cropH)>>1) & (~0x01);
	}else{
		cropW = src_w;
		cropH = src_h;
		top_offset=0;
		left_offset=0;
	}

	src = psY = (unsigned char*)(srcbuf)+top_offset*src_w+left_offset;
	//psUV = psY +src_w*src_h+top_offset*src_w/2+left_offset;
	psUV = (unsigned char*)(srcbuf) +src_w*src_h+top_offset*src_w/2+left_offset;

	
	srcW =src_w;
	srcH = src_h;
//	cropW = src_w;
//	cropH = src_h;

	
	dst = pdY = (unsigned char*)dstbuf;
	pdUV = pdY + dst_w*dst_h;
	dstW = dst_w;
	dstH = dst_h;

	zoomindstxIntInv = ((unsigned long)(cropW)<<16)/dstW + 1;
	zoomindstyIntInv = ((unsigned long)(cropH)<<16)/dstH + 1;
	//y
	//for(y = 0; y<dstH - 1 ; y++ ) {	
	for(y = 0; y<dstH; y++ ) {	
		yCoeff00 = (y*zoomindstyIntInv)&0xffff;
		yCoeff01 = 0xffff - yCoeff00;
		sY = (y*zoomindstyIntInv >> 16);
		sY = (sY >= srcH - 1)? (srcH - 2) : sY; 	
		for(x = 0; x<dstW; x++ ) {
			xCoeff00 = (x*zoomindstxIntInv)&0xffff;
			xCoeff01 = 0xffff - xCoeff00;	
			sX = (x*zoomindstxIntInv >> 16);
			sX = (sX >= srcW -1)?(srcW- 2) : sX;
			a = psY[sY*srcW + sX];
			b = psY[sY*srcW + sX + 1];
			c = psY[(sY+1)*srcW + sX];
			d = psY[(sY+1)*srcW + sX + 1];

			r0 = (a * xCoeff01 + b * xCoeff00)>>16 ;
			r1 = (c * xCoeff01 + d * xCoeff00)>>16 ;
			r0 = (r0 * yCoeff01 + r1 * yCoeff00)>>16;
			
			if(mirror)
				pdY[dstW -1 - x] = r0;
			else
				pdY[x] = r0;
		}
		pdY += dstW;
	}

	dstW /= 2;
	dstH /= 2;
	srcW /= 2;
	srcH /= 2;

	//UV
	//for(y = 0; y<dstH - 1 ; y++ ) {
	for(y = 0; y<dstH; y++ ) {
		yCoeff00 = (y*zoomindstyIntInv)&0xffff;
		yCoeff01 = 0xffff - yCoeff00;
		sY = (y*zoomindstyIntInv >> 16);
		sY = (sY >= srcH -1)? (srcH - 2) : sY;		
		for(x = 0; x<dstW; x++ ) {
			xCoeff00 = (x*zoomindstxIntInv)&0xffff;
			xCoeff01 = 0xffff - xCoeff00;	
			sX = (x*zoomindstxIntInv >> 16);
			sX = (sX >= srcW -1)?(srcW- 2) : sX;
			//U
			a = psUV[(sY*srcW + sX)*2];
			b = psUV[(sY*srcW + sX + 1)*2];
			c = psUV[((sY+1)*srcW + sX)*2];
			d = psUV[((sY+1)*srcW + sX + 1)*2];

			r0 = (a * xCoeff01 + b * xCoeff00)>>16 ;
			r1 = (c * xCoeff01 + d * xCoeff00)>>16 ;
			r0 = (r0 * yCoeff01 + r1 * yCoeff00)>>16;
		
			if(mirror && nv21DstFmt)
				pdUV[dstW*2-1- (x*2)] = r0;
			else if(mirror)
				pdUV[dstW*2-1-(x*2+1)] = r0;
			else if(nv21DstFmt)
				pdUV[x*2 + 1] = r0;
			else
				pdUV[x*2] = r0;
			//V
			a = psUV[(sY*srcW + sX)*2 + 1];
			b = psUV[(sY*srcW + sX + 1)*2 + 1];
			c = psUV[((sY+1)*srcW + sX)*2 + 1];
			d = psUV[((sY+1)*srcW + sX + 1)*2 + 1];

			r0 = (a * xCoeff01 + b * xCoeff00)>>16 ;
			r1 = (c * xCoeff01 + d * xCoeff00)>>16 ;
			r0 = (r0 * yCoeff01 + r1 * yCoeff00)>>16;

			if(mirror && nv21DstFmt)
				pdUV[dstW*2-1- (x*2+1) ] = r0;
			else if(mirror)
				pdUV[dstW*2-1-(x*2)] = r0;
			else if(nv21DstFmt)
				pdUV[x*2] = r0;
			else
				pdUV[x*2 + 1] = r0;
		}
		pdUV += dstW*2;
	}
	return 0;
}
#define DRM_YUV422

// 初始化显示内存
// out_type 0 hdmi_out 1 mipi_out
// buf_type 0 yuv  1 rgb888
int init_display_mem(int dispWidth, int dispHeight, int out_type, int buf_type){
	int ret;
	//int fmt = DRM_FORMAT_NV12;
	struct drm_mode_create_dumb cd;
	struct sp_bo* bo;
	struct drmDsp* pDrmDsp = &gDrmDsp;
	int wAlign16;
	int hAlign16;
	uint32_t handles[4]={0}, pitches[4]={0}, offsets[4]={0};

	// 测试发现hdmi显示和mipi显示的对其要求不通，不明白原因
	if(out_type == 0){
		wAlign16 = ((dispWidth+ 15) & (~15));
		hAlign16 = ((dispHeight + 15) &(~15));
	}
	else {
		// 竖屏要求1080x1920
		wAlign16 = dispWidth;
		hAlign16 = dispHeight;
	}


	//create bo
	if (!pDrmDsp->bo[0]) {
		printf("@@@@@@@@@@@@@@@@@@@%s:bo widthxheight:%dx%d\n", __func__, wAlign16, hAlign16);
		if(buf_type == 0) {
#ifdef DRM_YUV422
		// nv16 yuv422
		pDrmDsp->bo[0] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
				32, 16, DRM_FORMAT_NV16, 0);
		pDrmDsp->bo[1] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
				32, 16, DRM_FORMAT_NV16, 0);
		if (!pDrmDsp->bo[0] || !pDrmDsp->bo[1]) {
			printf("%s:create bo failed ! \n", __func__);
			return -1;
		}
#else
		// nv12 yuv420
		pDrmDsp->bo[0] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
				32, 16, DRM_FORMAT_NV12, 0);
		pDrmDsp->bo[1] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
				32, 16, DRM_FORMAT_NV12, 0);
		if (!pDrmDsp->bo[0] || !pDrmDsp->bo[1]) {
			printf("%s:create bo failed ! \n", __func__);
			return -1;
		}

#endif
		}
		else {
			// rgb8888
		pDrmDsp->bo[0] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
				32, 32, DRM_FORMAT_XRGB8888, 0);
				//32, 24, DRM_FORMAT_RGB888, 0);
				//32, 32, DRM_FORMAT_XRGB8888, 0);
		pDrmDsp->bo[1] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
				32, 32, DRM_FORMAT_XRGB8888, 0);
				//32, 24, DRM_FORMAT_RGB888, 0);
				//32, 32, DRM_FORMAT_XRGB8888, 0);
		if (!pDrmDsp->bo[0] || !pDrmDsp->bo[1]) {
			printf("%s:create bo failed ! \n", __func__);
			return -1;
		}

		}
		pDrmDsp->nextbo = pDrmDsp->bo[0];
	}

	if (!pDrmDsp->nextbo) {
		printf("%s:no available bo ! \n", __func__);
		return -1;
	}

	bo = pDrmDsp->nextbo;

	if(buf_type == 0){
		// yuv420 or 422
	handles[0] = bo->handle;
	pitches[0] = wAlign16;
	offsets[0] = 0;
	handles[1] = bo->handle;
	pitches[1] = wAlign16;
	offsets[1] = wAlign16 * hAlign16;
	}
	else {
	handles[0] = bo->handle;
	pitches[0] = wAlign16*4;


	}


	// 感觉这些代码只需要运行一次就好
	ret = drmModeAddFB2(bo->dev->fd, bo->width, bo->height,
			bo->format, handles, pitches, offsets,
			&bo->fb_id, bo->flags);
	if (ret) {
		printf("%s:failed to create fb ret=%d\n", __func__, ret);
		printf("fd:%d ,wxh:%ux%u,format:%u,handles:%u,%u,pictches:%u,%u,offsets:%u,%u,fb_id:%u,flags:%u \n",
				bo->dev->fd, bo->width, bo->height, bo->format,
				handles[0], handles[1], pitches[0], pitches[1],
				offsets[0], offsets[1], bo->fb_id, bo->flags);
		return ret;
	}

	printf("=======================> plane_id = %d, crtc-id = %d\n", 
			pDrmDsp->test_plane->plane->plane_id,
			pDrmDsp->test_crtc->crtc->crtc_id);
	printf("w:%d h:%d\n", wAlign16, hAlign16);
	ret = drmModeSetPlane(pDrmDsp->dev->fd, pDrmDsp->test_plane->plane->plane_id,
			pDrmDsp->test_crtc->crtc->crtc_id, bo->fb_id, 0, 0, 0,
			//pDrmDsp->test_crtc->crtc->mode.hdisplay,
			wAlign16, hAlign16,
			//pDrmDsp->test_crtc->crtc->mode.vdisplay,
			0, 0, wAlign16 << 16,  hAlign16 << 16);
	if (ret) {
		printf("----------------------->failed to set plane to crtc ret=%d, line=%d\n", ret, __LINE__);
		return ret;
	}
	return 0;
}


// 切换显示内存，就是刷新显示
void display_fush(){
	struct drmDsp* pDrmDsp = &gDrmDsp;
	//switch bo
	if (pDrmDsp->nextbo == pDrmDsp->bo[0])
		pDrmDsp->nextbo = pDrmDsp->bo[1];
	else
		pDrmDsp->nextbo = pDrmDsp->bo[0];
}

// 获取显示内存
int display_get_mem(void **ptr, int *fd) {
	struct drmDsp* pDrmDsp = &gDrmDsp;
	struct sp_bo* bo;
	bo = pDrmDsp->nextbo;
	*ptr = bo->map_addr;
	*fd = bo->fd;
	// printf("=================> yk debug in get mem  map_addr:%p, fd=%d\n", bo->map_addr, bo->fd);
	return 0;
}

static int start = 1;
int drmDspFrame(int srcWidth, int srcHeight, int dispWidth, int dispHeight,
		void* dmaFd, int fmt)
{
  int ret;
  struct drm_mode_create_dumb cd;
  struct sp_bo* bo;
  struct drmDsp* pDrmDsp = &gDrmDsp;

#if 1
  int wAlign16 = ((dispWidth+ 15) & (~15));
  int hAlign16 = ((dispHeight+15) &(~15));
#else
  int wAlign16 = dispWidth;
  int hAlign16 = dispHeight;
#endif
  int frameSize = wAlign16 * hAlign16 * 3 / 2;
  uint32_t handles[4], pitches[4], offsets[4];

  if (DRM_FORMAT_NV12 != fmt) {
    printf("%s just support NV12 to display\n", __func__);
    return -1;
  }
  //create bo
#if 1
  if (!pDrmDsp->bo[0]) {
    // printf("%s:bo widthxheight:%dx%d\n", __func__, wAlign16, hAlign16);
    pDrmDsp->bo[0] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
                                  16, 16, DRM_FORMAT_NV12, 0);
    pDrmDsp->bo[1] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
                                  16, 16, DRM_FORMAT_NV12, 0);
    if (!pDrmDsp->bo[0] || !pDrmDsp->bo[1]) {
      printf("%s:create bo failed ! \n", __func__);
      return -1;
    }
    pDrmDsp->nextbo = pDrmDsp->bo[0];
  }

  if (!pDrmDsp->nextbo) {
    printf("%s:no available bo ! \n", __func__);
    return -1;
  }

  bo = pDrmDsp->nextbo;
#else
  bo = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
                    16, 32, DRM_FORMAT_NV12, 0);
  if (!bo)
    printf("%s:create bo failed ! \n", __func__);
#endif


  handles[0] = bo->handle;
  pitches[0] = wAlign16;
  offsets[0] = 0;
  handles[1] = bo->handle;
  pitches[1] = wAlign16;
  offsets[1] = wAlign16 * hAlign16;

#if 1
  struct rkRgaCfg src_cfg, dst_cfg;

  src_cfg.addr = dmaFd;
  src_cfg.fmt = RK_FORMAT_YCrCb_420_SP;
  src_cfg.width = srcWidth;
  src_cfg.height = srcHeight;

  dst_cfg.addr = bo->map_addr;
  dst_cfg.fmt = RK_FORMAT_YCrCb_420_SP;
  dst_cfg.width = dispWidth;
  dst_cfg.height = dispHeight;

#if 1
  // yk debug change
  rkRgaBlit(&src_cfg, &dst_cfg);
#else
  dst_cfg.addr = dmaFd;
  bo->map_addr = dmaFd;
  hdnles[0] 

#endif

#else
  //copy src data to bo
  if (ori_width == width)
	  memcpy(bo->map_addr, dmaFd, wAlign16 * hAlign16 * 3 / 2);
  else
	  arm_camera_yuv420_scale_arm(dmaFd, bo->map_addr, ori_width, ori_height, width, height);
#endif

  if(start == 1)
  {
	  // 感觉这些代码只需要运行一次就好
	  start = 0;
  ret = drmModeAddFB2(bo->dev->fd, bo->width, bo->height,
                      bo->format, handles, pitches, offsets,
                      &bo->fb_id, bo->flags);
  if (ret) {
    printf("%s:failed to create fb ret=%d\n", __func__, ret);
    printf("fd:%d ,wxh:%ux%u,format:%u,handles:%u,%u,pictches:%u,%u,offsets:%u,%u,fb_id:%u,flags:%u \n",
           bo->dev->fd, bo->width, bo->height, bo->format,
           handles[0], handles[1], pitches[0], pitches[1],
           offsets[0], offsets[1], bo->fb_id, bo->flags);
    return ret;
  }

  ret = drmModeSetPlane(pDrmDsp->dev->fd, pDrmDsp->test_plane->plane->plane_id,
                        pDrmDsp->test_crtc->crtc->crtc_id, bo->fb_id, 0, 0, 0,
                        //pDrmDsp->test_crtc->crtc->mode.hdisplay,
			wAlign16, hAlign16,
                        //pDrmDsp->test_crtc->crtc->mode.vdisplay,
                        0, 0, wAlign16 << 16,  hAlign16 << 16);
  if (ret) {
    printf("failed to set plane to crtc ret=%d\n", ret);
    return ret;
  }
  }
  //free_sp_bo(bo);
#if 0
  if (pDrmDsp->test_plane->bo) {
    if (pDrmDsp->test_plane->bo->fb_id) {
      ret = drmModeRmFB(pDrmDsp->dev->fd, pDrmDsp->test_plane->bo->fb_id);
      if (ret)
        printf("Failed to rmfb ret=%d!\n", ret);
    }
    if (pDrmDsp->test_plane->bo->handle) {
      struct drm_gem_close req = {
        .handle = pDrmDsp->test_plane->bo->handle,
      };

      drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_CLOSE, &req);
      printf("%s:close bo success!\n", __func__);
    }

    if (!pDrmDsp->nextbo)
      free_sp_bo(pDrmDsp->test_plane->bo);
  }
  pDrmDsp->test_plane->bo = bo; //last po
#endif
#if 1
  //switch bo
  if (pDrmDsp->nextbo == pDrmDsp->bo[0])
    pDrmDsp->nextbo = pDrmDsp->bo[1];
  else
    pDrmDsp->nextbo = pDrmDsp->bo[0];
#endif


}

// 直接写数据不经过rga复制
int drmDspFrame_1080p(int srcWidth, int srcHeight, int dispWidth, int dispHeight,
		void* dmaFd, int fmt)
{
  int ret;
  struct drm_mode_create_dumb cd;
  struct sp_bo* bo;
  struct drmDsp* pDrmDsp = &gDrmDsp;

  int wAlign16 = ((dispWidth+ 15) & (~15));
  int hAlign16 = dispHeight;
  int frameSize = wAlign16 * hAlign16 * 3 / 2;
  uint32_t handles[4], pitches[4], offsets[4];

  if (DRM_FORMAT_NV12 != fmt) {
    printf("%s just support NV12 to display\n", __func__);
    return -1;
  }
  //create bo
#if 1
  if (!pDrmDsp->bo[0]) {
    printf("%s:bo widthxheight:%dx%d\n", __func__, wAlign16, hAlign16);
    pDrmDsp->bo[0] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
                                  16, 32, DRM_FORMAT_NV12, 0);
    pDrmDsp->bo[1] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
                                  16, 32, DRM_FORMAT_NV12, 0);
    if (!pDrmDsp->bo[0] || !pDrmDsp->bo[1]) {
      printf("%s:create bo failed ! \n", __func__);
      return -1;
    }
    pDrmDsp->nextbo = pDrmDsp->bo[0];
  }

  if (!pDrmDsp->nextbo) {
    printf("%s:no available bo ! \n", __func__);
    return -1;
  }

  bo = pDrmDsp->nextbo;
#else
  bo = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
                    16, 32, DRM_FORMAT_NV12, 0);
  if (!bo)
    printf("%s:create bo failed ! \n", __func__);
#endif


  //20210603
 #if 1
  handles[0] = bo->handle;
  pitches[0] = wAlign16;
  offsets[0] = 0;
  handles[1] = bo->handle;
  pitches[1] = wAlign16;
  offsets[1] = wAlign16 * hAlign16;
#else
  handles[0] = bo->handle;
  pitches[0] = 1080*3;
  offsets[0] = 0;
#endif

#if 1
  struct rkRgaCfg src_cfg, dst_cfg;

  src_cfg.addr = dmaFd;
  src_cfg.fmt = RK_FORMAT_YCrCb_420_SP;
  src_cfg.width = srcWidth;
  src_cfg.height = srcHeight;

  dst_cfg.addr = bo->map_addr;
  dst_cfg.fmt = RK_FORMAT_YCrCb_420_SP;
  dst_cfg.width = dispWidth;
  dst_cfg.height = dispHeight;

#if 1
  // yk debug change
  rkRgaBlit(&src_cfg, &dst_cfg);
#else
  dst_cfg.addr = dmaFd;
  bo->map_addr = dmaFd;
  hdnles[0] 

#endif

#else
  //copy src data to bo
  if (ori_width == width)
	  memcpy(bo->map_addr, dmaFd, wAlign16 * hAlign16 * 3 / 2);
  else
	  arm_camera_yuv420_scale_arm(dmaFd, bo->map_addr, ori_width, ori_height, width, height);
#endif

  if(start == 1)
  {
	  // 感觉这些代码只需要运行一次就好
	  start = 0;
  ret = drmModeAddFB2(bo->dev->fd, bo->width, bo->height,
                      bo->format, handles, pitches, offsets,
                      &bo->fb_id, bo->flags);
  if (ret) {
    printf("%s:failed to create fb ret=%d\n", __func__, ret);
    printf("fd:%d ,wxh:%ux%u,format:%u,handles:%u,%u,pictches:%u,%u,offsets:%u,%u,fb_id:%u,flags:%u \n",
           bo->dev->fd, bo->width, bo->height, bo->format,
           handles[0], handles[1], pitches[0], pitches[1],
           offsets[0], offsets[1], bo->fb_id, bo->flags);
    return ret;
  }

  ret = drmModeSetPlane(pDrmDsp->dev->fd, pDrmDsp->test_plane->plane->plane_id,
                        pDrmDsp->test_crtc->crtc->crtc_id, bo->fb_id, 0, 0, 0,
                        //pDrmDsp->test_crtc->crtc->mode.hdisplay,
			wAlign16, hAlign16,
                        //pDrmDsp->test_crtc->crtc->mode.vdisplay,
                        0, 0, wAlign16 << 16,  hAlign16 << 16);
  if (ret) {
    printf("failed to set plane to crtc ret=%d\n", ret);
    return ret;
  }
  }
  //free_sp_bo(bo);
#if 0
  if (pDrmDsp->test_plane->bo) {
    if (pDrmDsp->test_plane->bo->fb_id) {
      ret = drmModeRmFB(pDrmDsp->dev->fd, pDrmDsp->test_plane->bo->fb_id);
      if (ret)
        printf("Failed to rmfb ret=%d!\n", ret);
    }
    if (pDrmDsp->test_plane->bo->handle) {
      struct drm_gem_close req = {
        .handle = pDrmDsp->test_plane->bo->handle,
      };

      drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_CLOSE, &req);
      printf("%s:close bo success!\n", __func__);
    }

    if (!pDrmDsp->nextbo)
      free_sp_bo(pDrmDsp->test_plane->bo);
  }
  pDrmDsp->test_plane->bo = bo; //last po
#endif
#if 1
  //switch bo
  if (pDrmDsp->nextbo == pDrmDsp->bo[0])
    pDrmDsp->nextbo = pDrmDsp->bo[1];
  else
    pDrmDsp->nextbo = pDrmDsp->bo[0];
#endif


}


















int drmDspFrame_rgb(int srcWidth, int srcHeight, int dispWidth, int dispHeight,
		void* dmaFd, int fmt)
{
  int ret;
  struct drm_mode_create_dumb cd;
  struct sp_bo* bo;
  struct drmDsp* pDrmDsp = &gDrmDsp;

#if 0
  // yuv 420
  int wAlign16 = ((dispWidth+ 15) & (~15));
  int hAlign16 = dispHeight;
  int frameSize = wAlign16 * hAlign16 * 3 / 2;
#else
  // rgb 888
  int wAlign16 = ((dispWidth+ 15) & (~15));
  int hAlign16 = dispHeight;
  int frameSize = wAlign16 * hAlign16 * 3;
  //printf("wAlign16=%d hAlign16=%d\n", wAlign16, hAlign16);
#endif
  uint32_t handles[4]={0}, pitches[4]={0}, offsets[4]={0};

  if (DRM_FORMAT_BGR888 != fmt) {
    //printf("%s just support rgb to display\n", __func__);
    // return -1;
  }
  //create bo
#if 1
  if (!pDrmDsp->bo[0]) {
    printf("%s:bo widthxheight:%dx%d\n", __func__, wAlign16, hAlign16);
    pDrmDsp->bo[0] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
                                   24, 32, DRM_FORMAT_RGB888, 0);
                                   //16, 32, DRM_FORMAT_NV12, 0);
    pDrmDsp->bo[1] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
                                   24, 32, DRM_FORMAT_RGB888, 0);
                                  //16, 32, DRM_FORMAT_NV12, 0);
    if (!pDrmDsp->bo[0] || !pDrmDsp->bo[1]) {
      printf("%s:create bo failed ! \n", __func__);
      return -1;
    }
    pDrmDsp->nextbo = pDrmDsp->bo[0];

	  // printf("yk debug bo->format=%d   ===>rgb:%d\n", pDrmDsp->bo[0]->format, RK_FORMAT_RGB_888);
	  //printf("yk debug bo->format=%d   ===>rgb:%d\n", pDrmDsp->bo[1]->format, RK_FORMAT_RGB_888);
  }

  if (!pDrmDsp->nextbo) {
    printf("%s:no available bo ! \n", __func__);
    return -1;
  }

  bo = pDrmDsp->nextbo;

	  //printf("yk debug bo->format=%d   ===>rgb:%d\n", bo->format, RK_FORMAT_BGR_888);
#else
  bo = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16,
                    16, 32, DRM_FORMAT_NV12, 0);
  if (!bo)
    printf("%s:create bo failed ! \n", __func__);
#endif

#if 0
  handles[0] = bo->handle;
  pitches[0] = wAlign16;
  offsets[0] = 0;
  handles[1] = bo->handle;
  pitches[1] = wAlign16;
  offsets[1] = wAlign16 * hAlign16;
#else
  handles[0] = bo->handle;
  // 之前是因为这个只设置错误，导致显示异常
  pitches[0] = 720*3;//bo->pitch;
  offsets[0] = 0;

#if 0
  handles[1] = bo->handle;

  // pitches[1] = bo->pitch;
  pitches[1] = wAlign16;

  //offsets[1] = 0;
  offsets[1] = wAlign16 * hAlign16;
#endif
#endif



#if 1
  struct rkRgaCfg src_cfg, dst_cfg;

  src_cfg.addr = dmaFd;
  src_cfg.fmt = RK_FORMAT_BGR_888;
  // src_cfg.fmt = RK_FORMAT_YCrCb_420_SP;
  src_cfg.width = srcWidth;
  src_cfg.height = srcHeight;

  dst_cfg.addr = bo->map_addr;
  dst_cfg.fmt = RK_FORMAT_BGR_888;
  // dst_cfg.fmt = RK_FORMAT_YCrCb_420_SP;
  dst_cfg.width = dispWidth;
  dst_cfg.height = dispHeight;

#if 1
  // yk debug change
  rkRgaBlit(&src_cfg, &dst_cfg);
#else
	  memcpy(bo->map_addr, dmaFd, wAlign16 * hAlign16 * 3);

#endif

#else
  //copy src data to bo
  if (ori_width == width)
	  memcpy(bo->map_addr, dmaFd, wAlign16 * hAlign16 * 3 / 2);
  else
	  arm_camera_yuv420_scale_arm(dmaFd, bo->map_addr, ori_width, ori_height, width, height);
#endif

  if(start == 1)
  {
	  // 感觉这些代码只需要运行一次就好
	  start = 0;

	  printf("bo->format=%d   ===>rgb:%d  bgr:%d\n", bo->format, DRM_FORMAT_RGB888,  DRM_FORMAT_BGR888);

    printf("fd:%d ,wxh:%ux%u,format:%u,handles:%u,%u,pictches:%u,%u,offsets:%u,%u,fb_id:%u,flags:%u \n",
           bo->dev->fd, bo->width, bo->height, bo->format,
           handles[0], handles[1], pitches[0], pitches[1],
           offsets[0], offsets[1], bo->fb_id, bo->flags);
  ret = drmModeAddFB2(bo->dev->fd, bo->width, bo->height,
                      bo->format, handles, pitches, offsets,
                      &bo->fb_id, bo->flags);
  if (ret) {
    printf("%s:failed to create fb ret=%d\n", __func__, ret);
    printf("fd:%d ,wxh:%ux%u,format:%u,handles:%u,%u,pictches:%u,%u,offsets:%u,%u,fb_id:%u,flags:%u \n",
           bo->dev->fd, bo->width, bo->height, bo->format,
           handles[0], handles[1], pitches[0], pitches[1],
           offsets[0], offsets[1], bo->fb_id, bo->flags);
    return ret;
  }
#if 0
  ret = drmModeSetPlane(pDrmDsp->dev->fd, pDrmDsp->test_plane->plane->plane_id,
                        pDrmDsp->test_crtc->crtc->crtc_id, bo->fb_id, 0, 0, 0,
                        //pDrmDsp->test_crtc->crtc->mode.hdisplay,
			wAlign16, hAlign16,
                        //pDrmDsp->test_crtc->crtc->mode.vdisplay,
                        0, 0, wAlign16 << 16,  hAlign16 << 16);
  if (ret) {
    printf("failed to set plane to crtc ret=%d\n", ret);
    return ret;
  }
#else
  ret = drmModeSetPlane(pDrmDsp->dev->fd, pDrmDsp->test_plane->plane->plane_id,
                        pDrmDsp->test_crtc->crtc->crtc_id, bo->fb_id, 0, 0, 0,
                        //pDrmDsp->test_crtc->crtc->mode.hdisplay,
			wAlign16, hAlign16,
                        //pDrmDsp->test_crtc->crtc->mode.vdisplay,
                        0, 0, wAlign16 << 16,  hAlign16 << 16);
  if (ret) {
    printf("failed to set plane to crtc ret=%d\n", ret);
    return ret;
  }

  // yk debug  只有把两个图层都打开，才可以让QT显示在最上面
  if(0)
  {
	  ret = drmModeSetPlane(pDrmDsp->dev->fd, pDrmDsp->plane[0]->plane->plane_id,
			  pDrmDsp->test_crtc->crtc->crtc_id, bo->fb_id, 0, 0, 0,
			  //pDrmDsp->test_crtc->crtc->mode.hdisplay,
			  wAlign16, hAlign16,
			  //pDrmDsp->test_crtc->crtc->mode.vdisplay,
			  0, 0, wAlign16 << 16,  hAlign16 << 16);
	  if (ret) {
		  printf("failed to set plane to crtc ret=%d\n", ret);
		  return ret;
	  }
	  ret = drmModeSetPlane(pDrmDsp->dev->fd, pDrmDsp->plane[1]->plane->plane_id,
			  pDrmDsp->test_crtc->crtc->crtc_id, bo->fb_id, 0, 0, 0,
			  //pDrmDsp->test_crtc->crtc->mode.hdisplay,
			  wAlign16, hAlign16,
			  //pDrmDsp->test_crtc->crtc->mode.vdisplay,
			  0, 0, wAlign16 << 16,  hAlign16 << 16);
	  if (ret) {
		  printf("failed to set plane to crtc ret=%d\n", ret);
		  return ret;
	  }
  }
#endif
  }
  //free_sp_bo(bo);
#if 0
  if (pDrmDsp->test_plane->bo) {
    if (pDrmDsp->test_plane->bo->fb_id) {
      ret = drmModeRmFB(pDrmDsp->dev->fd, pDrmDsp->test_plane->bo->fb_id);
      if (ret)
        printf("Failed to rmfb ret=%d!\n", ret);
    }
    if (pDrmDsp->test_plane->bo->handle) {
      struct drm_gem_close req = {
        .handle = pDrmDsp->test_plane->bo->handle,
      };

      drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_CLOSE, &req);
      printf("%s:close bo success!\n", __func__);
    }

    if (!pDrmDsp->nextbo)
      free_sp_bo(pDrmDsp->test_plane->bo);
  }
  pDrmDsp->test_plane->bo = bo; //last po
#endif
#if 1
  //switch bo
  if (pDrmDsp->nextbo == pDrmDsp->bo[0])
    pDrmDsp->nextbo = pDrmDsp->bo[1];
  else
    pDrmDsp->nextbo = pDrmDsp->bo[0];
#endif


}
