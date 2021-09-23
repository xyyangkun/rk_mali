//
// Book:      OpenCL(R) Programming Guide
// Authors:   Aaftab Munshi, Benedict Gaster, Timothy Mattson, James Fung, Dan Ginsburg
// ISBN-10:   0-321-74964-2
// ISBN-13:   978-0-321-74964-2
// Publisher: Addison-Wesley Professional
// URLs:      http://safari.informit.com/9780132488006/
//            http://www.openclprogrammingguide.com
//

// HelloWorld.cpp
//
//    This is a simple example that demonstrates basic OpenCL setup and
//    use.

#include <iostream>
#include <fstream>
#include <sstream>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif


#define DRM

#ifdef DRM
//  参考
//  https://www.khronos.org/registry/OpenCL/extensions/arm/cl_arm_import_memory.txt
#include "rkmedia_api.h"
#include "CL/cl_ext.h"
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>

#endif



///
//  Constants
//
// const int ARRAY_SIZE = 1000;
//const int ARRAY_SIZE = 100;
//const int ARRAY_SIZE = 1920*1080*3/2;

// 处理数据的格式为yuv
// 这个程序使用opencl在1920x1080的(20, 20)位置上叠加一个720x576的数据
const unsigned int w1 = 1920;
const unsigned int h1 = 1080;
const unsigned int BIG_SIZE = w1*h1*3/2;

static struct timeval t_new, t_old;
///
//  Create an OpenCL context on the first available platform using
//  either a GPU or CPU depending on what is available.
//
cl_context CreateContext()
{
    cl_int errNum;
    cl_uint numPlatforms;
    cl_platform_id firstPlatformId;
    cl_context context = NULL;

    // First, select an OpenCL platform to run on.  For this example, we
    // simply choose the first available platform.  Normally, you would
    // query for all available platforms and select the most appropriate one.
    errNum = clGetPlatformIDs(1, &firstPlatformId, &numPlatforms);
    if (errNum != CL_SUCCESS || numPlatforms <= 0)
    {
        std::cerr << "Failed to find any OpenCL platforms." << std::endl;
        return NULL;
    }

    // Next, create an OpenCL context on the platform.  Attempt to
    // create a GPU-based context, and if that fails, try to create
    // a CPU-based context.
    cl_context_properties contextProperties[] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)firstPlatformId,
        0
    };
    context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU,
                                      NULL, NULL, &errNum);
    if (errNum != CL_SUCCESS)
    {
        std::cout << "Could not create GPU context, trying CPU..." << std::endl;
        context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_CPU,
                                          NULL, NULL, &errNum);
        if (errNum != CL_SUCCESS)
        {
            std::cerr << "Failed to create an OpenCL GPU or CPU context." << std::endl;
            return NULL;
        }
    }

    return context;
}

///
//  Create a command queue on the first device available on the
//  context
//
cl_command_queue CreateCommandQueue(cl_context context, cl_device_id *device)
{
    cl_int errNum;
    cl_device_id *devices;
    cl_command_queue commandQueue = NULL;
    size_t deviceBufferSize = -1;

    // First get the size of the devices buffer
    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceBufferSize);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Failed call to clGetContextInfo(...,GL_CONTEXT_DEVICES,...)";
        return NULL;
    }

    if (deviceBufferSize <= 0)
    {
        std::cerr << "No devices available.";
        return NULL;
    }

    // Allocate memory for the devices buffer
    devices = new cl_device_id[deviceBufferSize / sizeof(cl_device_id)];
    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize, devices, NULL);
    if (errNum != CL_SUCCESS)
    {
        delete [] devices;
        std::cerr << "Failed to get device IDs";
        return NULL;
    }

    // In this example, we just choose the first available device.  In a
    // real program, you would likely use all available devices or choose
    // the highest performance device based on OpenCL device queries
    commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);
    if (commandQueue == NULL)
    {
        delete [] devices;
        std::cerr << "Failed to create commandQueue for device 0";
        return NULL;
    }

    *device = devices[0];
    delete [] devices;
    return commandQueue;
}

