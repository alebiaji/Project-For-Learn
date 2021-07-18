#include "../head/head.h"

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
