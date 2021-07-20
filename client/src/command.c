#define _GNU_SOURCE
#include "../head/func.h"
#include "../head/md5.h"
#include "../head/cilent.h"


//将命令和参数赋值给command结构体变量
int GetCommand(pcommand_t pcommand, char *buf)
{
	char *word[10] = {0};			//用来保存字符串转化的结果
	int i = TransToWord(buf, word); //i代表这条命令的参数有几个

	//如果用户输入的全是空格，那么word[0]中就只有一个'\0'，长度就是1
	if (strlen(word[0]) == 0)
	{
		//这里就什么也不做，直接返回 -1
		return -1;
	}

	//用户输入的不是空格
	pcommand->c_argsnum = i;			  //参数的个数有i个
	strcpy(pcommand->c_content, word[0]); //拆分出来的第一个单词就是命令
	if (i == 1)							  //有一个参数
	{
		strcpy(pcommand->c_args1, word[1]);
	}
	else if (i == 2) //有两个参数
	{
		strcpy(pcommand->c_args1, word[1]);
		strcpy(pcommand->c_args2, word[2]);
	}

	return 0; //正常赋值，返回 0
}

//把指令字符串按照空格分割为单词存储
int TransToWord(char *str, char **word)
{ //word是一个指针数组，在传递参数时会退化为二级指针
	int i = 0;
	char *p1 = str, *p2 = str; //p1和p2分别指向每一个单词开头和单词结尾
	while (1)
	{
		while (*p1 == ' ') //p1指向的内容 是 空格时，p1，p2都像后移动，直到 其指向内容不是空格
		{
			p1++;
			p2++;
		}
		while (*p2 != ' ' && *p2 != '\0' && *p2 != '\n') //p1不动,p2向后移动，直到指向的内容是 空格 或者 \0
		{
			p2++;
		}
		if (*p2 == '\0' || *p2 == '\n') //判断p2是不是指向字符串的末尾了
		{
			break;
		}
		*p2 = '\0';
		p2++;
		word[i++] = p1; //得到了一个完整的单词，将他的地址给word[]
		p1 = p2;		//让p1 和 p2 指向相同位置，开始下一次循环
	}
	*p2 = '\0';
	word[i] = p1; //保存最后一个单词的地址

	return i;
}

//下载命令 执行的 函数
int DownloadCommand(pcommand_t pcommand, int sfd)
{
	off_t recvsize = 0;
	char buf[4096] = {0};
	off_t filesize = 0;
	int ret = 0, fd = 0;

	//根据输入的 文件名，创建或者打开该文件
	fd = open(pcommand->c_args1, O_RDWR | O_CREAT, 0766);
	ERROR_CHECK(fd, -1, "open");

	//获得文件大小，设置偏移量，并将文件大小赋值给command结构体变量
	struct stat fileinfo;
	memset(&fileinfo, 0, sizeof(fileinfo));
	ret = fstat(fd, &fileinfo);
	ERROR_CHECK(ret, -1, "fstat");
	recvsize = fileinfo.st_size; //用于断点续传，recvsize 表示 偏移量
	ret = lseek(fd, recvsize, SEEK_SET);
	ERROR_CHECK(ret, -1, "lseek"); //改变当前文件大小偏移ptr，使其能够指向正确的位置
	sprintf(pcommand->c_args2, "%ld", fileinfo.st_size);
	pcommand->c_argsnum += 1;

	//现在就可以把 这个命令 发送出去了
	ret = send(sfd, pcommand, sizeof(command_t), 0);
	ERROR_CHECK(ret, -1, "send_command");
	// printf("我发给对方的我这边的这个文件的大小recvsize = %d\n", atoi(pcommand->c_args2));

	ret = recv(sfd, &filesize, sizeof(off_t), MSG_WAITALL);
	ERROR_CHECK(ret, -1, "recv_filesize");
	//先接收off_t个字节，接收文件的大小

	printf("file size = %ld\n", filesize);
	printf("need transport size = %ld\n", filesize - fileinfo.st_size);
	while (recvsize < filesize)
	{ //已经接收到的字节 比 文件总字节 小，就循环
		memset(buf, 0, sizeof(buf));
		ret = recv(sfd, buf, 4095, 0);

		ERROR_CHECK(ret, -1, "recv_content");
		ret = write(fd, buf, ret); // ret 本次接收到的字节，将接收到的数据写入文件当中		
		ERROR_CHECK(ret, -1, "write");

		recvsize += ret; // 已经写入到文件中的字节数

		if (recvsize % 102400 == 0) // 每写入100字节 就打印一次进度
		{
			float rate = (float)recvsize / filesize * 100;
			printf("rate = %5.2f%%\r", rate);
			fflush(stdout);
		}
	}

	if (recvsize == filesize)
	{ //判断，如果退出接收时，已经写入的大小 = 文件大小，说明下载完成
		printf("download complished\n");
	}
	else
	{
		printf("我这边的文件大小，和，文件总大小 不匹配\n");
	}
	close(fd);
	return 1;
}

//上传命令 执行的 函数
int UploadCommand(pcommand_t pcommand, int sfd)
{
	printf("执行上传命令\n");
	off_t sendsize = 0;
	int fd = 0;
	int ret = 0;

	//获得要上传文件的md5码，并赋值给command结构体变量
	char file_path[64];
	sprintf(file_path, "%s", pcommand->c_args1);
	char md5_str[MD5_STR_LEN + 1];
	ret = Compute_file_md5(file_path, md5_str);
	printf("[file - %s] md5 value: ", file_path);
	printf("%s\n", md5_str);

	strcpy(pcommand->c_args3, md5_str);
	pcommand->c_argsnum += 1;

	//根据输入的 文件名，打开该文件,
	fd = open(pcommand->c_args1, O_RDONLY, 0666);
	ERROR_CHECK(fd, -1, "open");

	//获得文件大小, 把文件大小，放在command结构体中
	struct stat fileinfo;
	memset(&fileinfo, 0, sizeof(fileinfo));
	ret = fstat(fd, &fileinfo);
	ERROR_CHECK(ret, -1, "fstat");
	sprintf(pcommand->c_args2, "%ld", fileinfo.st_size);
	pcommand->c_argsnum += 1;

	//command结构体的参数已经赋值完毕，接下来要做的是，发送
	ret = send(sfd, pcommand, sizeof(command_t), 0);
	ERROR_CHECK(ret, -1, "send_command");

	//接收 服务器 发给我的 偏移量
	ret = recv(sfd, &sendsize, sizeof(off_t), MSG_WAITALL);
	ERROR_CHECK(ret, -1, "recv_offset");

	printf("file size = %ld\n", fileinfo.st_size);
	printf("need transport size = %ld\n", fileinfo.st_size - sendsize);
	off_t sendsize1 = sendsize;
	off_t size = 0;
	size = sendfile(sfd, fd, &sendsize, fileinfo.st_size);

	sendsize1 += size;
	if (sendsize1 == fileinfo.st_size)
	{
		printf("upload complished\n");
		close(fd);
	}
	else
	{
		printf("sendsize + size != filesize\n");
	}
	return 1;
}

//其他命令
int OtherCommand(pcommand_t pcommand, int sfd)
{
	int ret = send(sfd, pcommand, sizeof(command_t), 0);
	ERROR_CHECK(ret, -1, "send_command");
	return 0;
}