///
//  Create an OpenCL program from the kernel source file
//
cl_program CreateProgram(cl_context context, cl_device_id device, const char* fileName)
{
    cl_int errNum;
    cl_program program;

    std::ifstream kernelFile(fileName, std::ios::in);
    if (!kernelFile.is_open())
    {
        std::cerr << "Failed to open file for reading: " << fileName << std::endl;
        return NULL;
    }

    std::ostringstream oss;
    oss << kernelFile.rdbuf();

    std::string srcStdStr = oss.str();
    const char *srcStr = srcStdStr.c_str();
    program = clCreateProgramWithSource(context, 1,
                                        (const char**)&srcStr,
                                        NULL, NULL);
    if (program == NULL)
    {
        std::cerr << "Failed to create CL program from source." << std::endl;
        return NULL;
    }

    errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        // Determine the reason for the error
        char buildLog[16384];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                              sizeof(buildLog), buildLog, NULL);

        std::cerr << "Error in kernel: " << std::endl;
        std::cerr << buildLog;
        clReleaseProgram(program);
        return NULL;
    }

    return program;
}

///
//  Create memory objects used as the arguments to the kernel
//  The kernel takes three arguments: result (output), a (input),
//  and b (input)
//
bool CreateMemObjects(cl_context context, cl_mem memObjects[2],
                      unsigned char *src1, unsigned char *dst)
{
    memObjects[0] = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                   sizeof(unsigned char) * BIG_SIZE, src1, NULL);
    memObjects[1] = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                   sizeof(unsigned char) * BIG_SIZE, dst, NULL);

    if (memObjects[0] == NULL || memObjects[1] == NULL)
    {
        std::cerr << "Error creating memory objects." << std::endl;
        return false;
    }

    return true;
}

///
//  Cleanup any created OpenCL resources
//
void Cleanup(cl_context context, cl_command_queue commandQueue,
             cl_program program, cl_kernel kernel[2], cl_mem memObjects[2])
{
#ifndef DRM
    for (int i = 0; i < 2; i++)
    {
        if (memObjects[i] != 0)
            clReleaseMemObject(memObjects[i]);
    }
#endif
    if (commandQueue != 0)
        clReleaseCommandQueue(commandQueue);


    if (kernel[0] != 0)
        clReleaseKernel(kernel[0]);
    if (kernel[1] != 0)
        clReleaseKernel(kernel[1]);

    if (program != 0)
        clReleaseProgram(program);

    if (context != 0)
        clReleaseContext(context);

}
static unsigned int *table1 = NULL;
static unsigned int *table2 = NULL;
MEDIA_BUFFER mb_table1 = 0;
MEDIA_BUFFER mb_table2 = 0;
int fd_mb_table1;
int fd_mb_table2;
void delTable() {

	RK_MPI_MB_ReleaseBuffer(mb_table1);
	RK_MPI_MB_ReleaseBuffer(mb_table2);
	table1 = NULL;
	table2 = NULL;
}

