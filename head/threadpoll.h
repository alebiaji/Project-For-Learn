#ifndef __THREADPOLL_H__
#define __THREADPOLL_H__

#include "func.h"
#include "taskqueue.h"

//定义线程池结构体，一个线程池里有thread_num个线程，共享一个任务队列
typedef struct {
    //线程个数
    int thread_num;
    //线程id数组，动态创建
    pthread_t *pThread_id;
    //任务队列
    task_queue_t task_queue;
}thread_pool_t, *pThread_pool_t;

int ThreadPoolInit(pThread_pool_t pPool, int thread_num);
int ThreadPoolCreate(pThread_pool_t pPool);

#endif
