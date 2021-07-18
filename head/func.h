#ifndef __FUNC_H__
#define __FUNC_H__

#include "head.h"
#include "define.h"
#include "struct.h"
#include "database.h"

int TaskQueueInit(pTask_queue_t pQueue);
int InsertTaskQueue(pTask_queue_t pQueue, pTask_node_t Node);
int GetTaskNode(pTask_queue_t pQueue, pTask_node_t *pGetNode);

int ThreadPoolInit(pThread_pool_t pPool, int thread_num);
int ThreadPoolCreate(pThread_pool_t pPool);

socket_fd TcpInit();
int EpollAddFd(int epoll_fd, int fd);

int Login(pAccount_t acc, MYSQL *db_connect);
int SignIn(pAccount_t acc, MYSQL *db_connect);
int UserFunc(pTask_node_t pTask);
void WriteLog(log_fd fd, const char *buf);
int CmdAnalyse();

#endif


