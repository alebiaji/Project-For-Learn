#include "../head/head.h"

/**
 * 返回值：成功返回MySQL连接，失败返回(MYSQL *)-1
 */
MYSQL *database_connect()
{
	MYSQL *db_connect = NULL;

	/*char *server = "localhost";
	char *user = "root";
	char *password = "12345678";
	char *database = "project";*/

	char *server = "121.36.5.233";
	char *user = "demo";
	char *password = "Asd404!@#";
	char *database = "project";

	//初始化
	db_connect = mysql_init(NULL);
	if (db_connect == NULL)
	{
		printf("init\n");
		return (MYSQL *)-1;
	}

	//连接数据库，看连接是否成功，只有成功才能进行后面的操作
	if (!mysql_real_connect(db_connect, server, user, password, database, 0, NULL, 0))
	{
		printf("real_connect\n");
		return (MYSQL *)-1;
	}
	else{
		return db_connect;
	}
	
}

/**
 * 关闭数据库连接
 */
void database_close(MYSQL *db_connect)
{
	mysql_close(db_connect);
}

/**
 * 参数1：database_connect函数返回的 MySQL连接
 * 参数2：MySQL操作的字符串，如：select * from tb_name。不加分号。
 * 参数3：查询得到的字符串数组（传二级指针的地址，传出参数，调用者不用申请空间）
 * 返回值：成功返回查询结果数量，错误返回-1
 */
int database_operate(MYSQL *db_connect, const char *query, char ***result)
{
	MYSQL_RES *res; //该结构代表返回行的查询结果（SELECT, SHOW）。
	MYSQL_ROW row;

	long ret = 0;
	unsigned int rowNum = 0, i;

	//把SQL语句传递给MySQL
	unsigned int queryRet = mysql_query(db_connect, query);
	if (queryRet != 0)
	{
		return -1;
	}
	else
	{
		switch (query[0])
		{
		case 's': //select
			//用mysql_num_rows可以得到查询的结果集有几行
			//要配合mysql_store_result使用
			if(NULL == result){
				char **result_opt = NULL;
				result = &result_opt;
			}

			res = mysql_store_result(db_connect);

			rowNum = mysql_num_rows(res);

			*result = (char **)calloc(rowNum, sizeof(char *));
			for (i = 0; i < rowNum; i++)
			{
				(*result)[i] = (char *)calloc(50, sizeof(char));
			}

			row = mysql_fetch_row(res);

			i = 0;
			if (row == NULL)
			{
				printf("Don't find any data.\n");
				return -1;
			}
			else
			{
				do
				{
					for (queryRet = 0; queryRet < mysql_num_fields(res); queryRet++)
					{
						sprintf((*result)[i], "%s %s", (*result)[i], row[queryRet]);
					}
					i++;
				} while ((row = mysql_fetch_row(res)) != NULL);
			}
			mysql_free_result(res);
			break;

		case 'i':
		case 'd':
		case 'u':
			ret = mysql_affected_rows(db_connect);
			if (ret)
			{
				printf("operate success, mysql affected row : %lu\n", ret);
			}
			else
			{
				printf("operate failed, mysql affected rows : %lu\n", ret);
				return -1;
			}
			break;

		default:
			break;
		}
	}

	return rowNum;
}
