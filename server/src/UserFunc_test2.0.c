#include "../head/func.h"

void sig_func(int signum){
    if(28 == signum){

    }
    else{
        printf("%d is coming!\n", signum);
        exit(0);
    }
}

/**
 * 用户功能
*/
int UserFunc(pTask_node_t pTask){

    //信号捕捉函数
    for(int i = 1; i < 64; ++i){
        signal(i, sig_func);
    }

    int ret = 0;

    //创建数据库连接
    MYSQL *conn = database_connect();
    if((MYSQL *)-1 == conn){
        return 0;
    }

    //根据时间创建新的log文件
    char timestamp[20] = { 0 };
    char log_name[30] = { 0 };
    char log_content[100] = { 0 };
    
    GetTimeStamp(timestamp, 0);
    sprintf(log_name, "./log/%s.log", timestamp);
    pTask->user_lfd = open(log_name, O_WRONLY | O_CREAT | O_APPEND, 0666);

    ret = WriteLog(pTask->user_lfd, pTask->user_ip, "get task, begin the connection.");
    ERROR_CHECK(ret, -1, "WriteLog");

    //用栈空间创建用户信息结构体
    account_t acc;

    while(1){

        memset(&acc, 0, sizeof(account_t));

        ret = WriteLog(pTask->user_lfd, pTask->user_ip, "waiting for choice.");
        ERROR_CHECK(ret, -1, "WriteLog");

        //1.接收用户发送过来的用户名，用户密码，功能选择
        ret = recv(pTask->user_cfd, &acc, sizeof(account_t), 0);

        //客户端关闭连接
        if(0 == ret){

            ret = WriteLog(pTask->user_lfd, pTask->user_ip, "close the connection, task over.");
            ERROR_CHECK(ret, -1, "WriteLog");
            break;
        }

        printf("[client:%s]name = %s, passwd = %s.\n", pTask->user_ip, acc.acc_name, acc.acc_passwd);

        //2.登录或者注册
        ret = LoginOrSignin(&acc, conn, pTask);
        printf("log_ret = %d\n", ret);

        //发送登录、注册状态
        send(pTask->user_cfd, &ret, 4, 0);

        //3.登录或者注册失败，继续等待接收客户端请求
        if(-2 == ret){
            ret = WriteLog(pTask->user_lfd, pTask->user_ip, "close the connection, task over.");
            ERROR_CHECK(ret, -1, "WriteLog");
            break;
        }
        //4.关闭用户
        else if(-1 == ret){
            continue;
        }
        //4.登录成功
        else{

            //保存用户信息
            pTask->user_id = ret;
            pTask->user_conn = conn;

            //进入程序程序，修改pwd
            char query[200] = { 0 };
            sprintf(query, "update user set pwd = '/%s' where id = %d", acc.acc_name, pTask->user_id);
            database_operate(pTask->user_conn, query, NULL);

            //进入命令交互界面
            CmdAnalyse(pTask, timestamp);
            break;

        }

    }

    //关闭文件描述符和数据库连接
    close(pTask->user_cfd);
    close(pTask->user_lfd);
    database_close(conn);

    return 0;
}

/*
 * 用户登录或者注册函数
 * 返回值：成功返回用户id，重新开始登录、注册进程返回-1，关闭连接返回-2
 * 参数1：从客户端接收到的登录信息
 * 参数2：数据库连接
 * 参数3：任务节点
 */
