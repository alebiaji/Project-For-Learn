#ifndef __FUNC_H__
#define __FUNC_H__

#include "head.h"
#include "define.h"
#include "struct.h"
#include "database.h"
#include "command.h"
#include "fileTrans.h"


int TaskQueueInit(pTask_queue_t pQueue);
int InsertTaskQueue(pTask_queue_t pQueue, pTask_node_t Node);
int GetTaskNode(pTask_queue_t pQueue, pTask_node_t *pGetNode);

int ThreadPoolInit(pThread_pool_t pPool, int thread_num);
int ThreadPoolCreate(pThread_pool_t pPool);

socket_fd TcpInit();
int EpollAddFd(int epoll_fd, int fd);

int LoginOrSignin(pAccount_t pAcc, MYSQL *db_connect, pTask_node_t pTask);
int UserFunc(pTask_node_t pTask);
int WriteLog(log_fd fd, char *ip, char *buf);
int CmdAnalyse(pTask_node_t pTask, char *timestamp);
void GetRandomStr(char *str);
void GetTimeStamp(char *str, int flag);


int SignIn(pAccount_t pAcc, MYSQL *db_connect);
int Login(pAccount_t pAcc, MYSQL *db_connect);


int user_download(MYSQL *db_connect, int clientfd, int father_id, const char *filename, off_t file_offset);
int user_upload(MYSQL *db_connect, int clientfd, int father_id, const char *filename, const char *md5, off_t filesize, int user_id);


#endif


