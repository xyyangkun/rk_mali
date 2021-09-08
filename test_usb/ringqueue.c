/******************************************************************************
 *
 *       Filename:  ringqueue.c
 *
 *    Description: ring queue
 *
 *        Version:  1.0
 *        Created:  2021年04月11日 11时05分02秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include "ringqueue.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

int rqing_init(t_rq **q, int size)
{
	assert(size >0);

	t_rq *r = (t_rq *)calloc(1, sizeof(t_rq));
	r->size = size;
	r->head = 0;
	r->tail = r->size;

	// 实际分配buf的内存为r->size + 1;
	// 会有一个元素位置无法使用
	r->buf = (void *)calloc(1, sizeof(void*) * (r->size + 1));

	*q = r;

	return 0;
}

int rqing_deinit(t_rq **q)
{
	t_rq *r = *q;
	free(r->buf);
	r->buf = NULL;

	free(r);
	r = NULL;
}

// full tail == (head + size + 1)%size
int rqing_isfull(t_rq *q)
{
	assert(q!=NULL);
	//return  q->tail == (q->head + 1) % (q->size + 1);
	return  q->tail == q->head ;
}

int rqing_isempty(t_rq *q)
{
	assert(q!=NULL);
	return  q->head == (q->tail + 1) % (q->size + 1);
}

int rqing_push(t_rq *q, void* elem)
{
	assert(q!=NULL);
	// 1. 先判断是否满了
	// if(q->tail == (q->head + 1) % (q->size + 1))
	if(q->tail == q->head)
	{
		printf("is full\n");
		return RQ_FULL;	
	}

	q->buf[q->head] = elem;
	q->head = (q->head + 1)%(q->size + 1);
	
	return 0;
}

int rqing_pop(t_rq *q, void** elem)
{
	// 先判断是空了
	if(q->head == (q->tail + 1) % (q->size + 1))
	{
		//printf("is empyt!\n");
		return RQ_EMPTY;
	}
	else
	{
		//printf("success pop\n");
	}
	
	q->tail = (q->tail + 1)%(q->size + 1);
	*elem = q->buf[q->tail];

	return 0;
}