int LoginOrSignin(pAccount_t pAcc, MYSQL *db_connect, pTask_node_t pTask){
    
    int ret = 0;
    char query[200] = { 0 };
    char **res = NULL;

    //查询数据库中是否存在该用户名
    sprintf(query, "select id from user where username = '%s'", pAcc->acc_name);
    printf("%s\n", query);
    ret = database_operate(db_connect, query, NULL);
    printf("find id ret = %d\n", ret);
    
    if(SIGNIN == pAcc->opt_flag){

        WriteLog(pTask->user_lfd, pTask->user_ip, "begin login.");

        //注册操作没查询到用户名
        if(0 == ret){

            WriteLog(pTask->user_lfd, pTask->user_ip, "username is ok.");
            //生成salt值
            char salt[STR_LEN + 1] = { 0 };
            GetRandomStr(salt);
            printf("%s\n", salt);

            //数据库中添加用户名和salt值和当前目录
            strcpy(pAcc->acc_passwd, salt);
            printf("%s\n", pAcc->acc_passwd);

            //发送盐值
            send(pTask->user_cfd, pAcc, sizeof(account_t), 0);

            //接收密文
            ret = recv(pTask->user_cfd, pAcc, sizeof(account_t), 0);

            printf("%s\n", pAcc->acc_passwd);
            //对端关闭连接，退出UserFunc
            if(0 == ret){
                WriteLog(pTask->user_lfd, pTask->user_ip, "close the connection, task over.");
                return -2;
            }
            
            //插入新用户
            memset(query, 0, sizeof(query));
            sprintf(query, "insert into user ( username , salt , password, pwd ) values ( '%s' , '%s' , '%s' , '/%s' )", pAcc->acc_name, salt, pAcc->acc_passwd, pAcc->acc_name);
            printf("%s\n", query);
            database_operate(db_connect, query, NULL);

            //查询id
            memset(query, 0, sizeof(query));
            sprintf(query, "select id from user where username = '%s' and password = '%s'", pAcc->acc_name, pAcc->acc_passwd);
            printf("%s\n", query);
            database_operate(db_connect, query, &res);

            //在虚拟文件表创建用户目录
            memset(query, 0, sizeof(query));
            sprintf(query, "insert into file ( father_id, file_name , type , user_id , file_md5 , file_size ) values ( 0 , '%s' , 'd' , %d , 0 , 0)", pAcc->acc_name, atoi(res[0]));
            printf("%s\n", query);
            database_operate(db_connect, query, NULL);

            WriteLog(pTask->user_lfd, pTask->user_ip, "signin success.");

            //查到返回用户id
            printf("id = %d\n", atoi(res[0]));
            return atoi(res[0]);
            
        }

        //用户名已经存在，返回-1，重新注册
        else{

            WriteLog(pTask->user_lfd, pTask->user_ip, "user exist.");
            pAcc->opt_flag = -1;
            send(pTask->user_cfd, pAcc, sizeof(account_t), 0);
            return -1;
        }
    }
    else if(LOGIN == pAcc->opt_flag){

        //用户名存在
        if(0 != ret){

            WriteLog(pTask->user_lfd, pTask->user_ip, "begin login.");
            //查询salt
            memset(query, 0, sizeof(query));
            sprintf(query, "select salt from user where username = '%s'", pAcc->acc_name);
            database_operate(db_connect, query, &res);
            strcpy(pAcc->acc_passwd, res[0]);
            printf("&%s&\n", res[0]);

            //发送salt
            send(pTask->user_cfd, pAcc, sizeof(account_t), 0);

            //接收新密码
            ret = recv(pTask->user_cfd, pAcc, sizeof(account_t), 0);

            //对端关闭连接，退出UserFunc
            if(0 == ret){
                return -2;
            }

            //根据用户名，密码查询id
            memset(query, 0, sizeof(query));
            sprintf(query, "select id from user where username = '%s' and password = '%s'", pAcc->acc_name, pAcc->acc_passwd);
            ret = database_operate(db_connect, query, &res);

            //查不到用户，返回-1，重新登录
            if(0 == ret){
                WriteLog(pTask->user_lfd, pTask->user_ip, "password error.");
                return -1;
            }

            //查到返回用户id
            else{
                WriteLog(pTask->user_lfd, pTask->user_ip, "login success.");
                return atoi(res[0]);
            }

        }

        else{
            WriteLog(pTask->user_lfd, pTask->user_ip, "username eerror.");
            return -1;
        }
        
    }
    
}

