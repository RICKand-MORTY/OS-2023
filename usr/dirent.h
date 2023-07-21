#ifndef _DIRENT_H
#define _DIRENT_H


struct dirent
{
	long d_offset;		
	long d_type;
	long d_namelen;
	char d_name[];
};

struct DIR
{
	int fd;
	int buf_pos;
	int buf_end;
	char buf[256];
};

struct DIR* opendir(const char *path);
int closedir(struct DIR *dir);
struct dirent *readdir(struct DIR *dir);

#endif