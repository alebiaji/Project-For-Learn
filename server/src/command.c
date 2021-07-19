#include "func.h"
#include "database.h"
#include "command.h"

//获取path路径目录下的所有文件/文件夹的名称，以空格间隔存入buf里
int getls(MYSQL *conn, int user_id, char *path, char *buf)
{
    getPath(conn,user_id,path,buf);
    //printf("buf:%s\n",buf);

    int id = getFileId(conn, user_id, buf);

    char query[200] = "select file_name from file where father_id = ";
    sprintf(query, "%s%d and user_id = %d;", query, id, user_id);

    MYSQL_RES *res;
    MYSQL_ROW row;

    //printf("%s\n", query);

    //脏数据
    memset(buf, 0, 4096);
    int queryRet = mysql_query(conn, query);
    if (queryRet)
    {
        printf("Error query: %s\n", mysql_error(conn));
        return -1;
    }
    else
    {
        res = mysql_store_result(conn);
        /* printf("mysql_affacted_rows: %lu\n", (unsigned long)mysql_affected_rows(conn)); */
        /* printf("mysql_num_rows: %lu\n", (unsigned long)mysql_num_rows(res)); */

        row = mysql_fetch_row(res);
        if (NULL == row)
        {
            printf("Don't query any data\n");
        }
        else
        {
            int i = 0;
            do
            {
                /* printf("num row: %lu\n", (unsigned long)mysql_num_rows(res)); */
                /* printf("num fileds: %lu\n", (unsigned long)mysql_num_fields(res)); */
                for (queryRet = 0; queryRet != (int)mysql_num_fields(res); ++queryRet)
                {
                    /* printf("%8s ", row[queryRet]); */
                    for (int j = 0; j < strlen(row[queryRet]); j++)
                    {
                        buf[i++] = row[queryRet][j];
                    }
                    buf[i++] = ' ';
                }
                /* printf("\n"); */
                /*这里可以用最简便的方法就是胜哥说的sprintf(buf,..); */

            } while (NULL != (row = mysql_fetch_row(res)));
        }

        mysql_free_result(res);
    }
    return 0;
}

//cp 强

//tree 返回直接打印的内容存在buf里
int tree(MYSQL *conn, int user_id, char *path, char *buf)
{
    getPath(conn,user_id,path,buf);

    int id = getFileId(conn, user_id, buf);

    memset(buf, 0, 4096);
    char file_name[20] = {0};
    getFileName(conn, id, file_name);

    sprintf(buf, "%s\n", file_name);
    //printf("%s\n",file_name);

    treePrint(conn, user_id, id, buf, 5);

    return 0;
}

//递归打印每个分支
int treePrint(MYSQL *conn, int user_id, int id, char *buf, int width)
{
    int temp_id[200] = {0};
    memset(temp_id, 0, sizeof(int) * 200);

    char file_name[20] = {0};
    memset(file_name, 0, 20);

    getls_id(conn, user_id, id, temp_id);
    for (int i = 0; temp_id[i]; i++)
    {
        getFileName(conn, temp_id[i], file_name);
        sprintf(buf, "%s%*s%s%s\n", buf, width - 5, "", "|----", file_name);
        //printf("%*s%s%s\n",width-5,"","|----",file_name);
        memset(file_name, 0, 20);
        //文件夹
        if (getFileTypeFromId(conn, temp_id[i]) == 1)
        {
            treePrint(conn, user_id, temp_id[i], buf, width + 5);
        }
    }
    return 0;
}