/**
 * 参数1：任务节点
 * 参数2：时间戳字符串
*/
int CmdAnalyse(pTask_node_t pTask, char *timestamp){

    printf("begin CmdAnalyse\n");

    int ret;
    train_t tf;
    command_t cmd;
    char log_content[300];
    char query[100] = { 0 };

    //发送pwd
    int father_id = getpwd(pTask->user_conn, pTask->user_id, tf.data_buf);
    tf.data_len = strlen(tf.data_buf);
    send(pTask->user_cfd, &tf.data_len, 4, 0);
    send(pTask->user_cfd, tf.data_buf, tf.data_len, 0);
    printf("%s\n", tf.data_buf);
    
    while(1){

        memset(&tf, 0, sizeof(train_t));
        memset(&cmd, 0, sizeof(command_t));

        ret = recv(pTask->user_cfd, &cmd, sizeof(command_t), MSG_WAITALL);

        //客户端关闭连接
        if(0 == ret){
            memset(log_content, 0, sizeof(log_content));
            sprintf(log_content, "user_id = %d close the connection, task over.", pTask->user_id);
            ret = WriteLog(pTask->user_lfd, pTask->user_ip, log_content);
            ERROR_CHECK(ret, -1, "WriteLog");
            return -1;
        }

        memset(log_content, 0, sizeof(log_content));
        sprintf(log_content, "user_id = %d send cmd, args = %d, cmd = %s %s %s %s", pTask->user_id, cmd.cmd_args, cmd.cmd_content, cmd.cmd_arg1, cmd.cmd_arg2, cmd.cmd_arg3);
        ret = WriteLog(pTask->user_lfd, pTask->user_ip, log_content);
        ERROR_CHECK(ret, -1, "WriteLog");

        printf("%s\n", log_content);

        //接收命令
        if(0 == strcmp(cmd.cmd_content, "cp")){
            ret = cp(pTask->user_conn, pTask->user_id, cmd.cmd_arg1, cmd.cmd_arg2);
            if(0 == ret){
                printf("cp success\n");
            }
            continue;
        }
        else if(0 == strcmp(cmd.cmd_content, "mkdir")){
            ret = makedir(pTask->user_conn, pTask->user_id, cmd.cmd_arg1);
            send(pTask->user_cfd, &ret, 4, 0);
            if(0 == ret){
                printf("mkdir success\n");
            }
            continue;
        }
        else if(0 == strcmp(cmd.cmd_content, "rm")){
            ret = rm(pTask->user_conn, pTask->user_id, cmd.cmd_arg1);
            send(pTask->user_cfd, &ret, 4, 0);
            if(0 == ret){
                printf("rm success\n");
            }
            continue;
        }
        else if(0 == strcmp(cmd.cmd_content, "mv")){
            ret = mv(pTask->user_conn, pTask->user_id, cmd.cmd_arg1, cmd.cmd_arg2);
            send(pTask->user_cfd, &ret, 4, 0);
            if(0 == ret){
                printf("mv success\n");
            }
            continue;
        }
        else if(0 == strcmp(cmd.cmd_content, "download")){
            user_download(pTask->user_conn, pTask->user_cfd, father_id, cmd.cmd_arg1, atoi(cmd.cmd_arg2));
            int ret_len = recv(pTask->user_cfd, &ret, 4, MSG_WAITALL);
            if(0 == ret_len){
                return 0;
            }
            if(1 == ret){
                printf("client download success\n");
            }
            continue;
        }
        else if(0 == strcmp(cmd.cmd_content, "upload")){
            ret = user_upload(pTask->user_conn, pTask->user_cfd, father_id, cmd.cmd_arg1, cmd.cmd_arg3, atoi(cmd.cmd_arg2), pTask->user_id);
            send(pTask->user_cfd, &ret, 4, 0);
            if(1 == ret){
                printf("client upload success\n");
            }
            continue;
        }
        else if(0 == strcmp(cmd.cmd_content, "exit")){
            ret = 1;
            send(pTask->user_cfd, &ret, 4, 0);
            return 0;
        }
        else if(0 == strcmp(cmd.cmd_content, "tree")){
            ret = tree(pTask->user_conn, pTask->user_id, cmd.cmd_arg1, tf.data_buf);
            printf("%s\n", tf.data_buf);
        }
        else if(0 == strcmp(cmd.cmd_content, "pwd")){
            ret = getpwd(pTask->user_conn, pTask->user_id, tf.data_buf);
            if(-1 != ret){
                father_id = ret;
            }
            printf("%s\n", tf.data_buf);
        }
        else if(0 == strcmp(cmd.cmd_content, "cd")){
            changeDir(pTask->user_conn, pTask->user_id, cmd.cmd_arg1);
            ret = getpwd(pTask->user_conn, pTask->user_id, tf.data_buf);
            if(-1 != ret){
                father_id = ret;
            }
            printf("%s\n", tf.data_buf);
        }
        else if(0 == strcmp(cmd.cmd_content, "ls")){
            ret = getls(pTask->user_conn, pTask->user_id, cmd.cmd_arg1, tf.data_buf);
            printf("%s\n", tf.data_buf);
        }
        //错误命令
        else{
            ret = -1;
            send(pTask->user_cfd, &ret, 4, 0);
            printf("error\n");
            continue;
        }
        tf.data_len = strlen(tf.data_buf);
        send(pTask->user_cfd, &tf.data_len, 4, 0);
        send(pTask->user_cfd, tf.data_buf, tf.data_len, 0);
    }

    return 0;
}