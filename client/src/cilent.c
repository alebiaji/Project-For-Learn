#define _GNU_SOURCE
#include "../head/func.h"
#include "../head/cilent.h"
#include "../head/md5.h"

/* int exitpipe[2] = {0};  //定义一个整型数组，用来存储匿名管道的读端和写端
void sigfunc(int signum)        
{   //信号处理函数
    printf("sig is coming\n");

    write(exitpipe[1], &signum, 4);
    //将捕捉到的信号，写入管道，并在进程中监听管道的读端
} */

int main(int argc, char *argv[])
{
    /* pipe(exitpipe);     //创建管道，用来接收信号，实现文件传输的暂停
    signal(2, sigfunc);   //捕捉信号，并执行信号处理函数
 */
    ARGS_CHECK(argc, 3);

    //与服务器进行连接
    int sfd = socket(AF_INET, SOCK_STREAM, 0); //创建一个socket标识
    ERROR_CHECK(sfd, -1, "socket");
    struct sockaddr_in sockaddr; //创建一个结构体存储服务器的IP和端口号
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(argv[1]);
    sockaddr.sin_port = htons(atoi(argv[2]));
    int ret = connect(sfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    ERROR_CHECK(ret, -1, "connect"); //向服务端发起连接

    //用户登录或注册，进入文件传输系统
    UserLogin(sfd);


   /*  int epfd = epoll_create(1); //创建epoll文件对象
    epolladd(epfd, exitpipe[0]);    //将 传输信号的管道的读端，加入监听 */

    //创建一些必须的变量
    /* struct epoll_event eves[1];
    memset(eves, 0, sizeof(eves[0]));
    int readynum = 0; */
    char buf[4096] = {0};


    //循环接收 用户 输入的命令
    while (1)
    {
        /* readynum = epoll_wait(epfd, eves, 1, -1);
        if(eves[i].data.fd == exitpipe[0])
        {
            
        } */


        //读取用户输入的命令
        ret = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        if (ret == 0)
        {
            printf("connection break\n");
            break;
        }
        ERROR_CHECK(ret, -1, "read");

        //获得用户输入的命令和参数
        command_t command;
        memset(&command, 0, sizeof(command));
        if (GetCommand(&command, buf) == -1)
        {
            // 如果用户输入的是空格，那么就什么也不做，直接开始下一次的命令输入
            continue;
        }

        //分析用户输入的命令的类型，并分情况处理
        if (strcmp(command.c_content, "download") == 0)
        {
            //如果用户输入的是 下载命令
            ret = DownloadCommand(&command, sfd);
        }
        else if (strcmp(command.c_content, "upload") == 0)
        {
            //如果用户输入的是 上传命令
            ret = UploadCommand(&command, sfd);
        }
        else
        {
            //其他类型的命令
            ret = OtherCommand(&command, sfd);
        }
    }

    return 0;
}
