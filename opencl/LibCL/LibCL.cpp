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
ClOperate::ClOperate(int w1, int h1, int w2, int h2, int x, int y)
{
	this->w1 = w1;
	this->h1 = h1;
	this->w2 = w2;
	this->h2 = h2;
	this->x = x;
	this->y = y;

	type = 0;
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
    // Set the kernel arguments (w ,h ,....)
    errNum  = clSetKernelArg(kernel[1], 0, sizeof(int), &w11);
    errNum |= clSetKernelArg(kernel[1], 1, sizeof(int), &h11);
    errNum |= clSetKernelArg(kernel[1], 2, sizeof(cl_mem), &memObjects[0]);
    errNum |= clSetKernelArg(kernel[1], 3, sizeof(cl_mem), &memObjects[1]);
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
