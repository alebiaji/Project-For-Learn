#ifndef __FILETRANS_H__
#define __FILETRANS_H__

#include "database.h"
#include "head.h"

#define SQL_NOT_HAVE_FILE -2
#define CLIENT_DISCONNECTION -3

int user_download(MYSQL *db_connect, int clientfd, int father_id, const char *filename, off_t file_offset);
int user_upload(MYSQL *db_connect, int clientfd, int father_id, const char *filename, const char *md5, off_t filesize, int user_id);

#endif