//在给定的path路径下创建目录（目录名为path中最后一个字符串）
int makedir(MYSQL *conn, int user_id, char *path)
{

    char buf[200] = {0};
    char save_ptr[20][20] = {0};
    int length = 0;
    int length_path = strlen(path);

    printf("length_path:%d\n",length_path);
    myStrTok(path, save_ptr, &length);

    for(int i = 0 ; i < length ; i++)
        printf("%s\n",save_ptr[i]);
    printf("length:%d\n",length);

    for(int i =length_path-1 ; i >= length_path-1-strlen(save_ptr[length-1]) ; i--)
    {
        printf("1111111\n");
        printf("path:%s\n",path);
        printf("%c\n",path[i]);
        path[i] = '\0';
    }
    printf("path:%s\n",path);

    getPath(conn,user_id,path,buf);
    printf("buf:%s\n",buf);

    int id = getFileId(conn, user_id, buf);
    printf("id:%d\n",id);

    char query[200] = "insert into file (father_id,file_name,file_md5,file_size,type,user_id) values (";
    sprintf(query, "%s%d,'%s',%d,%d,'%s',%d%s;", query, id, save_ptr[length - 1], 0, 0, "d", user_id, ")");
    //printf("%s\n",query);

    int queryRet = mysql_query(conn, query);
    if (queryRet)
    {
        printf("Error query: %s\n", mysql_error(conn));
        return -1;
    }
    else
    {
        return 0;
    }
}

//给出path路径,然后删除该路径下的文件/文件夹（文件/文件夹 名为path中最后一个字符串）
int rm(MYSQL *conn, int user_id, char *path)
{
    char buf[200] = {0};
    getpwd(conn, user_id, buf);
    sprintf(buf, "%s/%s", buf, path);

    int id = getFileId(conn, user_id, buf);

    //不存在返回-1
    if (id == -1)
    {
        printf("要删除的文件不存在\n");
        return -1;
    }

    //目录返回1
    if (getFileTypeFromId(conn, id) == 1)
        rmDirFunc(conn, user_id, id);

    //文件返回2
    if (getFileTypeFromId(conn, id) == 2)
        rmFileFunc(conn, user_id, id);

    return 0;
}

//删除文件夹处理函数，id为文件夹id
int rmDirFunc(MYSQL *conn, int user_id, int id)
{
    int temp_id[200] = {0};
    memset(temp_id, 0, sizeof(int) * 200);

    rmFileFunc(conn, user_id, id);

    //返回所有的孩子id
    getls_id(conn, user_id, id, temp_id);

    for (int i = 0; temp_id[i]; i++)
    {
        //目录返回1
        if (getFileTypeFromId(conn, temp_id[i]) == 1)
            rmDirFunc(conn, user_id, temp_id[i]);
        //文件返回2
        if (getFileTypeFromId(conn, temp_id[i]) == 2)
            rmFileFunc(conn, user_id, temp_id[i]);
    }

    return 0;
}

//删除文件处理函数，id为文件id
int rmFileFunc(MYSQL *conn, int user_id, int id)
{
    //获取文件的md5
    char query[200] = {0};
    char file_md5[20] = {0};

    memset(query, 0, 200);
    sprintf(query, "select file_md5 from file where id = %d", id);
    //printf("%s\n",query);

    MYSQL_RES *res;
    MYSQL_ROW row;

    int queryRet;
    queryRet = mysql_query(conn, query);

    if (queryRet)
    {
        //printf("Error making query: %s\n", mysql_error(conn));
        return -1;
    }
    else
    {
        res = mysql_store_result(conn);
        row = mysql_fetch_row(res);
        if (NULL == row)
        {
            //printf("getNextFileId1:Don't find any data\n");
            return -1;
        }
        else
        {
            memset(file_md5, 0, 20);
            strcpy(file_md5,row[0]);
        }
        mysql_free_result(res);
    }
    //printf("%s\n",file_md5);

    //printf("%s\n",query);

    

    //如果是目录不用管，如果是文件则要考虑md5
    //如果先删file的记录，下面判断类型的接口就调用不了
    if (getFileTypeFromId(conn, id) == 2)
    {
        //file_count--
        //printf("11111111111111111\n");
        memset(query, 0, 200);
        sprintf(query, "update file_real set file_count=file_count-1 where md5 = '%s'", file_md5);
        //printf("%s\n",query);
        mysql_query(conn, query);


        //如果file_count减为0，则删除该文件（即删除该记录）
        memset(query, 0, 200);
        sprintf(query, "select file_count from file_real where md5 = '%s'", file_md5);
        //printf("%s\n",query);
        int file_count = 2; //只要初始化一开始不为0即可

        queryRet = mysql_query(conn, query);

        if (queryRet)
        {
            //printf("Error making query: %s\n", mysql_error(conn));
            return -1;
        }
        else
        {
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);
            if (NULL == row)
            {
                //printf("getNextFileId1:Don't find any data\n");
                return -1;
            }
            else
            {
                file_count = atoi(row[0]);
            }
            mysql_free_result(res);
        }
        //printf("%d\n",file_count);
        if(file_count == 0)
        {
            memset(query, 0, 200);
            sprintf(query, "delete from file_real where md5 = '%s' ", file_md5);
            mysql_query(conn, query);
        }

    }

    //删除文件/文件名
    memset(query, 0, 200);
    sprintf(query, "delete from file where id = %d", id);
    //printf("%s\n",query);

    queryRet = mysql_query(conn, query);
    return 0;
}

