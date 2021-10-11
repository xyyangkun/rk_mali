/******************************************************************************
 *
 *       Filename:  LibCL.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2021年09月18日 16时49分47秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include "LibCL.h"
cl_context ClOperate::CreateContext()
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
cl_command_queue ClOperate::CreateCommandQueue(cl_context context, cl_device_id *device)
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
cl_program ClOperate::CreateProgram(cl_context context, cl_device_id device, const char* fileName)
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
//  Cleanup any created OpenCL resources
//
void ClOperate::Cleanup(cl_context context, cl_command_queue commandQueue,
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

// blend
// type = 0 普通blend type = 1查表优化blend
ClOperate::ClOperate(int w1, int h1, int w2, int h2, int x, int y, int type/*=0*/)
{
	this->w1 = w1;
	this->h1 = h1;
	this->w2 = w2;
	this->h2 = h2;
	this->x = x;
	this->y = y;
	this->type = type/*default = 0*/;

	one = 1;
	context = 0;
    commandQueue = 0;
    program = 0;
    device = 0;
    kernel[0] = 0;
    kernel[1] = 0;
    memObjects[0] = 0;
    memObjects[1] = 0;
	BIG_SIZE = w1*h1*3/2;
	LIT_SIZE = w2*h2*3/2;

	tableObjects[0] = 0;
	tableObjects[1] = 0;
}

// rotate
ClOperate::ClOperate(int w1, int h1)
{

	this->w1 = w1;
	this->h1 = h1;

	type = 1;
	one = 1;
	context = 0;
    commandQueue = 0;
    program = 0;
    device = 0;
    kernel[0] = 0;
    kernel[1] = 0;
    memObjects[0] = 0;
    memObjects[1] = 0;
	BIG_SIZE = w1*h1*3/2;
}

ClOperate::~ClOperate()
{
	deinit_blend_table();
	deinitcl();
}

int ClOperate::initcl() {
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
    program = CreateProgram(context, device, "LibCL.cl");
    if (program == NULL)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }


}
int ClOperate::deinitcl() {
	Cleanup(context, commandQueue, program, kernel, memObjects);
}

int ClOperate::initkernel() {

	if(type == 0) { // blend
		// Create OpenCL kernel y
		kernel[0] = clCreateKernel(program, "blend_y", NULL);
		if (kernel[0] == NULL)
		{
			std::cerr << "Failed to create kernel[0]" << std::endl;
			Cleanup(context, commandQueue, program, kernel, memObjects);
			return 1; 
		}

		// Create OpenCL kernel uv
		kernel[1] = clCreateKernel(program, "blend_uv", NULL);
		if (kernel[1] == NULL)
		{
			std::cerr << "Failed to create kernel[1]" << std::endl;
			Cleanup(context, commandQueue, program, kernel, memObjects);
			return 1; 
		}
	}else if(type == 1) {
		// Create OpenCL kernel y
		kernel[0] = clCreateKernel(program, "rotate_y", NULL);
		if (kernel[0] == NULL)
		{
			std::cerr << "Failed to create kernel[0]" << std::endl;
			Cleanup(context, commandQueue, program, kernel, memObjects);
			return 1; 
		}

		// Create OpenCL kernel uv
		kernel[1] = clCreateKernel(program, "rotate_uv", NULL);
		if (kernel[1] == NULL)
		{
			std::cerr << "Failed to create kernel[1]" << std::endl;
			Cleanup(context, commandQueue, program, kernel, memObjects);
			return 1; 
		}

	}else if(type == 2) {
		// Create OpenCL kernel y
		kernel[0] = clCreateKernel(program, "blend_1d_y", NULL);
		if (kernel[0] == NULL)
		{
			std::cerr << "Failed to create kernel[0]" << std::endl;
			Cleanup(context, commandQueue, program, kernel, memObjects);
			return 1; 
		}

		// Create OpenCL kernel uv
		kernel[1] = clCreateKernel(program, "blend_1d_uv", NULL);
		if (kernel[1] == NULL)
		{
			std::cerr << "Failed to create kernel[1]" << std::endl;
			Cleanup(context, commandQueue, program, kernel, memObjects);
			return 1; 
		}

	} else {
		assert(1);
	}

}

int ClOperate::deinitkernel() {

}

