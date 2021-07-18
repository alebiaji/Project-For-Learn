#ifndef __TASKQUEUE_H__
#define __TASKQUEUE_H__

#include "func.h"

//每个任务中存储与客户端相关信息
typedef struct task_node_s{
    //与客户端通信的文件描述符
    client_fd cfd;
    //客户端用户的id
    int user_id;
    //客户端的地址信息
    char ip[15];
    //下一个任务
    struct task_node_s *pNext;
}task_node_t, *pTask_node_t;

//任务队列
typedef struct task_queue_s{
    //退出信号
    short exit_flag;
    //任务队列长度
    int queue_len;
    //线程的条件变量
    pthread_cond_t queue_cond;
    //线程锁
    pthread_mutex_t queue_mutex;
    //任务队列的首尾指针
    pTask_node_t pHead, pTail;
}task_queue_t, *pTask_queue_t;

int TaskQueueInit(pTask_queue_t pQueue);
int InsertTaskQueue(pTask_queue_t pQueue, pTask_node_t Node);
int GetTaskNode(pTask_queue_t pQueue, pTask_node_t *pGetNode);

#endif