//重命名/剪切移动  src:源路径，dest:目的路径
int mv(MYSQL* conn, int user_id, char* src, char* dest)
{

    char srcpath[256] = {0};
    char destpath[256] = {0};
    getPath(conn, user_id, src, srcpath);
    getPath(conn, user_id, dest, destpath);

// printf("src = %s\n",srcpath);
// printf("dest = %s\n",destpath);

    int srcFileId = getFileId(conn, user_id, srcpath);
    int destFileId = getFileId(conn, user_id, destpath);

    int srcFileType = getFileTypeFromId(conn,srcFileId);
    int destFileType = getFileTypeFromId(conn,destFileId);

// printf("srcFileId = %d type = %d\n",srcFileId, srcFileType);
// printf("destFileId = %d type = %d\n",destFileId,destFileType);

    //如果源文件不存在直接返回
    if(srcFileId == -1){
        printf("源文件不存在!!!\n");
        return -1;
    }
    //如果dest存在且是文件也直接返回
    if(destFileId != -1 && destFileType == 2){
        printf("目标文件已存在!!!\n");
        return -1;
    }


    //目标不存在则改名
    if(destFileType != 1){
        //获取文件或目录名字
        char arr[20][20] = {0};
        int len = 0;
        myStrTok(destpath, arr, &len);

        char newarr[20][20] = {0};
        int k = 0, newlen = 0;
        for(int i = 0; i < len;++i){
            if (strcmp(arr[i], ".") == 0) {
                //do nothing
            }
            else if (strcmp(arr[i], "..") == 0) {
                --newlen;
                --k;
            }
            else {
                strcpy(newarr[k], arr[i]);
                ++k;
                ++newlen;
            }

        }
        char name[64] = {0};
        strcpy(name,newarr[newlen-1]);

        printf("name = %s\n",name);

        char query[200]= {0};
        sprintf(query,"UPDATE file SET file_name = '%s' WHERE id = %d", name, srcFileId);

        int queryResult = mysql_query(conn, query);
        if(queryResult){
            //printf("Error making query:%s\n",mysql_error(conn));
            return -1;
        }
        else{
            int ret = mysql_affected_rows(conn);
            if(ret){
                //printf("update success\n");
                return 0;
            }
            else{
                //printf("update fail, mysql_affected_rows:%d\n", ret);
                return -1;
            }
        }

    }

    //目标存在且为目录则移动
    if(destFileId != -1 && destFileType == 1){

        char query[200]= {0};
        sprintf(query,"UPDATE file SET father_id = %d WHERE id = %d", destFileId, srcFileId);

        int queryResult = mysql_query(conn, query);
        if(queryResult){
            //printf("Error making query:%s\n",mysql_error(conn));
            return -1;
        }
        else{
            int ret = mysql_affected_rows(conn);
            if(ret){
                //printf("update success\n");
                return 0;
            }
            else{
                //printf("update fail, mysql_affected_rows:%d\n", ret);
                return -1;
            }
        }

    }


    printf("Done\n");
    return 0;
}

