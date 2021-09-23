#pragma OPENCL EXTENSION cl_amd_printf : enable
__kernel void rotate_y(
						__global const unsigned int *table,
						__global const unsigned char *src1,
						__global unsigned       char *src2
						)
{
    int index = get_global_id(0);
	src2[index] = src1[table[index]];
}

__kernel void rotate_uv(
						__global const unsigned int *table,
						const int offset,
						__global const unsigned char *src1,
						__global unsigned       char *src2
						)
{
    int index = get_global_id(0);

	src2[index + offset] = src1[table[index]];
}


