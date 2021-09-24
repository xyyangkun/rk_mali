/******************************************************************************
 *
 *       Filename:  LibCL.h
 *
 *    Description:  libcl
 *
 *        Version:  1.0
 *        Created:  2021年09月18日 16时50分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef _LIBCL_H_
#define _LIBCL_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <sys/time.h>

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
#endif


class ClOperate
{
public:
ClOperate(int w1, int h1, int w2, int h2, int x, int y, int type = 0);
ClOperate(int w1, int h1);
~ClOperate();
int initcl();
int deinitcl();
int initkernel();
int deinitkernel();

int init_blend_table();
int deinit_blend_table();

int blend(MEDIA_BUFFER mb1, MEDIA_BUFFER mb2);
int blend_1d(MEDIA_BUFFER mb1, MEDIA_BUFFER mb2);

int rotate(MEDIA_BUFFER mb1, MEDIA_BUFFER mb2);

private:
void Cleanup(cl_context context, cl_command_queue commandQueue,
             cl_program program, cl_kernel kernel[2], cl_mem memObjects[2]);
cl_program CreateProgram(cl_context context, cl_device_id device, const char* fileName);
cl_command_queue CreateCommandQueue(cl_context context, cl_device_id *device);

cl_context CreateContext();

private:
	unsigned int w1,h1, w2,h2, x, y;
	int BIG_SIZE;
	int LIT_SIZE;
	int type;

	cl_uint one;
	cl_event event;

	cl_context context ;
    cl_command_queue commandQueue;
    cl_program program ;
    cl_device_id device ;
    cl_kernel kernel[2] ;
    cl_mem memObjects[2];
    cl_int errNum;

	cl_mem tableObjects[2] ;

};
#endif//_LIBCL_H_