//获取当前工作目录，存到buf里，成功返回当前工作目录id
int getpwd(MYSQL* conn,int user_id, char* buf)
{
    MYSQL_RES *res;
    MYSQL_ROW row;

    char query[300] = "SELECT pwd FROM user WHERE id = ";
    sprintf(query, "%s%d", query, user_id);

    //把SQL语句传递给MySQL
    int queryRet = mysql_query(conn, query);
    if(queryRet){
        printf("Error making query: %s\n", mysql_error(conn));
        return -1;
    }
    else{
        res = mysql_store_result(conn);
        row = mysql_fetch_row(res);

        if(NULL == row){
            printf("getpwd:Don't find any data\n");
            return -1;
        }
        else{
            //printf("%s\n",row[0]);
            strcpy(buf,row[0]);

        }
        mysql_free_result(res);
    }

    int file_id = getFileId(conn, user_id, buf);
    //printf("buf = %s id = %d\n",buf,file_id);
    return file_id;
}

//cd命令的实现，targetDir为要改变到的工作目录,成功返回当前(目标)工作目录id
int changeDir(MYSQL* conn, int user_id, char* dest)
{

    char path[256] = {0};
    getPath(conn, user_id, dest, path);
    // printf("oldpath = %s\n",path);

    char arr[20][20] = {0};
    int len = 0;
    myStrTok(path, arr, &len);

    // for(int i = 0; i < len; ++i){
    //     printf("%s\n",arr[i]);
    // }
    // printf("len = %d\n",len);

    char newarr[20][20] = {0};
    int k = 0, newlen = 0;
    for(int i = 0; i < len;++i){
        if (strcmp(arr[i], ".") == 0) {
            //do nothing
        }
        else if (strcmp(arr[i], "..") == 0) {
            --newlen;
            --k;
        }
        else {
            strcpy(newarr[k], arr[i]);
            ++k;
            ++newlen;
        }

    }
    // for(int i = 0; i < newlen; ++i){
    //     printf("%s\n",newarr[i]);
    // }
    //printf("newlen = %d\n",newlen);

    char newpath [256] = {0};
    char tmp[64] = {0};
    for(int i = 0; i < newlen; ++i){
        memset(tmp,0,sizeof(tmp));
        sprintf(tmp,"/%s",newarr[i]);
        strcat(newpath,tmp);
    }
    
// printf("newpath = %s\n",newpath);



    int newpathid = getFileId(conn,user_id,newpath);
    if(newpathid == -1){
        printf("没有这个目录\n");
        return -1;
    }
// printf("newpathid = %d\n",newpathid);

    if(getFileTypeFromId(conn, newpathid) != 1){
        printf("不是一个目录\n");
        return -1;
    }

    char getpwdpath[128] = {0};
    getpwd(conn, user_id ,getpwdpath);


    if(strcmp(getpwdpath,newpath) == 0){
        //printf("输入了一个相同目录\n");
        return newpathid;
    }
    else{
        char query[200]= {0};
        sprintf(query," UPDATE user SET pwd = '%s' WHERE id =%d",newpath,user_id);

        //puts(query);

        int queryResult = mysql_query(conn, query);

        if(queryResult){
            printf("Error making query:%s\n",mysql_error(conn));
            return -1;
        }
        else{
            int ret = mysql_affected_rows(conn);
            if(ret){
                //printf("update success\n");
            }
            else{
                printf("update fail, mysql_affected_rows:%d\n", ret);
                return -1;
            }
        }
    }
    return newpathid;
}

