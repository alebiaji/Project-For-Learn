#ifndef __DEFINE_H__
#define __DEFINE_H__

//检查运行参数数量
#define ARGS_CHECK(argc, num) {\
    if(argc != num){\
        fprintf(stderr, "ARGS ERROR!\n");\
        return -1;\
    }\
}

//检查错误信息
#define ERROR_CHECK(ret, num, err_msg) {\
    if(ret == num){\
        perror(err_msg);\
        return -1;\
    }\
}

//检查线程错误信息
#define THREAD_ERROR_CHECK(ret, name)\
    do {\
        if(ret != 0){\
            printf("%s : %s\n", name, strerror(ret));\
        }\
    }while(0)

//线程池的数量
#define PTHREAD_NUM 20

//salt值字符串长度
#define STR_LEN 10

//登录操作
#define LOGIN 1

//注册操作
#define SIGNIN 0

#endif