// 实现查找表查找表
void createRotateTable_270(unsigned int width, unsigned int height) {
	assert(table1 == NULL);
	assert(table2 == NULL);

	MB_IMAGE_INFO_S disp_info = {width*4, height, width*4, height, IMAGE_TYPE_ARGB8888};

	MEDIA_BUFFER mb_table1 = RK_MPI_MB_CreateImageBuffer(&disp_info, RK_TRUE, 0); 
	if (!mb_table1) {
		printf("ERROR: no space left!1\n");
		return ;
	}
	MEDIA_BUFFER mb_table2 = RK_MPI_MB_CreateImageBuffer(&disp_info, RK_TRUE, 0); 
	if (!mb_table2) {
		printf("ERROR: no space left!2\n");
		return ;
	}
	fd_mb_table1 = RK_MPI_MB_GetFD(mb_table1);
	fd_mb_table2 = RK_MPI_MB_GetFD(mb_table2);
	table1 = (unsigned int *)RK_MPI_MB_GetPtr(mb_table1);
	table2 = (unsigned int *)RK_MPI_MB_GetPtr(mb_table2);


	int wh = width * height;


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
void createRotateTable_90(unsigned int width, unsigned int height) {
	assert(table1 == NULL);
	assert(table2 == NULL);

	// 更改为short 类型
	MB_IMAGE_INFO_S disp_info = {width*4, height, width*4, height, IMAGE_TYPE_ARGB8888};

	MEDIA_BUFFER mb_table1 = RK_MPI_MB_CreateImageBuffer(&disp_info, RK_TRUE, 0); 
	if (!mb_table1) {
		printf("ERROR: no space left!1\n");
		return ;
	}
	MEDIA_BUFFER mb_table2 = RK_MPI_MB_CreateImageBuffer(&disp_info, RK_TRUE, 0); 
	if (!mb_table2) {
		printf("ERROR: no space left!2\n");
		return ;
	}
	fd_mb_table1 = RK_MPI_MB_GetFD(mb_table1);
	fd_mb_table2 = RK_MPI_MB_GetFD(mb_table2);
	table1 = (unsigned int *)RK_MPI_MB_GetPtr(mb_table1);
	table2 = (unsigned int *)RK_MPI_MB_GetPtr(mb_table2);



	int wh = width * height;

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


///
//	main() for HelloWorld example
//
int main(int argc, char** argv)
{
	const char *in1 = "1080_nv12.yuv";
	const char *out = "1080x1920_nv12_out.yuv";
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


    cl_context context = 0;
    cl_command_queue commandQueue = 0;
    cl_program program = 0;
    cl_device_id device = 0;
    cl_kernel kernel[2] = {0, 0};
    cl_mem memObjects[2] = { 0, 0};
    cl_int errNum;

    // Create an OpenCL context on first available platform
    context = CreateContext();
    if (context == NULL)
    {
        std::cerr << "Failed to create OpenCL context." << std::endl;
        return 1;
    }

    // Create a command-queue on the first device available
    // on the created context
    commandQueue = CreateCommandQueue(context, &device);
    if (commandQueue == NULL)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

    // Create OpenCL program from HelloWorld.cl kernel source
    program = CreateProgram(context, device, "Rotate_drm_1d.cl");
    if (program == NULL)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

#ifndef DRM
    // Create memory objects that will be used as arguments to
    // kernel.  First create host memory arrays that will be
    // used to store the arguments to the kernel
    unsigned char *src = (unsigned char *)malloc(BIG_SIZE);
    unsigned char *dst = (unsigned char *)malloc(BIG_SIZE);
	assert(src != NULL);
	assert(dst != NULL);
#else
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

    unsigned char *src     = (unsigned char *)RK_MPI_MB_GetPtr(mb1);
    unsigned char *dst     = (unsigned char *)RK_MPI_MB_GetPtr(mb2);
    unsigned char *result   = dst;;

	int fd_mb1 = RK_MPI_MB_GetFD(mb1);
	int fd_mb2 = RK_MPI_MB_GetFD(mb2);
#endif

	// 加载图片
	fread(src, BIG_SIZE, 1, in1_fp);

	printf("=================>after read img\n");

#ifndef DRM
    if (!CreateMemObjects(context, memObjects, src, dst))
    {
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }
#else
	// 使用arm drm内存进行zero-copy操作
	cl_int error = CL_SUCCESS;
	const cl_import_properties_arm import_properties[] =
	{
		CL_IMPORT_TYPE_ARM,
		//CL_IMPORT_TYPE_HOST_ARM,
		CL_IMPORT_TYPE_DMA_BUF_ARM,
		0
	};

#endif

	// Create OpenCL kernel
    kernel[0] = clCreateKernel(program, "rotate_y", NULL);
    if (kernel[0] == NULL)
    {
        std::cerr << "Failed to create kernel" << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1; 
	}

	// Create OpenCL kernel
    kernel[1] = clCreateKernel(program, "rotate_uv", NULL);
    if (kernel[1] == NULL)
    {
        std::cerr << "Failed to create kernel" << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }


	printf("=================>after  create objs\n");



	cl_mem tableObjects[2] = { 0, 0};
	// 初始化表
	createRotateTable_90(w1, h1);
	// 构造opencl table 内存
	tableObjects[0] = clImportMemoryARM(context,
										CL_MEM_READ_ONLY,
										import_properties,
										&fd_mb_table1,
										w1*h1*4,
										&error);
	if( error != CL_SUCCESS ) {
		printf("ERROR to IMPORT table MEM ARM1\n");
		return -1;
	}
	assert(tableObjects[0]!=0);
	tableObjects[1] = clImportMemoryARM(context,
										CL_MEM_READ_ONLY,
										import_properties,
										&fd_mb_table2,
										w1*h1*2,
										&error);
	if( error != CL_SUCCESS ) {
		printf("ERROR to IMPORT table MEM ARM2\n");
		return -1;
	}




	cl_uint one = 1;
	cl_event event;

	// int count = 10000;
	int count = 1;
	// 循环100 次计算时间
	for(int i=0; i<count; i++)
	{
gettimeofday(&t_old, 0);
#if 1
	memObjects[0] = clImportMemoryARM(context,
										CL_MEM_READ_WRITE,
										import_properties,
										&fd_mb1,
										BIG_SIZE,
										&error);
	if( error != CL_SUCCESS ) {
		printf("ERROR to IMPORT MEM ARM1\n");
		return -1;
	}
	assert(memObjects[0]!=0);
	memObjects[1] = clImportMemoryARM(context,
										CL_MEM_READ_WRITE,
										import_properties,
										&fd_mb2,
										BIG_SIZE,
										&error);
	if( error != CL_SUCCESS ) {
		printf("ERROR to IMPORT MEM ARM2\n");
		return -1;
	}
    
#endif

	{
	// 合成Y
    // Set the kernel arguments (w ,h ,....)
    errNum  = clSetKernelArg(kernel[0], 0, sizeof(cl_mem), &tableObjects[0]);
    errNum |= clSetKernelArg(kernel[0], 1, sizeof(cl_mem), &memObjects[0]);
    errNum |= clSetKernelArg(kernel[0], 2, sizeof(cl_mem), &memObjects[1]);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error setting kernel arguments." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }
	//printf("=================>after  set param\n");

    size_t globalWorkSize[1] = { h1*w1};
    size_t localWorkSize[1] = { 320 };
	size_t dim = 1;

    // Queue the kernel up for execution across the array
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel[0], dim, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error queuing kernel for execution." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

	//printf("=================>after call cl\n");

	}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
	// if(0)
	{
	// 合成UV
	int offset = w1 * h1;
    // Set the kernel arguments (w ,h ,....)
    errNum  = clSetKernelArg(kernel[1], 0, sizeof(cl_mem), &tableObjects[1]);
    errNum |= clSetKernelArg(kernel[1], 1, sizeof(int), &offset);
    errNum |= clSetKernelArg(kernel[1], 2, sizeof(cl_mem), &memObjects[0]);
    errNum |= clSetKernelArg(kernel[1], 3, sizeof(cl_mem), &memObjects[1]);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error setting kernel arguments." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }
	//printf("=================>after  set param\n");

    size_t globalWorkSize[1] = { w1*h1/2 };
    size_t localWorkSize[1] = { 300 };
	size_t dim = 1;

    // Queue the kernel up for execution across the array
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel[1], dim, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, &event);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error queuing kernel for execution." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

	//printf("=================>after call cl\n");

	}



	clReleaseMemObject(memObjects[0]);
	memObjects[0] = 0;

	clReleaseMemObject(memObjects[1]);
	memObjects[1] = 0;

	//clFinish(commandQueue);
	clWaitForEvents(one, &event);
	clReleaseEvent(event);



gettimeofday(&t_new, 0);
printf("=====>%s index:%d :%d\n","wait cl finish time:", i, (t_new.tv_sec-t_old.tv_sec)*1000000 + (t_new.tv_usec - t_old.tv_usec));
	}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////



	clFinish(commandQueue);

#if 1
#ifndef DRM
    // Read the output buffer back to the Host
    errNum = clEnqueueReadBuffer(commandQueue, memObjects[1], CL_TRUE,
                                 0, BIG_SIZE* sizeof(unsigned char), dst,
                                 0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error reading result buffer." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        //return 1;
		goto out;
    }
#endif
#endif

	printf("=================>after get buf\n");
	// 写入数据
	fwrite(dst, BIG_SIZE, 1, out_fp);

    std::cout << std::endl;
    std::cout << "Executed program succesfully." << std::endl;

#ifndef DRM
    Cleanup(context, commandQueue, program, kernel, memObjects);
#else

#endif



out:
#ifndef DRM
	free(src);
	free(dst);
#else
	RK_MPI_MB_ReleaseBuffer(mb1);
	RK_MPI_MB_ReleaseBuffer(mb2);
#endif
	fclose(in1_fp);
	fclose(out_fp);

    return 0;
}