//exit

/*以下接口是上面是这些命令接口中需要调用*/

//给出当前id,给出一个子目录名，获取该子目录名的id
int getNextFileId(MYSQL *conn, int user_id, const int cur_fileId, char *next)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    unsigned int queryRet;
    char query[300] = {0};
    int id = 0;

    if (strcmp(next, ".") == 0)
    {
        //直接返回当前id
        return cur_fileId;
    }
    else if (strcmp(next, "..") == 0)
    {
        //返回父亲id
        memset(query, 0, sizeof(query));
        sprintf(query, "SELECT father_id FROM file WHERE id = %d", cur_fileId);
        //printf("%s\n",query);
        //把SQL语句传递给MySQL
        queryRet = mysql_query(conn, query);
        if (queryRet)
        {
            //printf("Error making query: %s\n", mysql_error(conn));
            return -1;
        }
        else
        {
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);
            if (NULL == row)
            {
                //printf("getNextFileId1:Don't find any data\n");
                return -1;
            }
            else
            {
                id = atoi(row[0]);
            }
            mysql_free_result(res);
            return id;
        }
    }
    else
    {
        //查询返回孩子id
        memset(query, 0, sizeof(query));
        sprintf(query, "SELECT id FROM file WHERE file_name = '%s' AND father_id = %d AND user_id = %d", next, cur_fileId, user_id);
        //printf("%s\n",query);
        //把SQL语句传递给MySQL
        queryRet = mysql_query(conn, query);
        if (queryRet)
        {
            //printf("Error making query: %s\n", mysql_error(conn));
            return -1;
        }
        else
        {
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);
            if (NULL == row)
            {
                //printf("getNextFileId2:Don't find any data\n");
                return -1;
            }
            else
            {
                id = atoi(row[0]);
            }
            mysql_free_result(res);
            return id;
        }
    }
    return -1;
}

//给一个路径，获取路径的id
int getFileId(MYSQL *conn, int user_id, char *path)
{
    //printf("%s\n",path);
    char arr[20][20] = {0};
    int len = 0;
    myStrTok(path, arr, &len);

    // for(int i = 0; i < len; ++i){
    //     printf("%s\n",arr[i]);
    // }
    // printf("len = %d\n",len);

    int curFileId = 0;
    for (int i = 0; i < len; ++i)
    {
        curFileId = getNextFileId(conn, user_id, curFileId, arr[i]);
    }
    //printf("%d\n",curFileId);
    return curFileId;
}

//给一个文件/文件夹id,获取类型
int getFileTypeFromId(MYSQL *conn, int file_id)
{
    //出错返回-1  目录返回1，文件返回2

    MYSQL_RES *res;
    MYSQL_ROW row;
    unsigned int queryRet;

    char query[300] = {0};

    memset(query, 0, sizeof(query));
    sprintf(query, "SELECT type FROM file WHERE id = %d ", file_id);
    //printf("%s\n",query);
    //把SQL语句传递给MySQL
    queryRet = mysql_query(conn, query);
    if (queryRet)
    {
        printf("Error making query: %s\n", mysql_error(conn));
    }
    else
    {
        res = mysql_store_result(conn);
        row = mysql_fetch_row(res);
        if (NULL == row)
        {
            //printf("Don't find any data\n");
            return -1;
        }
        else
        {
            //printf("%s\n",row[0]);
            if (strcmp(row[0], "d") == 0)
            {
                //printf("111111\n");
                return 1;
            }
            if (strcmp(row[0], "f") == 0)
            {
                //printf("222222\n");
                return 2;
            }
        }

        mysql_free_result(res);
    }

    return -1;
}

