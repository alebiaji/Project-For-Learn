#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <stdio.h>
#include <mysql/mysql.h>
MYSQL *database_connect();
int database_operate(MYSQL *db_connect, const char *query, char ***result);
void database_close(MYSQL *db_connect);

#endif