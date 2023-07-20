#include <plic.h>
#include "shell.h"
#include "user_syscall.h"
#include "../lib/lib.h"

#define NULL ((void *)0)
#define MAXARGSIZE  32
#define MAXARGNUM   10
char argv[MAXARGNUM][MAXARGSIZE];
int argc = 0;

char *current_dir = NULL;

int cd_command(){}
int ls_command(){}
int pwd_command()
{
	if(current_dir)
		print("%s\n",current_dir);
	return 0;
}
int cat_command(){}
int touch_command(){}
int rm_command(){}
int mkdir_command(){}
int rmdir_command(){}
int exec_command(){}
int shutdown_command()
{
    
}


/*command list*/
struct	buildincmd shell_internal_cmd[] = 
{
	{"cd",cd_command},
	{"ls",ls_command},
	{"pwd",pwd_command},
	{"cat",cat_command},
	{"touch",touch_command},
	{"rm",rm_command},
	{"mkdir",mkdir_command},
	{"rmdir",rmdir_command},
	{"exec",exec_command},
	{"shutdown",shutdown_command},
    {"exec",exec_command}
};

static int run_command(int index)
{
	int res = 0;
    print("run_command %s\n",shell_internal_cmd[index].name);
	res = shell_internal_cmd[index].function();
    if(res)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static int find_cmd()
{
	int i = 0;
	for(i = 0;i<sizeof(shell_internal_cmd)/sizeof(struct buildincmd);i++)
		if(!strcmp(argv[0],shell_internal_cmd[i].name))
			return i;
	return -1;
}

/*解析指令和参数,失败返回-1，成功返回0*/
int parse_command(void)
{
    int i = 0;
    int j = 0;
    int argv_count = 0;
    int k = 0;
    while(keyboard.buf[j] == ' ')
    {
        j++;
    }
    for(i = j; i < keyboard.len; i++)
    {
        if(keyboard.buf[i] != ' ' && (keyboard.buf[i + 1] == ' ' || keyboard.buf[i + 1] == '\0'))
        {
            argv[argv_count][k] = keyboard.buf[i];
            k++;
            argv[argv_count][k] = '\0';
            k = 0;
            argv_count++;
        }
        else
        {
            argv[argv_count][k] = keyboard.buf[i];
            k++;
        }
    }
    argc = argv_count;
    print("command argc = %d\n", argc);
    if(!argc)
    {
        return -1;
    }
    memset(&keyboard, 0, sizeof(keyboard));
    for(i = 0 ; i < argc; i++)
    {
        print("argv[%d]:%s'\t",i,argv[i]);
    }
    print("\n");
    i = find_cmd();
    if(i == -1)
    {
        return -1;
    }
    run_command(i);
    return 0;
}