#include "../head/head.h"

//线程池的数量
#define PTHREAD_NUM 20

int main()
{
    int ret = 0;

    //创建进程池结构体，不是指针！
    thread_pool_t pool;
    memset(&pool, 0, sizeof(thread_pool_t));

    //1.初始化线程池，创建pthread_num个线程空间，创建任务队列
    ret = ThreadPoolInit(&pool, PTHREAD_NUM);
    THREAD_ERROR_CHECK(ret, "ThreadPoolInit");

    //2.开启线程池，创建线程，接收任务
    ret = ThreadPoolCreate(&pool);
    THREAD_ERROR_CHECK(ret, "ThreadPoolCreate");

    //3.创建Tcp套接字监听连接
    socket_fd sfd = TcpInit();
    ERROR_CHECK(ret, -1, "TcpInit");

    //4.创建epoll，参数只要大于0就行，并将需要监听的文件描述符添加到epoll中
    int epoll_fd = epoll_create(1);
    ret = EpollAddFd(epoll_fd, sfd);
    ERROR_CHECK(ret, -1, "EpollAddFd");

    //epoll结构体，存放就绪文件描述符
    struct epoll_event evs[2];
    memset(evs, 0, sizeof(struct epoll_event));

    while(1){
        
        printf("wait client\n");

        //-1表示无条件等待
        int ready_num = epoll_wait(epoll_fd, evs, 2, -1);

        for(int i = 0; i < ready_num; ++i){

            if(evs[i].data.fd == sfd){

                //创建结构体存储对端地址和端口
                struct sockaddr_in addr_client;
                socklen_t len = sizeof(struct sockaddr_in);
                memset(&addr_client, 0, len);

                //接收新连接的文件描述符
                client_fd new_fd = accept(sfd, (struct sockaddr *)&addr_client, &len);
                ERROR_CHECK(new_fd, -1, "accept");

                pTask_node_t task = (pTask_node_t)calloc(1, sizeof(task_node_t));

                //添加任务节点
                task->cfd = new_fd;
                strcpy(task->ip, inet_ntoa(addr_client.sin_addr));

                //上锁互斥访问任务队列
                pthread_mutex_lock(&pool.task_queue.queue_mutex);

                //将与客户端通信的文件描述符加入任务队列
                ret = InsertTaskQueue(&pool.task_queue, task);
                ERROR_CHECK(ret, -1, "Insert task queue");

                //激发所有子线程
                ret = pthread_cond_broadcast(&pool.task_queue.queue_cond);
                THREAD_ERROR_CHECK(ret, "broadcast");

                //解锁
                pthread_mutex_unlock(&pool.task_queue.queue_mutex);
                
            }

        }
    }
}

