/******************************************************************************
 *
 *       Filename:  c_rotate.c
 *
 *    Description: 使用c 验证 rotate算法 
 *
 *        Version:  1.0
 *        Created:  2021年09月23日 10时04分36秒
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
#include <assert.h>
#include <string.h>

// 这个是镜像
static void rotateYUV420SP_mirror(char* src,char* dst, int width,int height)
{

	int wh = width * height;
	//旋转Y
	int k = 0;
	for(int i=0;i<width;i++) {
		for(int j=0;j<height;j++) 
		{
			dst[k] = src[width*j + i];   
			k++;
		}
	}

	for(int i=0;i<width;i+=2) {
		for(int j=0;j<height/2;j++) 
		{ 
			dst[k] = src[wh+ width*j + i]; 
			dst[k+1]=src[wh + width*j + i+1];
			k+=2;
		}
	}
}
// 这个是逆时针转90度
static void rotateYUV420SP_270(char* src,char* dst, int width,int height)
{
	int t;
	int wh = width * height;
	//旋转Y
	int k = 0;
	for(int i=0;i<width;i++) {
		for(int j=0;j<height;j++) 
		{
			dst[k] = src[width*j + (width-i)];   
			k++;
		}
	}

	for(int i=0;i<width;i+=2) {
		for(int j=0;j<height/2;j++) 
		{ 
			dst[k] = src[wh+  width*j + (width-i)]; 
			dst[k+1]=src[wh + width*j + (width-i)+1];
			k+=2;
		}
	}
}

#if 1
// 顺时针方向旋转 更适应opencl
static void rotateYUV420SP_90(char* src,char* dst, int width,int height)
{

	int wh = width * height;
	//旋转Y
	for(int i=0;i<width;i++) {
		for(int j=0;j<height;j++) 
		{
			dst[height*i + j] = src[width*(height-j) + i];   
		}
	}
#if 1
	//unsigned short *_s = (unsigned short *)(src + 1920*1080);
	//unsigned short *_d = (unsigned short *)(dst + 1920*1080);

	unsigned char *_s = (unsigned char *)(src + 1920*1080);
	unsigned char *_d = (unsigned char *)(dst + 1920*1080);
	for(int w=0;w<width/2;w++) {
		for(int h=0;h<height/2;h++) 
		{ 
			// _d[height*i + 2*j] = _s[width*j + 2*i];
			// _d[height*i + 2*j +1] = _s[width*j + 2*i + 1];
			_d[height*w + 2*h]    = _s[width*(height/2 - h) + 2*w];
			_d[height*w + 2*h +1] = _s[width*(height/2 - h) + 2*w + 1];

			//_d[height*i/2 + j] = _s[width*j/2 + i];
			//printf("w=%d h=%d\n", w, h);
			// printf("%d %d %d %d==>%d %d\n", w, h, width, height, height*w + 2*h, width*(height/2 - h) + 2*w);
		}
	}
#endif
}
#endif


#if 0
// 顺时针方向旋转
static void rotateYUV420SP(char* src,char* dst, int width,int height)
{

	int wh = width * height;
	//旋转Y
	int k = 0;
	for(int i=0;i<width;i++) {
		for(int j=0;j<height;j++) 
		{
			dst[k] = src[width*(height-j) + i];   
			k++;
		}
	}

	for(int i=0;i<width;i+=2) {
		for(int j=0;j<height/2;j++) 
		{ 
			dst[k] = src[wh+ width*(height/2-j) + i]; 
			dst[k+1]=src[wh + width*(height/2-j) + i+1];
			k+=2;
		}
	}
}
#endif
// 以上可以通过查找表优化计算时间

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
void createTable_270(int width, int height) {
	assert(table1 == NULL);
	assert(table2 == NULL);
	int wh = width * height;

	table1 = (int *)malloc(wh*sizeof(int));
	table2 = (int *)malloc(wh/2*sizeof(int));

	printf("will create y table\n");
	//旋转Y
	int k = 0;
	for(int i=0;i<width;i++) {
		for(int j=0;j<height;j++) 
		{
			table1[k] = width*j + (width-i);   
			k++;
		}
	}
	assert(k <= wh);

	k = 0;
	printf("will create uv table\n");
	for(int i=0;i<width;i+=2) {
		for(int j=0;j<height/2;j++) 
		{ 
			table2[k]   = wh + width*j + (width-i); 
			table2[k+1] = wh + width*j + (width-i)+1;
			k+=2;
		}
	}
	assert(k <= wh+wh/2);
	printf("after table:k=%d wh_wh/2=%d\n", k, wh+wh/2);

}
void createTable_90(int width, int height) {
	assert(table1 == NULL);
	assert(table2 == NULL);
	int wh = width * height;

	table1 = (int *)malloc(wh*sizeof(int));
	table2 = (int *)malloc(wh/2*sizeof(int));

	printf("will create y table\n");
	//旋转Y
	int k = 0;
	for(int i=0;i<width;i++) {
		for(int j=0;j<height;j++) 
		{
			table1[k] = width*(height-j) + i;   
			k++;
		}
	}
	assert(k <= wh);

	k = 0;
	printf("will create uv table\n");
	for(int i=0;i<width;i+=2) {
		for(int j=0;j<height/2;j++) 
		{ 
			table2[k]   = wh + width*(height/2-j) + i; 
			table2[k+1] = wh + width*(height/2-j) + i+1;
			k+=2;
		}
	}
	assert(k <= wh+wh/2);
	printf("after table:k=%d wh_wh/2=%d\n", k, wh+wh/2);

}
// 通过查找表示实现
static void rotateYUV420SPTable(char* src,char* dst, int width,int height)
{

	int wh = width * height;
	int k = 0;
	printf("will rotate y\n");
	//旋转Y
	for(int i=0;i<wh;i++) {
			dst[k] = src[table1[k]];
			k++;
	}

	printf("will rotate uv\n");
#if 0
	for(int i=0;i<wh/2;i+=2) {
			dst[k] = src[table2[k] ]; 
			dst[k+1]=src[table2[k+1]];
			k+=2;
	}
#else
	k = 0;
	for(int i=0;i<wh/4;i++) {
		dst[wh + 2*i] = src[table2[2*i] ]; 
		dst[wh + 2*i+1]=src[table2[2*i+1]];
	}
#endif
	printf("after rotate uv\n");
}
//void delTable() {}
//void createTable(int width, int height) {}

#if 1
// rotation
int main(int argc, char **argv) {
	char *in = "1920x1080_nv12.yuv";
	char *out = "1080x1920_nv12.yuv";

	int choice = 3;
	if(argc == 2) {
		choice =atoi(argv[1]);
	}
	else if(argc == 3) {
		in = argv[1];
		out = argv[2];
	}
	else if(argc == 4) {
		in = argv[1];
		out = argv[2];
		choice = atoi(argv[3]);
	}

	const unsigned int w = 1920;
	const unsigned int h = 1080;
	const unsigned long int size = w*h*3/2;



	char *src = (char *)malloc(size);
	char *dst = (char *)malloc(size);
	char *dst_table = (char *)malloc(size);

	FILE *in_fp = fopen(in , "rb");
	if(!in_fp) {
		printf("ERROR to open infile\n");
		return -1;
	}
	FILE *out_fp = fopen(out , "wb");
	if(!out_fp) {
		printf("ERROR to open outfile\n");
		return -1;
	}
	fread(src, size, 1, in_fp);
	
	printf("will transfer\n");
	printf("will rotate\n");
	// transfer
	if(choice == 0) {
		printf("route 90\n");
		// 90度旋转
		rotateYUV420SP_90(src, dst, w, h);

		createTable_90(w, h);	
		rotateYUV420SPTable(src, dst_table, w, h);
		delTable();
		assert(0 == memcmp(dst, dst_table, size));
	} else if(choice == 1) {
		printf("route 270\n");
		// 270度旋转
		rotateYUV420SP_270(src, dst, w, h);

		createTable_270(w, h);	
		rotateYUV420SPTable(src, dst_table, w, h);
		delTable();
		assert(0 == memcmp(dst, dst_table, size));
	} else {
		printf("route 90 and 270\n");
		// 90度旋转
		rotateYUV420SP_90(src, dst, w, h);

		createTable_90(w, h);	
		rotateYUV420SPTable(src, dst_table, w, h);
		delTable();
		assert(0 == memcmp(dst, dst_table, size));

		// 270度旋转
		rotateYUV420SP_270(src, dst, w, h);

		createTable_270(w, h);	
		rotateYUV420SPTable(src, dst_table, w, h);
		delTable();
		assert(0 == memcmp(dst, dst_table, size));
	}

	fwrite(dst, size, 1, out_fp);


	free(src);	
	free(dst);	
	fclose(in_fp);
	fclose(out_fp);
}
#endif
