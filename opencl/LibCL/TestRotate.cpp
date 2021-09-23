#include "LibCL.h"
const int w1 = 1920;
const int h1 = 1080;
const int BIG_SIZE = w1*h1*3/2;



int main(int argc, char** argv) {
	// 读文件
	const char *in1 = "1080_nv12.yuv";
	const char *out = "1920x1080_nv12_out.yuv";
	if(argc == 3) {
		in1 = argv[1];
		out = argv[2];
	}

    FILE *in1_fp = fopen(in1 , "rb");
    if(!in1_fp) {
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
	MB_IMAGE_INFO_S disp_info2 = {h1, w1, h1, w1, IMAGE_TYPE_NV12};
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
    unsigned char *result   = src2;;


	// 加载图片
	fread(src1, BIG_SIZE, 1, in1_fp);

	printf("=================>after read img\n");

	static struct timeval t_new, t_old;


	// opencl 初始化
	ClOperate cl(w1, h1);
	cl.initcl();
	cl.initkernel();

	cl.rotate(mb1, mb2);
	
	// 写文件
	fwrite(result, BIG_SIZE, 1, out_fp);

	out:
	RK_MPI_MB_ReleaseBuffer(mb1);
	RK_MPI_MB_ReleaseBuffer(mb2);
	fclose(in1_fp);
	fclose(out_fp);


}
