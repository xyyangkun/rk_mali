/******************************************************************************
 *
 *       Filename:  myutils.h
 *
 *    Description:  工具
 *
 *        Version:  1.0
 *        Created:  2021年09月01日 13时25分27秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef _MYUTILS_H_
#define _MYUTILS_H_

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#define print_fps(name) do {\
	static int count=0; \
	static struct timeval t_old = {0}; \
	if(count%120 == 0) \
	{ \
		static struct timeval t_new; \
		gettimeofday(&t_new, 0); \
		printf("=====>%s:%d\n",name,120*1000000/((t_new.tv_sec-t_old.tv_sec)*1000000 + (t_new.tv_usec - t_old.tv_usec)));\
		t_old = t_new; \
	} \
	count++; \
} while(0)



#endif//_MYUTILS_H_
