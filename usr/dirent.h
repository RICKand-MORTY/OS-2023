#ifndef _DIRENT_H
#define _DIRENT_H


/*暂未按照标准*/
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


#endif