int ClOperate::blend(MEDIA_BUFFER mb1, MEDIA_BUFFER mb2) {
    unsigned char *src1     = (unsigned char *)RK_MPI_MB_GetPtr(mb1);
    unsigned char *src2     = (unsigned char *)RK_MPI_MB_GetPtr(mb2);
    unsigned char *result   = src1;;

	int fd_mb1 = RK_MPI_MB_GetFD(mb1);
	int fd_mb2 = RK_MPI_MB_GetFD(mb2);

	// 使用arm drm内存进行zero-copy操作
	cl_int error = CL_SUCCESS;
	const cl_import_properties_arm import_properties[] =
	{
		CL_IMPORT_TYPE_ARM,
		//CL_IMPORT_TYPE_HOST_ARM,
		CL_IMPORT_TYPE_DMA_BUF_ARM,
		0
	};

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
										LIT_SIZE,
										&error);
	if( error != CL_SUCCESS ) {
		printf("ERROR to IMPORT MEM ARM2\n");
		return -1;
	}

	// 合成Y
	{
	// Set the kernel arguments (w ,h ,....)
    errNum  = clSetKernelArg(kernel[0], 0, sizeof(int), &w1);
    errNum |= clSetKernelArg(kernel[0], 1, sizeof(int), &h1);
    errNum |= clSetKernelArg(kernel[0], 2, sizeof(int), &w2);
    errNum |= clSetKernelArg(kernel[0], 3, sizeof(int), &h2);
    errNum |= clSetKernelArg(kernel[0], 4, sizeof(int), &x);
    errNum |= clSetKernelArg(kernel[0], 5, sizeof(int), &y);
    errNum |= clSetKernelArg(kernel[0], 6, sizeof(cl_mem), &memObjects[0]);
    errNum |= clSetKernelArg(kernel[0], 7, sizeof(cl_mem), &memObjects[1]);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error setting kernel arguments." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }
	//printf("=================>after  set param\n");



    size_t globalWorkSize[2] = { h2, w2};
    size_t localWorkSize[2] = { 10, 10 };
	size_t dim = 2;

    // Queue the kernel up for execution across the array
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel[0], dim, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error y queuing kernel for execution." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }



	//clReleaseKernel(kernel[0]);
	}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
	if(1)
	{
	// 合成UV
     // Set the kernel arguments (w ,h ,....)
    errNum  = clSetKernelArg(kernel[1], 0, sizeof(int), &w1);
    errNum |= clSetKernelArg(kernel[1], 1, sizeof(int), &h1);
    errNum |= clSetKernelArg(kernel[1], 2, sizeof(int), &w2);
    errNum |= clSetKernelArg(kernel[1], 3, sizeof(int), &h2);
    errNum |= clSetKernelArg(kernel[1], 4, sizeof(int), &x);
    errNum |= clSetKernelArg(kernel[1], 5, sizeof(int), &y);
    errNum |= clSetKernelArg(kernel[1], 6, sizeof(cl_mem), &memObjects[0]);
    errNum |= clSetKernelArg(kernel[1], 7, sizeof(cl_mem), &memObjects[1]);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error setting kernel arguments." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }
	//printf("=================>after  set param\n");

    // size_t globalWorkSize[2] = { ARRAY_SIZE/10,  ARRAY_SIZE/10};
    size_t globalWorkSize[2] = { h2/2, w2/2};
    size_t localWorkSize[2] = { 10, 10 };
	size_t dim = 2;

    // Queue the kernel up for execution across the array
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel[1], dim, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, &event);
                                    //0, NULL, NULL);

	//clFinish(commandQueue);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error uv queuing kernel for execution." << std::endl;
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


	return 0;

}

