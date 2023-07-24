#ifndef _SHELL_H
#define _SHELL_H


struct	buildincmd
{
	char *name;
	int (*function)();
};

extern char *current_dir;

int parse_command(void);

#endif