/******************************************************************************
 *
 *       Filename:  ringqueue.h
 *
 *    Description:  ring queue
 *
 *        Version:  1.0
 *        Created:  2021年04月11日 10时57分30秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef _RING_QUEUE_H_
#define _RING_QUEUE_H_
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct s_rq
{
	int head;
	int tail;
	int size;
	void **buf;
	long long elem;
}t_rq;

typedef enum rq_type
{
	RQ_SUCCESS = 0,
	RQ_FULL = 1,
	RQ_EMPTY = 2,

}e_rq_type;

int rqing_init(t_rq **q, int size);
int rqing_deinit(t_rq **q);
int rqing_isfull(t_rq *q);
int rqing_isempty(t_rq *q);

int rqing_push(t_rq *q, void* emem);
int rqing_pop(t_rq *q, void** emem);

#ifdef __cplusplus
}
#endif
#endif//_RING_QUEUE_H_
