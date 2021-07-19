#include "../head/func.h"
#include "../head/cilent.h"

//用户登录或注册
int UserLogin(int sfd)
{
    while (1)
    {
        int flag = 0, ret = 0;      //用于判断是登陆还是注册
        account_t account; //保存用户名
        memset(&account, 0, sizeof(account_t));

        //显示界面，选择登录 还是  注册
        ShowManu();

        //用户选择登录还是注册
        UseChoose(&account);

        //将账号密码发送给服务器
        ret = send(sfd, &account, sizeof(account), 0);
        ERROR_CHECK(ret, -1, "send_account");
        ret = recv(sfd, &flag, sizeof(int), 0);
        ERROR_CHECK(ret, -1, "recv");
        //匹配，即登陆成功，返回一个结果，不匹配，登录或注册失败返回-1
        if (flag != -1)
        {
            printf("successful\n");
            break;
        }
        else
        {
            if (account.flag == 1) //如果是 登陆失败
            {
                printf("error password\n");
            }
            else //如果是注册失败
            {
                printf("user name exist\n");
            }
            printf("enter any key to continue\n");
            getchar();
            system("clear");
        }
    }
}


//显示菜单
void ShowManu()
{
    printf("*********************************************\n");
    printf("************ FILE SERVE SYSTEM **************\n");
    printf("*********************************************\n");
    printf("\n\n");
    printf("1.User Login\n");
    printf("2.User Register\n");
}


//用户选择功能，并输入用户名密码
int UseChoose(paccount_t paccount)
{
    int flag = 0;
    scanf("%d", &flag);

    if (flag == 1)
    {
        paccount->flag = 1; //表示登录
        GetAccountPasswd(paccount);
        //获得用户输入的用户名，密码
        system("clear");
    }
    else if (flag == 2)
    {
        while (1)
        {
            paccount->flag = 0; //表示 注册
            GetAccountPasswd(paccount);
            //获得用户输入的用户名，密码
            printf("configue user passwd:\n");
            //再次输入密码
            char buf[20] = {0};
            HidePasswd(buf);
            char buf1[20] = {0};
            sprintf(buf1, "%s%s%s", "\'", buf, "\'");
            // 将密码加上 单引号
            if (strcmp(buf1, paccount->a_passwd) == 0)
            {
                // 如果两次输入的密码相同，则可以发送
                break;
            }
            else
            {
                // 如果两次输入的密码不同，则需要重新输入
                printf("password wrong, please reset passwd\n");
                printf("enter any key to continue\n");
                getchar();

                system("clear");
                //清理屏幕，开始下一次的输入密码

                memset(buf, 0, 20);
                memset(buf1, 0, 20);
                memset(paccount->a_name, 0, sizeof(paccount->a_name));
                memset(paccount->a_passwd, 0, sizeof(paccount->a_passwd));
            }
        }
    }
}


//用户登录函数，保存用户输入的用户名和密码
void GetAccountPasswd(paccount_t paccount)
{
    char buf[20] = {0};
    printf("enter user name:\n");
    scanf("%s", buf);
    sprintf(paccount->a_name, "%s%s%s", "\'", buf, "\'"); //给用户名两端加上 “ ' ”，方便服务器操作
    //输入用户名，并保存至 account 结构体当中

    memset(buf, 0, sizeof(buf));
    printf("enter user password:\n");
    getchar();
    HidePasswd(buf);
    printf("buf = %s\n", buf);
    sprintf(paccount->a_passwd, "%s%s%s", "\'", buf, "\'");
    //将用户输入的密码，加上两个单引号，保存到 account 结构体中
}


//自定义的getch()函数，使在键盘上输入的字符不显示
int getch()
{
    int c = 0;
    struct termios org_opts, new_opts;
    int res = 0;
    //-----  store old settings -----------
    res = tcgetattr(STDIN_FILENO, &org_opts);
    assert(res == 0);
    //---- set new terminal parms --------
    memcpy(&new_opts, &org_opts, sizeof(new_opts));
    new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
    c = getchar();
    //------  restore old settings ---------
    res = tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
    assert(res == 0);
    return c;
}


//实现隐藏密码的功能，将输入的密码显示为 *
void HidePasswd(char pb[])
{
    
    int i;
    for (i = 0;; i++)
    {
        pb[i] = getch();
        if (pb[i] == '\n')
        {
            pb[i] = '\0';
            break;
        }
        if (pb[i] == '\b') //删除 输入的 就是 '\b'
        {
            printf("\b \b");
            i = i - 2;
        }
        else
        {
            printf("*");
        }
        if (i < 0)
        {
            pb[0] = '\0';
        }
    }

    // printf("\ncode:%s\n", pb);
}
