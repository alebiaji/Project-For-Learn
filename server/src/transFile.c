#include "../head/func.h"

#define BIG_SIZE 104857600 //100M，大文件阈
#define QUERY_LEN 256      //SQL语句字符串长度
#define REAL_NAME_LEN 64   //真实文件名字符串长度
#define BUF_LEN 4096      //传输buf大小
#define NET_DISK "./file/"    //真实文件存储位置

//用户下载
int user_download(MYSQL *db_connect, int clientfd, int father_id, const char *filename, off_t file_offset)
{
    //SQL查询语句
    char query[QUERY_LEN] = {0};
    sprintf(query, "select file_md5 from file where father_id = %d and file_name = '%s'",
            father_id, filename);
    printf("%s\n", query);
    //查file表获取真实文件名(即md5)
    char **file_md5 = NULL;
    int ret = database_operate(db_connect, query, &file_md5);
    // printf("ret = %d  filename = %s\n",ret, file_md5[0]);
    if (ret == 0) //虚拟文件表中不存在该文件
    {
        printf("file is not exist\n");
        return SQL_NOT_HAVE_FILE;
    }

    //组合真实文件路径
    char real_filename[REAL_NAME_LEN] = NET_DISK;
    sprintf(real_filename, "%s%s", real_filename, file_md5[0]);
    printf("filename=%s\n", real_filename);

    //打开真实文件
    int filefd = open(real_filename, O_RDONLY);
    ERROR_CHECK(filefd, -1, "open");

    //获取真实文件大小
    struct stat fileStat;
    memset(&fileStat, 0, sizeof(struct stat));
    fstat(filefd, &fileStat);

    fileStat.st_size -= file_offset; //文件大小减去偏移量(即客户端已下载的文件大小)

    //向客户端发送待传输的文件大小
    ret = send(clientfd, &fileStat.st_size, sizeof(off_t), 0);
    ERROR_CHECK(ret, -1, "recv file size");
    printf("file size = %ld\n", fileStat.st_size);

    //向客户端传输文件内容
    char trans_buf[BUF_LEN] = {0};

    off_t sendLength = 0; //记录已传送的数据量

    if (fileStat.st_size < BIG_SIZE) //小文件
    {
        lseek(filefd, file_offset, SEEK_SET); //偏移文件读写指针：断点续传

        while (sendLength < fileStat.st_size)
        {
            memset(trans_buf, 0, BUF_LEN);

            ret = read(filefd, trans_buf, BUF_LEN); //先读文件内容到trans_buf
            ERROR_CHECK(ret, -1, "read real file");

            ret = send(clientfd, trans_buf, ret, 0); //再发送到客户端
            ERROR_CHECK(ret, -1, "send file to client");
            if (ret == -1)
            {
                return CLIENT_DISCONNECTION;
            }

            sendLength += ret;
        }
    }
    else //大文件，零拷贝技术传输。自带偏移量：可实现断点续传
    {
        sendfile(clientfd, filefd, &file_offset, fileStat.st_size);
    }

    return 0;
}

//用户上传
int user_upload(MYSQL *db_connect, int clientfd, int father_id, const char *filename, const char *md5, off_t filesize, int user_id)
{
    //组合真实文件路径
    char real_filename[REAL_NAME_LEN] = NET_DISK;
    sprintf(real_filename, "%s%s", real_filename, md5);

    //打开\创建真实文件
    int filefd = open(real_filename, O_RDWR | O_CREAT, 0666);
    ERROR_CHECK(filefd, -1, "open real");

    //获取真实文件大小(可能是0，即为新文件)
    struct stat *fileStat = (struct stat *)calloc(1, sizeof(struct stat));
    fstat(filefd, fileStat);

    //向客户端发送现有文件大小
    int ret = send(clientfd, &fileStat->st_size, sizeof(off_t), 0);
    ERROR_CHECK(ret,-1,"send file size");
    printf("trans size = %ld\n",fileStat->st_size);

    //SQL查询语句
    char query[QUERY_LEN] = {0};
    sprintf(query, "select * from file_real where file_md5 = '%s'", md5);

    //查询file_real中是否存在此次上传的MD5码
    ret = database_operate(db_connect, query, NULL);
    printf("upload ret = %d\n", ret);

    if (ret != 0 && fileStat->st_size == filesize) //MySQL中存在(即上传旧文件)，且大小相等：文件秒传
    {
        memset(query, 0, QUERY_LEN);

        //更新file_real表中对应记录的file_count值
        sprintf(query, "update file_real set file_count = file_count + 1 where md5 ='%s'", md5);
        // printf("%s\n",query);

        database_operate(db_connect, query, NULL);
    }
    else //1、服务器中没有，就接收上传新文件。2、服务器中有但文件大小不等，就接收断点续传
    {
        if (ret == 0) //服务器中没有,MySQL中插入
        {
            memset(query, 0, QUERY_LEN);

            //插入新纪录到file表
            sprintf(query, "insert into file (father_id,file_name,file_md5,file_size,type,user_id) values(%d, '%s', '%s', %ld, 'f',%d) ", father_id, filename, md5, filesize, user_id);
            printf("%s\n",query);

            database_operate(db_connect, query, NULL);

            memset(query, 0, QUERY_LEN);

            //插入新记录到file_real表
            sprintf(query, "insert into file_real (md5,file_count) values ('%s',1)", md5);
            printf("%s\n",query);

            database_operate(db_connect, query, NULL);
        }

        //接收客户端上传
        off_t recvLen = 0;
        off_t needLen = filesize - fileStat->st_size;

        lseek(filefd, 0, SEEK_END); //读写指针偏移到文件尾：断点续传

        char gets_buf[BUF_LEN] = {0};
        while (recvLen < needLen)
        {
            memset(gets_buf, 0, BUF_LEN);

            ret = recv(clientfd, gets_buf, BUF_LEN, 0);
            ERROR_CHECK(ret, -1, "server recv");
            if (ret == 0)
            {
                return CLIENT_DISCONNECTION;
            }
            ret = write(filefd, gets_buf, ret);
            ERROR_CHECK(ret, -1, "server write");

            recvLen += ret;
        }
    }
    return 0;
}