#ifndef __FUNC_H__
#define __FUNC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <mysql/mysql.h>

#define ARGS_CHECK(argc, num) {\
    if(argc != num){\
        fprintf(stderr, "ARGS ERROR!\n");\
        return -1;\
    }\
}
#define ERROR_CHECK(ret, num, err_msg) {\
    if(ret == num){\
        perror(err_msg);\
        return -1;\
    }\
}
#define THREAD_ERROR_CHECK(ret, name)\
    do {\
        if(ret != 0){\
            printf("%s : %s\n", name, strerror(ret));\
        }\
    }while(0)

//服务器用于接收tcp连接的文件描述符
typedef int socket_fd;
//服务器用于和客户端通信的文件描述符
typedef int client_fd;

//用于传输数据
typedef struct{
    int data_len;
    char data_buf[1000];
}train_t;

//用户登录、注册结构体
typedef struct account_s{
    //1是登录，0是注册
    int opt_flag;
    char acc_name[20];
    char acc_passwd[20];
}account_t, *pAccount_t;

//命令结构体
typedef struct command_s{
    //命令参数数量
    int cmd_args;
    //命令
    char cmd_content[20];
    //参数1
    char cmd_arg1[20];
    //参数2
    char cmd_arg2[20];
}command_t, *pCommand_t;

socket_fd TcpInit();
int EpollAddFd(int epoll_fd, int fd);
int Login(pAccount_t acc, MYSQL *db_connect);
int SignIn(pAccount_t acc, MYSQL *db_connect);
int CmdAnalyse(client_fd fd);

int TransFile(client_fd fd);

#endif
