#pragma OPENCL EXTENSION cl_amd_printf : enable
__kernel void rotate_y(
					    const int w1,
						const int h1,
						__global unsigned char *src,
						__global unsigned char *dst
						)
{
    int h = get_global_id(0);
    int w = get_global_id(1);
	//printf("w=%d, h=%d\n", w, h);
	//printf("%d == %d\n", (h+y)*w1 + x + w, h*w2 + w);

	// 这个是复制
	// dst[w1*h + w] = src [w1*h+w];

	// 镜像
	dst[w*h1 + h] = src [w1*(h1 - h)+w];

	//  90度
	//dst[(w1-w)*h1 + h] = src [w1*h+w];
}

// 这个和 cpu 上实现不同的地方是注意width, height,
// opencl 中参数是 width/2  height/2
__kernel void rotate_uv(
					    const int width,
						const int height,
						__global unsigned char *src,
						__global unsigned char *dst
						)
{
    int h = get_global_id(0);
    int w = get_global_id(1);

	// 复制颜色 
	//dst[w1 * h + 2 * w    ] = src [w1 * h + 2 * w];
	//dst[w1 * h + 2 * w + 1] = src [w1 * h + 2 * w + 1];

	// 镜像
	dst[2073600 + w * height*2 + 2 * h    ] = src [2073600 + width*2 * (height - h) + 2 * w];
	dst[2073600 + w * height*2 + 2 * h + 1] = src [2073600 + width*2 * (height - h) + 2 * w + 1];

	// 90度
	//dst[2073600 + (w1 - w) * h1 + 2 * h    ] = src [2073600 + w1 * h + 2 * w];
	//dst[2073600 + (w1 - w) * h1 + 2 * h + 1] = src [2073600 + w1 * h + 2 * w + 1];
	//printf("%d %d %d %d==>%d %d\n", w, h, width, height, (w * height*2 + 2 * h), (width*2 * (height - h) + 2 * w));
}


