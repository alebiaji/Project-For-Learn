#include "../head/func.h"
#include "../head/cilent.h"
#include "../head/md5.h"

int main(int argc, char *argv[])
{

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

    char buf[4096] = {0};
    char username[4096] = {0};
    char userdir[8192] = {0};
    result_t result;
    memset(&result, 0, sizeof(result));
    int recvflag = 0;

    memset(&result, 0, sizeof(result));

    // printf("hello\n");

    //用result结构体接收，先接受4个字节的长度，在接受内容
    ret = recv(sfd, &result.len, sizeof(result.len), MSG_WAITALL);
    ERROR_CHECK(ret, -1, "recv_len");
    recv(sfd, result.buf, result.len, MSG_WAITALL);
    ERROR_CHECK(ret, -1, "recv_buf");
    strcpy(username, result.buf);

    sprintf(userdir, "%s%s%s", username, ":", "$ ");
    memset(username, 0, sizeof(username));

    //循环接收 用户 输入的命令
    while (1)
    {
        puts("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        printf("%s", userdir);
        fflush(stdout);

        //读取用户输入的命令
        memset(buf, 0, sizeof(buf));
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
            //下载完成，给对方发送一个整数
            send(sfd, &ret, 4, 0);
        }
        else if (strcmp(command.c_content, "upload") == 0)
        {
            //如果用户输入的是 上传命令
            ret = UploadCommand(&command, sfd);
            //上传完成，接收对方发给我的一个整数
            recv(sfd, &recvflag, 4, MSG_WAITALL);
        }
        else if ((strcmp(command.c_content, "ls") == 0) || (strcmp(command.c_content, "tree") == 0) || (strcmp(command.c_content, "pwd") == 0))
        {
            //其他类型的命令
            OtherCommand(&command, sfd);

            memset(&result, 0, sizeof(result));

            //用result结构体接收，先接受4个字节的长度，在接受内容
            ret = recv(sfd, &result.len, sizeof(result.len), MSG_WAITALL);
            ERROR_CHECK(ret, -1, "recv_len");
            if(result.len == 0)
            {
                printf("%s", userdir);
                printf("\b \b");
                printf("\b \b\n");
                continue;
            }
            recv(sfd, result.buf, result.len, MSG_WAITALL);
            ERROR_CHECK(ret, -1, "recv_buf");
            puts(result.buf);
            // fflush(stdout);
            // printf("结果以输出\n");
        }
        else if (strcmp(command.c_content, "cd") == 0)
        {
            //其他类型的命令
            OtherCommand(&command, sfd);

            memset(&result, 0, sizeof(result));

            //用result结构体接收，先接受4个字节的长度，在接受内容
            ret = recv(sfd, &result.len, sizeof(result.len), MSG_WAITALL);
            ERROR_CHECK(ret, -1, "recv_len");

            recv(sfd, result.buf, result.len, MSG_WAITALL);
            ERROR_CHECK(ret, -1, "recv_buf");

            strcpy(username, result.buf);
            memset(userdir, 0, sizeof(userdir));
            sprintf(userdir, "%s%s%s", username, ":", "$ ");
            memset(username, 0, sizeof(username));
        }
        else if (strcmp(command.c_content, "clear") == 0)
        {
            system("clear");
        }
        else if (strcmp(command.c_content, "exit") == 0)
        {
            OtherCommand(&command, sfd);
            recv(sfd, &recvflag, 4, 0);
            printf("bye bye\n");
            break;
        }
        else
        {
            OtherCommand(&command, sfd);
            recv(sfd, &recvflag, 4, MSG_WAITALL);
            if (recvflag != -1)
            {
                printf("%s success\n", command.c_content);
            }
            else
            {
                printf("%s failed\n", command.c_content);
            }
        }
    }

    return 0;
}
