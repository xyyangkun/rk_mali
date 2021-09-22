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
#include <assert.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

///
//  Constants
//
// const int ARRAY_SIZE = 1000;
//const int ARRAY_SIZE = 100;
//const int ARRAY_SIZE = 1920*1080*3/2;

// 处理数据的格式为yuv
// 这个程序使用opencl在1920x1080的(20, 20)位置上叠加一个720x576的数据
const int w1 = 1920;
const int h1 = 1080;
const int BIG_SIZE = w1*h1*3/2;

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
             cl_program program, cl_kernel kernel, cl_mem memObjects[2])
{
    for (int i = 0; i < 2; i++)
    {
        if (memObjects[i] != 0)
            clReleaseMemObject(memObjects[i]);
    }
    if (commandQueue != 0)
        clReleaseCommandQueue(commandQueue);

    if (kernel != 0)
        clReleaseKernel(kernel);

    if (program != 0)
        clReleaseProgram(program);

    if (context != 0)
        clReleaseContext(context);

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
    cl_kernel kernel = 0;
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
    program = CreateProgram(context, device, "Rotate.cl");
    if (program == NULL)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

    // Create memory objects that will be used as arguments to
    // kernel.  First create host memory arrays that will be
    // used to store the arguments to the kernel
    unsigned char *src = (unsigned char *)malloc(BIG_SIZE);
    unsigned char *dst = (unsigned char *)malloc(BIG_SIZE);
	assert(src != NULL);
	assert(dst != NULL);

	// 加载图片
	fread(src, BIG_SIZE, 1, in1_fp);

	printf("=================>after read img\n");

    if (!CreateMemObjects(context, memObjects, src, dst))
    {
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

	printf("=================>after  create objs\n");


	{

	// 合成Y
    // Create OpenCL kernel
    kernel = clCreateKernel(program, "rotate_y", NULL);
    if (kernel == NULL)
    {
        std::cerr << "Failed to create kernel" << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1; 
	}

    // Set the kernel arguments (w ,h ,....)
    errNum  = clSetKernelArg(kernel, 0, sizeof(int), &w1);
    errNum |= clSetKernelArg(kernel, 1, sizeof(int), &h1);
    errNum |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &memObjects[0]);
    errNum |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &memObjects[1]);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error setting kernel arguments." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }
	printf("=================>after  set param\n");

    size_t globalWorkSize[2] = { h1, w1};
    size_t localWorkSize[2] = { 1, 1 };
	size_t dim = 2;

    // Queue the kernel up for execution across the array
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel, dim, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error queuing kernel for execution." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

	printf("=================>after call cl\n");

	clReleaseKernel(kernel);
	}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
	//if(0)
	{
	// 合成UV
    // Create OpenCL kernel
    kernel = clCreateKernel(program, "rotate_uv", NULL);
    if (kernel == NULL)
    {
        std::cerr << "Failed to create kernel" << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }
	const int w11 = w1/2;
	const int h11 = h1/2;
    // Set the kernel arguments (w ,h ,....)
    errNum  = clSetKernelArg(kernel, 0, sizeof(int), &w11);
    errNum |= clSetKernelArg(kernel, 1, sizeof(int), &h11);
    errNum |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &memObjects[0]);
    errNum |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &memObjects[1]);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error setting kernel arguments." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }
	printf("=================>after  set param\n");

    // size_t globalWorkSize[2] = { ARRAY_SIZE/10,  ARRAY_SIZE/10};
    size_t globalWorkSize[2] = { h11, w11};
    size_t localWorkSize[2] = { 1, 1 };
	size_t dim = 2;

    // Queue the kernel up for execution across the array
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel, dim, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error queuing kernel for execution." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

	printf("=================>after call cl\n");

	}




///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////




#if 1
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

	printf("=================>after get buf\n");
	// 写入数据
	fwrite(dst, BIG_SIZE, 1, out_fp);

    std::cout << std::endl;
    std::cout << "Executed program succesfully." << std::endl;
    Cleanup(context, commandQueue, program, kernel, memObjects);

out:
	free(src);
	free(dst);
	fclose(in1_fp);
	fclose(out_fp);

    return 0;
}