//获取绝对路径，str为传入的路径（可能为绝对的也可能为相对的），用buf返回
int getPath(MYSQL *conn, int user_id, const char *str, char *buf)
{
    //如果是绝对路径，就直接返回
    if (str[0] == '/')
    {
        strcpy(buf, str);
    }
    //如果是相对路径，就和当前工作目录拼接起来返回
    else
    {
        char tmp[200] = {0};
        getpwd(conn, user_id, tmp);
        char destpath[200] = {0};
        sprintf(destpath, "%s%s%s", tmp, "/", str);
        strcpy(buf, destpath);
    }
    return 0;
}

//用于分割路径path,得到的每个名称，存入二维数组save_ptr里，和总长度*length
int myStrTok(char *path, char save_ptr[][20], int *length)
{
    char b[20];
    memset(b, 0, sizeof(char) * 20);
    int i = 0, j = 0, k = 0, flag = 0; //flag=1判断前面的字符串加好了没
    for (; path[i] == '/'; i++)
        ;                //单个字符用''，字符串才用""
    for (; path[i]; i++) //0和' '是不一样的
    {

        if (path[i] != '/')
        {
            b[j++] = path[i];
            b[j] = 0;
            flag = 1;
        }
        else
        {
            if (flag == 1)
            {
                strcpy(save_ptr[k++], b);
                memset(b, 0, sizeof(char) * 20);
                j = 0;
                flag = 0;
            }
        }
    }
    *length = k;
    if ((path[i - 1] != '/') && (flag == 1))
    {
        strcpy(save_ptr[k], b);
        (*length)++;
    }

    return 0;
}

//根据目录id获取该目录下所有文件/文件夹id，并存入temp_id数组里
int getls_id(MYSQL *conn, int user_id, int id, int temp_id[])
{
    char query[200] = {0};
    sprintf(query, "select id from file where father_id = %d and user_id = %d", id, user_id);

    MYSQL_RES *res;
    MYSQL_ROW row;

    //printf("%s\n", query);

    int queryRet = mysql_query(conn, query);
    if (queryRet)
    {
        printf("Error query: %s\n", mysql_error(conn));
        return -1;
    }
    else
    {
        res = mysql_store_result(conn);
        /* printf("mysql_affacted_rows: %lu\n", (unsigned long)mysql_affected_rows(conn)); */
        /* printf("mysql_num_rows: %lu\n", (unsigned long)mysql_num_rows(res)); */

        row = mysql_fetch_row(res);
        if (NULL == row)
        {
            //printf("Don't query any data\n");
        }
        else
        {
            int i = 0;
            do
            {
                /* printf("num=%d\n",mysql_num_fields(res));//列数 */

                //每次for循环打印一整行的内容
                for (queryRet = 0; queryRet < mysql_num_fields(res); ++queryRet)
                {
                    temp_id[i++] = atoi(row[queryRet]);
                    //printf("%d ", atoi(row[queryRet]));
                }

            } while (NULL != (row = mysql_fetch_row(res)));
        }

        mysql_free_result(res);
    }
    return 0;
}

//根据id获取file_name
int getFileName(MYSQL *conn, int id, char *file_name)
{
    char query[200] = {0};
    sprintf(query, "select file_name from file where id = %d ", id);
    //printf("%s\n",query);

    MYSQL_RES *res;
    MYSQL_ROW row;

    //printf("%s\n", query);

    int queryRet = mysql_query(conn, query);
    if (queryRet)
    {
        printf("Error query: %s\n", mysql_error(conn));
        return -1;
    }
    else
    {
        res = mysql_store_result(conn);
        /* printf("mysql_affacted_rows: %lu\n", (unsigned long)mysql_affected_rows(conn)); */
        /* printf("mysql_num_rows: %lu\n", (unsigned long)mysql_num_rows(res)); */

        row = mysql_fetch_row(res);
        if (NULL == row)
        {
            //printf("Don't query any data\n");
        }
        else
        {
            strcpy(file_name, row[0]);
        }

        mysql_free_result(res);
    }
    return 0;
}