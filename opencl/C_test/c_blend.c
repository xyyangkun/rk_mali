/******************************************************************************
 *
 *       Filename:  c_blend.c
 *
 *    Description:  c 叠加算法验证 
 *
 *        Version:  1.0
 *        Created:  2021年09月23日 10时06分27秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// yuv420 需要保证 横竖长度都是2字节对齐的


void blend(char *src1, int w1, int h1, char *src2, int x, int y, int w2, int h2) 
{
	int wh1 = w1*h1;
	int wh2 = w2*h2;

	printf("will blend, w1=%d h1=%d x=%d, y=%d w2=%d h2=%d\n",
			w1, h1, x, y, w2, h2);
	// copy y
	for(int h = 0; h<h2; h++){
		for(int w = 0; w<w2; w++) {
			src1[(h+y)*w1 + x + w] = src2 [ h*w2 + w];
		}
	}
	printf("will copy uv\n");

	// copy uv
	for(int h = 0; h<h2/2; h++){
		for(int w = 0; w<w2; w+=2) {
			src1[wh1 + (h + y / 2 ) * w1 + x + w    ] = src2 [wh2 +  h*w2 + w];
			src1[wh1 + (h + y / 2 ) * w1 + x + w + 1] = src2 [wh2 +  h*w2 + w + 1];
			assert(wh1 + (h+y/2)*w1 + x + w <wh1*3/2);
		}
	}
	printf("copy over\n");

}

static int *table1 = NULL;
static int *table2 = NULL;

// 顺时针方向旋转
void delTable() {
	free(table1);
	free(table2);
	table1 = NULL;
	table2 = NULL;
}
// 实现查找表查找表
void createTable(int w1, int h1, int x, int y, int w2, int h2) {
	assert(table1 == NULL);
	assert(table2 == NULL);
	int wh = w1 * h1;

	table1 = (int *)malloc(wh*sizeof(int));
	table2 = (int *)malloc(wh/2*sizeof(int));

	printf("will create y table\n");
	//旋转Y
	int k = 0;
	for(int h = 0; h<h2; h++){
		for(int w = 0; w<w2; w++) {
			table1[h*w2 + w] = (h+y)*w1 + x + w;
		}
	}

	k = 0;
	printf("will create uv table\n");
	for(int h = 0; h<h2/2; h++){
		for(int w = 0; w<w2; w+=2) {
			table2[h*w2 + w]     = wh + (h + y / 2 ) * w1 + x + w; 
			table2[h*w2 + w + 1] = wh + (h + y / 2 ) * w1 + x + w + 1;
		}
	}
	printf("after table:k=%d wh_wh/2=%d\n", k, wh+wh/2);

}
// 通过查找表示实现
static void blendTable(char* dst, char* src, int width, int height)
{

	int wh = width * height;
	printf("will rotate y\n");
	//旋转Y
	for(int i=0;i<wh;i++) {
		dst[table1[i]] = src[i];
	}

	printf("will rotate uv\n");
	for(int i=0;i<wh/2;i++) {
		dst[table2[i]] = src[wh + i]; 
	}
	printf("after rotate uv\n");
}


int main(int argc, char **argv) {
	char *in1 = "1920x1080_nv12.yuv";
	// char *in2 = "720x576_nv12.yuv";
	char *in2 = "720p_nv12.yuv";
	char *out = "1920x1080_nv12_blendout.yuv";

	if(argc == 3) {
		in1 = argv[1];
		in2 = argv[2];
	}

	int w1 = 1920;
	int h1 = 1080;
	int size1 = w1*h1*3/2;

	// int w2 = 720;
	//int h2 = 576;
	int w2 = 1280;
	int h2 = 720;
	int size2 = w2*h2*3/2;



	char *src1 = (char *)malloc(size1);
	char *src2 = (char *)malloc(size2);
	char *src_table = (char *)malloc(size1);

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
	fread(src1, size1, 1, in1_fp);
	memcpy(src_table, src1, size1);
	fread(src2, size2, 1, in2_fp);

	int x  = 100;
	int y = 100;
	
	printf("before blend in1=%s in2=%s\n", in1, in2);
	// transfer
	blend(src1, w1, h1, src2, x, y, w2, h2);
	createTable(w1, h1, x, y, w2, h2);
	blendTable(src_table, src2, w2, h2);
	delTable();

	// 对比两次叠加是否相同
	assert(0 == memcmp(src1, src_table, size1));

	// fwrite(src1, size1, 1, out_fp);
	fwrite(src_table, size1, 1, out_fp);

	free(src1);	
	free(src2);	
	fclose(in1_fp);
	fclose(in2_fp);
	fclose(out_fp);
}
