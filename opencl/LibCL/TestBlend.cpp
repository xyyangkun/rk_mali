#include "LibCL.h"
const int w1 = 1920;
const int h1 = 1080;
const int w2 = 1280;
const int h2 = 720;
const int x = 20;
const int y = 20;
const int BIG_SIZE = w1*h1*3/2;
const int LIT_SIZE = w2*h2*3/2;



int main(int argc, char** argv) {
	// 读文件
	const char *in1 = "1080_nv12.yuv";
	const char *in2 = "720p_nv12.yuv";
	const char *out = "1920x1080_nv12_out.yuv";
	if(argc == 3) {
		in1 = argv[1];
		in2 = argv[2];
	}

    FILE *in1_fp = fopen(in1 , "rb");
    if(!in1_fp) {
        printf("ERROR to open infile\n");
        return -1;
    }

    FILE *in2_fp = fopen(in2 , "rb");
    if(!in2_fp) {
        printf("ERROR to open infile\n");
        return -1;
    }

    FILE *out_fp = fopen(out , "wb");
    if(!out_fp) {
        printf("ERROR to open outfile\n");
        return -1;
    }

	// 分配drm内存 
	MB_IMAGE_INFO_S disp_info1 = {w1, h1, w1, h1, IMAGE_TYPE_NV12};
	MB_IMAGE_INFO_S disp_info2 = {w2, h2, w2, h2, IMAGE_TYPE_NV12};
	MEDIA_BUFFER mb1 = RK_MPI_MB_CreateImageBuffer(&disp_info1, RK_TRUE, 0); 
	if (!mb1) {
		printf("ERROR: no space left!1\n");
		return -1;
	}

	MEDIA_BUFFER mb2 = RK_MPI_MB_CreateImageBuffer(&disp_info2, RK_TRUE, 0); 
	if (!mb1) {
		printf("ERROR: no space left!2\n");
		return -1;
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

	static struct timeval t_new, t_old;


	// opencl 初始化
	ClOperate cl(1920, 1080, 1280, 720, 20, 20);
	cl.initcl();
	cl.initkernel();

	cl.blend(mb1, mb2);
	
	// 写文件
	fwrite(result, BIG_SIZE, 1, out_fp);

	out:
	RK_MPI_MB_ReleaseBuffer(mb1);
	RK_MPI_MB_ReleaseBuffer(mb2);
	fclose(in1_fp);
	fclose(in2_fp);
	fclose(out_fp);


}
