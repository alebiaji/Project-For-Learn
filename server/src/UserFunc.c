#include "../head/func.h"

/**
 * 用户功能
*/
int UserFunc(pTask_node_t pTask){

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
    log_fd lfd= open(log_name, O_WRONLY | O_CREAT | O_APPEND, 0666);

    GetTimeStamp(timestamp, 1);
    sprintf(log_content, "[%s %s]get task, begin the connection.\n", timestamp, pTask->user_ip);
    ret = WriteLog(lfd, log_content);
    ERROR_CHECK(ret, -1, "WriteLog");

    //用栈空间创建用户信息结构体
    account_t acc;

    while(1){

        memset(&acc, 0, sizeof(account_t));

        GetTimeStamp(timestamp, 1);
        sprintf(log_content, "[%s %s]waiting for choice.\n", timestamp, pTask->user_ip);
        ret = WriteLog(lfd, log_content);
        ERROR_CHECK(ret, -1, "WriteLog");

        //1.接收用户发送过来的用户名，用户密码，功能选择
        ret = recv(pTask->user_cfd, &acc, sizeof(account_t), MSG_WAITALL);

        //客户端关闭连接
        if(0 == ret){
            GetTimeStamp(timestamp, 1);
            sprintf(log_content, "[%s %s]close the connection, task over.\n", timestamp, pTask->user_ip);
            ret = WriteLog(lfd, log_content);
            ERROR_CHECK(ret, -1, "WriteLog");
            break;
        }

        printf("[client:%s]name = %s, passwd = %s.\n", \
            pTask->user_ip, acc.acc_name, acc.acc_passwd);

        //2.登录或者注册
        if(LOGIN == acc.opt_flag){
            GetTimeStamp(timestamp, 1);
            sprintf(log_content, "[%s %s]login.\n", timestamp, pTask->user_ip);
            ret = WriteLog(lfd, log_content);
            ERROR_CHECK(ret, -1, "WriteLog");
            ret = Login(&acc, conn);
        }
        else if(SIGNIN == acc.opt_flag){
            GetTimeStamp(timestamp, 1);
            sprintf(log_content, "[%s %s]signin.\n", timestamp, pTask->user_ip);
            ret = WriteLog(lfd, log_content);
            ERROR_CHECK(ret, -1, "WriteLog");
            ret = SignIn(&acc, conn, pTask->user_cfd);
        }
        printf("ret = %d\n", ret);
        send(pTask->user_cfd, &ret, 4, 0);

        //3.登录或者注册失败，继续等待接收客户端请求
        if(-1 == ret){
            GetTimeStamp(timestamp, 1);
            sprintf(log_content, "[%s %s]failure.\n", timestamp, pTask->user_ip);
            ret = WriteLog(lfd, log_content);
            ERROR_CHECK(ret, -1, "WriteLog");
            continue;
        }
        //4.登录成功
        else{

            //保存用户信息
            pTask->user_lfd = lfd;
            pTask->user_id = ret;
            pTask->user_conn = conn;
            GetTimeStamp(timestamp, 1);
            sprintf(log_content, "[%s %s]user_id = %d login success.\n", timestamp, pTask->user_ip, pTask->user_id);
            ret = WriteLog(lfd, log_content);
            ERROR_CHECK(ret, -1, "WriteLog");

            //进入命令交互界面
            CmdAnalyse(pTask, timestamp);
            char query[100] = { 0 };
            sprintf(query, "update user set pwd = '%s' where id = %d", acc.acc_name, pTask->user_id);
            database_operate(pTask->user_conn, query, NULL);
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
 * 用户登录函数
 * 返回值：成功返回0，失败返回-1
 * 参数1：从客户端接收到的登录信息
 * 参数2：数据库连接
 * 在数据库中搜索是否存在用户名
 * 再数据库中查询该用户密码是否匹配
 * 用户名或者密码任意一个匹配失败都令acc->flag = -1
 * 给客户端发送一个int型的数据，成功返回用户id，失败返回-1
 */
int Login(pAccount_t pAcc, MYSQL *db_connect){

    //查询数据库中是否存在该用户名
    char query[100] = { 0 };
    char **res = NULL;

    //设置查询语句
    sprintf(query, "select id from user where username = '%s' and password = '%s'", pAcc->acc_name, pAcc->acc_passwd);
    int ret = database_operate(db_connect, query, &res);
    
    //查找失败，返回-1
    if(0 == ret){
        return -1;
    }

    //查找成功返回用户id
    else{
        return atoi(res[0]);
    }
}

/*
 * 用户注册函数
 * 返回值：成功返回0，失败返回-1
 * 参数1：从客户端接收到的注册信息
 * 参数2：数据库连接
 * 在数据库中搜索是否存在用户名
 * 如果存在acc->flag = -1表示注册失败
 * 如果不存在在数据库中新增用户数据
 * 给客户端发送一个int型的数据，成功用户id，失败返回-1
 */
int SignIn(pAccount_t pAcc, MYSQL *db_connect, client_fd fd){

    int ret = 0;

    //查询数据库中是否存在该用户名
    char query[200] = { 0 };

    //设置查询语句
    sprintf(query, "select id from user where username = '%s'", pAcc->acc_name);
    ret = database_operate(db_connect, query, NULL);
    
    //用户名存在，注册失败，返回-1
    if(ret > 0){
        return -1;
    }
    //用户名不存在，注册用户
    else{
        char salt[STR_LEN + 1] = { 0 };
        GetRandomStr(salt);
        //设置插入语句
        memset(query, 0, sizeof(query));
        sprintf(query, "insert into user ( username , salt , password, pwd ) values ( '%s' , '%s' , '%s' , '%s' )", pAcc->acc_name, salt, pAcc->acc_passwd, pAcc->acc_name);
        //printf("%s\n", query);
        ret = database_operate(db_connect, query, NULL);
        //strcpy(pAcc->acc_passwd, salt);

        //send(fd, )

        //用户注册成功，直接给他登录
        if(ret > 0){
            //登录系统并创建虚拟文件系统用户根目录
            int user_id = Login(pAcc, db_connect);
            memset(query, 0, sizeof(query));
            sprintf(query, "insert into file ( father_id, file_name , type , user_id , file_md5 , file_size ) values ( 0 , '%s' , 'd' , %d , 0 , 0)", pAcc->acc_name, user_id);
            //printf("%s\n", query);
            database_operate(db_connect, query, NULL);
            return user_id;
        }
        
        return ret;
    }
}

/**
 * 参数1：用于通信文件的描述符
 * 参数2：写入log文件的文件描述符
 * 参数3：用户id
 * 参数4：数据库的连接
*/
int CmdAnalyse(pTask_node_t pTask, char *timestamp){

    printf("CmdAnalyse\n");

    int ret;
    command_t cmd;
    char func_ret_buf[4096] = { 0 };
    char log_content[200] = { 0 };
    char query[100] = { 0 };
    int father_id = getpwd(pTask->user_conn, pTask->user_id, func_ret_buf);

    while(1){

        memset(&cmd, 0, sizeof(command_t));
        ret = recv(pTask->user_cfd, &cmd, sizeof(command_t), MSG_WAITALL);

        //客户端关闭连接
        if(0 == ret){
            GetTimeStamp(timestamp, 1);
            sprintf(log_content, "[%s %s]user_id = %d close the connection, task over.\n", timestamp, pTask->user_ip, pTask->user_id);
            ret = WriteLog(pTask->user_lfd, log_content);
            ERROR_CHECK(ret, -1, "WriteLog");
            return -1;
        }

        GetTimeStamp(timestamp, 1);
        sprintf(log_content, "[%s %s]user_id = %d send cmd, args = %d, cmd = %s %s %s %s\n",\
            timestamp, pTask->user_ip, pTask->user_id, cmd.cmd_args, cmd.cmd_content, cmd.cmd_arg1, cmd.cmd_arg2, cmd.cmd_arg3);
        ret = WriteLog(pTask->user_lfd, log_content);
        ERROR_CHECK(ret, -1, "WriteLog");

        //接收命令
        if(0 == strcmp(cmd.cmd_content, "ls")){
            ret = getls(pTask->user_conn, pTask->user_id, cmd.cmd_arg1, func_ret_buf);
            printf("%s\n", func_ret_buf);
        }
        else if(0 == strcmp(cmd.cmd_content, "cp")){
            ret = 1;
            printf("cp\n");
        }
        else if(0 == strcmp(cmd.cmd_content, "tree")){
            ret = tree(pTask->user_conn, pTask->user_id, cmd.cmd_arg1, func_ret_buf);
            printf("%s\n", func_ret_buf);
        }
        else if(0 == strcmp(cmd.cmd_content, "mkdir")){
            ret = 1;
            printf("mkdir\n");
        }
        else if(0 == strcmp(cmd.cmd_content, "rm")){
            ret = rm(pTask->user_conn, pTask->user_id, cmd.cmd_arg1);
            if(0 == ret){
                printf("rm success\n");
            }
        }
        else if(0 == strcmp(cmd.cmd_content, "mv")){
            ret = mv(pTask->user_conn, pTask->user_id, cmd.cmd_arg1, cmd.cmd_arg2);
            if(0 == ret){
                printf("rm success\n");
            }
        }
        else if(0 == strcmp(cmd.cmd_content, "pwd")){
            ret = getpwd(pTask->user_conn, pTask->user_id, func_ret_buf);
            if(-1 != ret){
                father_id = ret;
            }
            printf("%s\n", func_ret_buf);
        }
        else if(0 == strcmp(cmd.cmd_content, "cd")){
            ret = changeDir(pTask->user_conn, pTask->user_id, cmd.cmd_arg1);
            if(-1 != ret){
                father_id = ret;
            }
            printf("%s\n", func_ret_buf);
        }
        else if(0 == strcmp(cmd.cmd_content, "download")){
            ret = user_download(pTask->user_conn, pTask->user_cfd, father_id, cmd.cmd_arg1, atoi(cmd.cmd_arg2));
            if(0 == ret){
                printf("download success\n");
            }
        }
        else if(0 == strcmp(cmd.cmd_content, "upload")){
            ret = user_upload(pTask->user_conn, pTask->user_cfd, father_id, cmd.cmd_arg1, cmd.cmd_arg3, atoi(cmd.cmd_arg2), pTask->user_id);
            if(0 == ret){
                printf("upload success\n");
            }
            
        }
        else if(0 == strcmp(cmd.cmd_content, "exit")){
            ret = 1;
            send(pTask->user_cfd, &ret, 4, 0);
            return -1;
        }
        else{
            ret = -1;
            printf("error\n");
        }
        send(pTask->user_cfd, &ret, 4, 0);
    }

    return 0;
}
