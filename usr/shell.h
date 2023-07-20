#ifndef _SHELL_H
#define _SHELL_H


struct	buildincmd
{
	char *name;
	int (*function)();
};


int parse_command(void);

#endif