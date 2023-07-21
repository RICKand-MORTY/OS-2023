#include "dirent.h"
#include "user_syscall.h"
#include <sysflags.h>

#define NULL ((void*)0)

/*打开目录*/
struct DIR* opendir(const char *path)
{
	int fd = 0;
	struct DIR* dir = NULL;

	fd = open(path,O_DIRECTORY);

	if(fd >= 0)
		dir = (struct DIR*)malloc(1);
	else
		return NULL;
	memset(dir,0,sizeof(struct DIR));
	dir->fd = fd;
	dir->buf_pos = 0;
	dir->buf_end = 256;
	
	return dir;
}

/*关闭目录*/
int closedir(struct DIR *dir)
{
	close(dir->fd);
	free(dir, 1);
	return 0;
}

/*从目录中读取数据，返回dirent指针*/
struct dirent *readdir(struct DIR *dir)
{
	int len = 0;
	memset(dir->buf,0,256);
	len = getdents(dir->fd,(struct dirent *)dir->buf,256);
	if(len > 0)
		return (struct dirent *)dir->buf;
	else
		return NULL;
}

