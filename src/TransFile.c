#define _GNU_SOURCE
#include "../head/head.h"

void sigfunc(int signum){
    printf("SIGPIPE is coming");
}

int TransFile(client_fd fd){

    signal(SIGPIPE, sigfunc);
    
    int ret = 0;

    //打开文件描述符
    int fd = open("1.pdf", O_RDWR);
    ERROR_CHECK(ret, -1, "open");

    //获取当前文件的状态
    struct stat fileInfo;
    memset(&fileInfo, 0, sizeof(fileInfo));
    ret = fstat(fd, &fileInfo);
    ERROR_CHECK(ret, -1, "fstat");

    //创建传输数据结构体
    train_t train;
    memset(&train, 0, sizeof(train));
    
    //发送文件名
    train.data_len = strlen("1.pdf");
    strcpy(train.data_buf, "1.pdf");
    ret = send(fd, &train, 4 + train.data_len, 0);
    ERROR_CHECK(ret, -1, "send");
    memset(&train, 0, sizeof(train));

    //发送文件大小
    train.data_len = sizeof(fileInfo.st_size);
    memcpy(train.data_buf, &fileInfo.st_size, sizeof(off_t));
    send(fd, &train, 4 + train.data_len, 0);
    printf("fileSize = %ld\n", fileInfo.st_size);

    long send_size = 0;
    int fds[2] = { 0 };
    pipe(fds);
    
    while(send_size < fileInfo.st_size){
        ret = splice(fd, 0, fds[1], 0, 64, 0);
        ret = splice(fds[0], 0, fd, 0, ret, 0);
        ERROR_CHECK(ret, -1, "splice");
        send_size += ret;
    }

    close(fds[0]);
    close(fds[1]);

    return 0;
}