int ClOperate::rotate(MEDIA_BUFFER mb1, MEDIA_BUFFER mb2) {
	unsigned char *src     = (unsigned char *)RK_MPI_MB_GetPtr(mb1);
    unsigned char *dst     = (unsigned char *)RK_MPI_MB_GetPtr(mb2);
    unsigned char *result   = dst;;

	int fd_mb1 = RK_MPI_MB_GetFD(mb1);
	int fd_mb2 = RK_MPI_MB_GetFD(mb2);

	// 使用arm drm内存进行zero-copy操作
	cl_int error = CL_SUCCESS;
	const cl_import_properties_arm import_properties[] =
	{
		CL_IMPORT_TYPE_ARM,
		//CL_IMPORT_TYPE_HOST_ARM,
		CL_IMPORT_TYPE_DMA_BUF_ARM,
		0
	};

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
    
	{
	// 合成Y
    // Set the kernel arguments (w ,h ,....)
    errNum  = clSetKernelArg(kernel[0], 0, sizeof(int), &w1);
    errNum |= clSetKernelArg(kernel[0], 1, sizeof(int), &h1);
    errNum |= clSetKernelArg(kernel[0], 2, sizeof(cl_mem), &memObjects[0]);
    errNum |= clSetKernelArg(kernel[0], 3, sizeof(cl_mem), &memObjects[1]);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error setting kernel arguments." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }
	//printf("=================>after  set param\n");

    size_t globalWorkSize[2] = { h1, w1};
    size_t localWorkSize[2] = { 10, 10 };
	size_t dim = 2;

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
	{
	// 合成UV
	const unsigned int w11 = w1/2;
	const unsigned int h11 = h1/2;
	const int offset = w1*h1;
    // Set the kernel arguments (w ,h ,....)
    errNum  = clSetKernelArg(kernel[1], 0, sizeof(int), &w11);
    errNum |= clSetKernelArg(kernel[1], 1, sizeof(int), &h11);
    errNum |= clSetKernelArg(kernel[1], 2, sizeof(cl_mem), &memObjects[0]);
    errNum |= clSetKernelArg(kernel[1], 3, sizeof(cl_mem), &memObjects[1]);
    errNum |= clSetKernelArg(kernel[1], 4, sizeof(int), &offset);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error setting kernel arguments." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }
	//printf("=================>after  set param\n");

    // size_t globalWorkSize[2] = { ARRAY_SIZE/10,  ARRAY_SIZE/10};
    size_t globalWorkSize[2] = { h11, w11};
    size_t localWorkSize[2] = { 10, 10 };
	size_t dim = 2;

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

	return 0;
}

static int *table1 = NULL;
static int *table2 = NULL;
MEDIA_BUFFER mb_table1 = 0;
MEDIA_BUFFER mb_table2 = 0;
int fd_mb_table1;
int fd_mb_table2;
void delTable() {

	if(mb_table1) {
		RK_MPI_MB_ReleaseBuffer(mb_table1);
		table1 = NULL;
	}
	if(mb_table2) {
		RK_MPI_MB_ReleaseBuffer(mb_table2);
		table2 = NULL;
	}
}
// 实现查找表查找表
void createBlendTable(unsigned int w1, unsigned int h1, unsigned int x, unsigned int y, 
						unsigned int w2, unsigned int h2) {
	assert(table1 == NULL);
	assert(table2 == NULL);

	MB_IMAGE_INFO_S disp_info1 = {w2, h2, w2, h2, IMAGE_TYPE_ARGB8888};
	MB_IMAGE_INFO_S disp_info2 = {w2, h1/2, w2, h2/2, IMAGE_TYPE_ARGB8888};

	MEDIA_BUFFER mb_table1 = RK_MPI_MB_CreateImageBuffer(&disp_info1, RK_TRUE, 0); 
	if (!mb_table1) {
		printf("ERROR: no space left!1\n");
		return ;
	}
	MEDIA_BUFFER mb_table2 = RK_MPI_MB_CreateImageBuffer(&disp_info2, RK_TRUE, 0); 
	if (!mb_table2) {
		printf("ERROR: no space left!2\n");
		return ;
	}
	fd_mb_table1 = RK_MPI_MB_GetFD(mb_table1);
	fd_mb_table2 = RK_MPI_MB_GetFD(mb_table2);
	table1 = (int *)RK_MPI_MB_GetPtr(mb_table1);
	table2 = (int *)RK_MPI_MB_GetPtr(mb_table2);

	printf("will create y table\n");

	int wh = w1 * h1;
	//旋转Y
	int k = 0;
	for(int h = 0; h<h2; h++){
		for(int w = 0; w<w2; w++) {
			table1[h*w2 + w] = (h+y)*w1 + x + w;
			assert(h*w2 + w < w2*h2);
		}
	}

	k = 0;
	printf("will create uv table\n");
	for(int h = 0; h<h2/2; h++){
		for(int w = 0; w<w2/2; w++) {
			table2[h*w2 + 2*w]     = wh + (h + y/2 ) * w1 + x + 2*w; 
			table2[h*w2 + 2*w + 1] = wh + (h + y/2 ) * w1 + x + 2*w + 1;
			assert(h*w2 + 2*w + 1 < w2*h2/2);
		}
	}
	printf("after table:k=%d wh_wh/2=%d\n", k, wh+wh/2);

}
// 通过查找表示实现
static void blendByTable(char* dst, char* src, int width, int height)
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

int ClOperate::init_blend_table() {
	cl_int error = CL_SUCCESS;
	const cl_import_properties_arm import_properties[] =
	{
		CL_IMPORT_TYPE_ARM,
		//CL_IMPORT_TYPE_HOST_ARM,
		CL_IMPORT_TYPE_DMA_BUF_ARM,
		0
	};

	// 初始化表
	createBlendTable(w1, h1, x, y, w2, h2);
#if 1
	// 构造opencl table 内存
	tableObjects[0] = clImportMemoryARM(context,
										CL_MEM_READ_ONLY,
										import_properties,
										&fd_mb_table1,
										w2*h2*4,
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
										w2*h2*2,
										&error);
	if( error != CL_SUCCESS ) {
		printf("ERROR to IMPORT table MEM ARM2\n");
		return -1;
	}
#endif
	return 0;
}
int ClOperate::deinit_blend_table() {
	//  释放查找表内存
	if(tableObjects[0]) {
		clReleaseMemObject(tableObjects[0]);
		tableObjects[0] = 0;
	}

	if(tableObjects[1]) {
		clReleaseMemObject(tableObjects[1]);
		tableObjects[1] = 0;
	}

	delTable();


	return 0;
}
int ClOperate::blend_1d(MEDIA_BUFFER mb1, MEDIA_BUFFER mb2) {
    unsigned char *src1     = (unsigned char *)RK_MPI_MB_GetPtr(mb1);
    unsigned char *src2     = (unsigned char *)RK_MPI_MB_GetPtr(mb2);
    unsigned char *result   = src1;;

	int fd_mb1 = RK_MPI_MB_GetFD(mb1);
	int fd_mb2 = RK_MPI_MB_GetFD(mb2);

	// 使用arm drm内存进行zero-copy操作
	cl_int error = CL_SUCCESS;
	const cl_import_properties_arm import_properties[] =
	{
		CL_IMPORT_TYPE_ARM,
		//CL_IMPORT_TYPE_HOST_ARM,
		CL_IMPORT_TYPE_DMA_BUF_ARM,
		0
	};

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
										LIT_SIZE,
										&error);
	if( error != CL_SUCCESS ) {
		printf("ERROR to IMPORT MEM ARM2\n");
		return -1;
	}

	// 合成Y
	{
	// Set the kernel arguments (w ,h ,....)
    errNum  = clSetKernelArg(kernel[0], 0, sizeof(cl_mem), &tableObjects[0]);
    errNum |= clSetKernelArg(kernel[0], 1, sizeof(cl_mem), &memObjects[0]);
    errNum |= clSetKernelArg(kernel[0], 2, sizeof(cl_mem), &memObjects[1]);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error setting y kernel arguments." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }
	//printf("=================>after  set param\n");

    size_t globalWorkSize[1] = { h2 *w2};
    size_t localWorkSize[1] = { 200 };
	size_t dim = 1;

    // Queue the kernel up for execution across the array
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel[0], dim, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error y queuing kernel for execution." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }


	//clReleaseKernel(kernel[0]);
	}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
	if(1)
	{
	// 合成UV
	int offset = w2 * h2;
     // Set the kernel arguments (w ,h ,....)
    errNum |= clSetKernelArg(kernel[1], 0, sizeof(cl_mem), &tableObjects[1]);
    errNum |= clSetKernelArg(kernel[1], 1, sizeof(int), &offset);
    errNum |= clSetKernelArg(kernel[1], 2, sizeof(cl_mem), &memObjects[0]);
    errNum |= clSetKernelArg(kernel[1], 3, sizeof(cl_mem), &memObjects[1]);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error setting uv kernel arguments." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }
	//printf("=================>after  set param\n");

    size_t globalWorkSize[1] = { w2*h2/2};
    size_t localWorkSize[1] = { 200 };
	size_t dim = 1;

    // Queue the kernel up for execution across the array
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel[1], dim, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, &event);
                                    //0, NULL, NULL);

	//clFinish(commandQueue);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error uv queuing kernel for execution." << std::endl;
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


	return 0;
}
