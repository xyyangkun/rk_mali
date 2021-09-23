#include "LibCL.h"
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#define print_fps(name) do {\
	static int count=0; \
	static struct timeval t_old = {0}; \
	if(count%120 == 0) \
	{ \
		static struct timeval t_new; \
		gettimeofday(&t_new, 0); \
		printf("=====>%s:%d\n",name,120*1000000/((t_new.tv_sec-t_old.tv_sec)*1000000 + (t_new.tv_usec - t_old.tv_usec)));\
		t_old = t_new; \
	} \
	count++; \
} while(0)

const unsigned int w1 = 1920;
const unsigned int h1 = 1080;
const unsigned int w2 = 1280;
const unsigned int h2 = 720;
const unsigned int x = 20;
const unsigned int y = 20;
const unsigned int BIG_SIZE = w1*h1*3/2;
const unsigned int LIT_SIZE = w2*h2*3/2;

bool quit = false;
static void sigterm_handler(int sig) {
	fprintf(stderr, "signal %d\n", sig);
	quit = true;
}


void *blend_proc(void *param) {
	// 读文件
	const char *in1 = "1080_nv12.yuv";
	const char *in2 = "720p_nv12.yuv";

    FILE *in1_fp = fopen(in1 , "rb");
    if(!in1_fp) {
        printf("ERROR to open infile\n");
        return NULL;
    }

    FILE *in2_fp = fopen(in2 , "rb");
    if(!in2_fp) {
        printf("ERROR to open infile\n");
        return NULL;
    }

	// 分配drm内存 
	MB_IMAGE_INFO_S disp_info1 = {w1, h1, w1, h1, IMAGE_TYPE_NV12};
	MB_IMAGE_INFO_S disp_info2 = {w2, h2, w2, h2, IMAGE_TYPE_NV12};
	MEDIA_BUFFER mb1 = RK_MPI_MB_CreateImageBuffer(&disp_info1, RK_TRUE, 0); 
	if (!mb1) {
		printf("ERROR: no space left!1\n");
		return NULL;
	}

	MEDIA_BUFFER mb2 = RK_MPI_MB_CreateImageBuffer(&disp_info2, RK_TRUE, 0); 
	if (!mb1) {
		printf("ERROR: no space left!2\n");
		return NULL;
	}

    unsigned char *src1     = (unsigned char *)RK_MPI_MB_GetPtr(mb1);
    unsigned char *src2     = (unsigned char *)RK_MPI_MB_GetPtr(mb2);
    unsigned char *result   = src1;;

	int fd_mb1 = RK_MPI_MB_GetFD(mb1);
	int fd_mb2 = RK_MPI_MB_GetFD(mb2);

	// 加载图片
	fread(src1, BIG_SIZE, 1, in1_fp);
	fread(src2, LIT_SIZE, 1, in2_fp);

	printf("=================>after read img\n");



	// opencl 初始化
	ClOperate cl(1920, 1080, 1280, 720, 20, 20);
	cl.initcl();
	cl.initkernel();

	// 循环调用
	while(!quit) {
		cl.blend(mb1, mb2);
		print_fps("blend fps:");	
	}
	

	out:
	RK_MPI_MB_ReleaseBuffer(mb1);
	RK_MPI_MB_ReleaseBuffer(mb2);
	fclose(in1_fp);
	fclose(in2_fp);

}

void *rotate_proc(void *param) {
	// 读文件
	const char *in1 = "1080_nv12.yuv";

    FILE *in1_fp = fopen(in1 , "rb");
    if(!in1_fp) {
        printf("ERROR to open infile\n");
        return NULL;
    }

	// 分配drm内存 
	MB_IMAGE_INFO_S disp_info1 = {w1, h1, w1, h1, IMAGE_TYPE_NV12};
	MB_IMAGE_INFO_S disp_info2 = {h1, w1, h1, w1, IMAGE_TYPE_NV12};
	MEDIA_BUFFER mb1 = RK_MPI_MB_CreateImageBuffer(&disp_info1, RK_TRUE, 0); 
	if (!mb1) {
		printf("ERROR: no space left!1\n");
		return NULL;
	}

	MEDIA_BUFFER mb2 = RK_MPI_MB_CreateImageBuffer(&disp_info2, RK_TRUE, 0); 
	if (!mb1) {
		printf("ERROR: no space left!2\n");
		return NULL;
	}

    unsigned char *src1     = (unsigned char *)RK_MPI_MB_GetPtr(mb1);
    unsigned char *src2     = (unsigned char *)RK_MPI_MB_GetPtr(mb2);
    unsigned char *result   = src2;;

	int fd_mb1 = RK_MPI_MB_GetFD(mb1);
	int fd_mb2 = RK_MPI_MB_GetFD(mb2);

	// 加载图片
	fread(src1, BIG_SIZE, 1, in1_fp);

	printf("=================>after read img\n");



	// opencl 初始化
	ClOperate cl(1920, 1080);
	cl.initcl();
	cl.initkernel();

	// 循环调用
	while(!quit) {
		cl.rotate(mb1, mb2);
		print_fps("rotate fps:");	
	}
	
	out:
	RK_MPI_MB_ReleaseBuffer(mb1);
	RK_MPI_MB_ReleaseBuffer(mb2);
	fclose(in1_fp);
}



int main(int argc, char** argv) {
	signal(SIGINT, sigterm_handler);

	pthread_t blend_thread;
	pthread_create(&blend_thread, NULL, blend_proc, NULL);

	pthread_t rotate_thread;
	pthread_create(&rotate_thread, NULL, rotate_proc, NULL);

	while(!quit) {
		usleep(10*1000);
	}

	sleep(1);

	pthread_join(blend_thread, NULL);
	pthread_join(rotate_thread, NULL);
}
