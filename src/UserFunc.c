#include "../head/head.h"

int UserFunc(pTask_node_t pTask){

    int ret = 0;

    //创建数据库连接
    MYSQL *conn = database_connect();
    if((MYSQL *)-1 == conn){
        return 0;
    }

    //用栈空间创建用户信息结构体
    account_t acc;

    while(1){

        memset(&acc, 0, sizeof(account_t));
        
        printf("[client:%s]waiting for choice.\n", pTask->ip);

        //接收用户发送过来的用户名，用户密码
        ret = recv(pTask->cfd, &acc, sizeof(account_t), MSG_WAITALL);
        
        if(0 == ret){
            printf("[client:%s]close the connection.\n", pTask->ip);
            close(pTask->cfd);
            return -1;
        }

        printf("[client:%s]name = %s, passwd = %s.\n", pTask->ip, acc.acc_name, acc.acc_passwd);

        //登录或者注册，将结果发送给客户端
        if(1 == acc.opt_flag){
            printf("client login\n");
            ret = Login(&acc, conn);
        }
        else if(0 == acc.opt_flag){
            printf("client signin\n");
            ret = SignIn(&acc, conn);
            //注册成功后再登录
            if(0 == ret){
                ret = Login(&acc, conn);
            }
        }
        send(pTask->cfd, &ret, 4, 0);

    }

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
    sprintf(query, "%s %s %s %s %s", "select id", "from user where username =", pAcc->acc_name, "and password =", pAcc->acc_passwd);
    int ret = database_operate(db_connect, query, &res);
    
    //查找失败，返回-1
    if(ret < 1){
        printf("[LOGIN]:FAILURE\n");
        return -1;
    }

    //查找成功返回用户id
    else{
        printf("[LOGIN]:SUCCESS\n");
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
 * 给客户端发送一个int型的数据，成功返回0，失败返回-1
 */
int SignIn(pAccount_t pAcc, MYSQL *db_connect){

    int ret = 0;

    //查询数据库中是否存在该用户名
    char query[200] = { 0 };

    //设置查询语句
    sprintf(query, "%s %s %s", "select id", "from user where username =", pAcc->acc_name);
    ret = database_operate(db_connect, query, NULL);
    
    //用户名存在，注册失败，返回-1
    if(ret > 0){
        printf("[SIGNIN]:FAILURE\n");
        return -1;
    }
    //用户名不存在，注册用户
    else{

        //设置插入语句
        memset(query, 0, sizeof(query));
        sprintf(query, "%s %s %s %s %s %s %s", "insert into user ( username , password , pwd ) values (", pAcc->acc_name, ",", pAcc->acc_passwd, ",", pAcc->acc_name, ")");
        database_operate(db_connect, query, NULL);
        printf("[SIGNIN]:SUCCESS\n");
        return 0;
    }
}
