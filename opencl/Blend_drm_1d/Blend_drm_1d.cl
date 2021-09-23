//#pragma OPENCL EXTENSION cl_arm_printf : enable
__kernel void blend_y(
						__global unsigned int *table,
						__global unsigned char *src1,
						__global const unsigned char *src2
						)
{
    int index = get_global_id(0);
	src1[table[index]] = src2 [index];
	//printf("index=%d table:%d\n", index, table[index]);
}

__kernel void blend_uv(
						__global unsigned int *table,
						const int offset,
						__global unsigned char *src1,
						__global const unsigned char *src2
						)
{
    int index = get_global_id(0);

	src1[table[index]] = src2[index + offset];
}



