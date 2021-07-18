#include "../head/func.h"

//返回一个用于tcp通信的套接字
socket_fd TcpInit(){

    int ret = 0;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    /*分别设置协议、IP地址和端口号*/
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("192.168.5.87");
    addr.sin_port = htons(2000);

    socket_fd sfd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(sfd, -1, "socket");

    //设置地址可重用
    int reuse = 1;
    ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
    ERROR_CHECK(ret, -1, "setsockopt");
    
    //绑定ip地址和端口号
    ret = bind(sfd, (struct sockaddr*)&addr, sizeof(addr));
    ERROR_CHECK(ret, -1, "bind");

    //监听描述符，最大连接数设置为10
    ret = listen(sfd, 10);
    ERROR_CHECK(ret, -1, "ret");

    return sfd;
}

int EpollAddFd(int epoll_fd, int fd){

    int ret = 0;
    struct epoll_event events;
    memset(&events, 0, sizeof(events));

    events.events = EPOLLIN;
    events.data.fd = fd;

    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &events);
    ERROR_CHECK(ret, -1, "epoll_ctl");

    return 0;
}

/**
 * 参数1：用于写log文件的文件描述符
 * 参数2：写入的语句
*/
void WriteLog(log_fd fd, const char *buf){
    write(fd, buf, sizeof(buf));
}