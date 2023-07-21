#include <plic.h>
#include "shell.h"
#include "dirent.h"
#include "user_syscall.h"
#include "../lib/lib.h"
#include <sysflags.h>
#include <memory.h>

#define NULL ((void *)0)
#define MAXARGSIZE  32
#define MAXARGNUM   10
char argv[MAXARGNUM][MAXARGSIZE];
int argc = 0;

char *current_dir = "/";

int cd_command(){}

int ls_command()
{
    struct DIR* dir = NULL;
    struct dirent * buf = NULL;
    int i = 0;
    dir = opendir(current_dir);
    unsigned int name_pos = 0;
    unsigned int name_num = 0; 
    char *str = malloc(1);
    //print("ls_command opendir:%d\n",dir->fd);
    while(1)
    {
        buf = readdir(dir);
        if(buf == NULL)
        {
            break;
        }
        memcpy(str + name_pos,buf->d_name, buf->d_namelen);
        name_num++;
        name_pos+=32;
    }
    name_pos = 0;
    print("content:\n");
    for(; i < name_num; i++, name_pos+=32)
    {
        print("%s\n",(str + name_pos));
    }
    print("\n\n");
    free(str, 1);
    closedir(dir);
}

int pwd_command()
{
	if(current_dir)
		print("%s\n",current_dir);
	return 0;
}

int cat_command()
{
    int len = 0;
    char * filename = NULL;
    int fd = 0;
    char * buf = NULL;
    int filepath_len = 0;
    unsigned int needpage = 0;
    int file_len = 0;
    
    len = strlen(current_dir);
    filepath_len = len + strlen(argv[1]);
    filename = malloc(1);
    memset(filename, 0, filepath_len + 2);      //2 is user for / and \0
    strcpy(filename, current_dir);
    if(len > 1)
    {
        filename[len] = '/';
    }
    strcat(filename, argv[1]);
    print("filename:%s\n",filename);
    fd = open(filename, 0);
    if(fd == -1)
    {
        print("error:not that file!\n");
        return -1;
    }
    file_len = lseek(fd, 0, SEEK_END);
    needpage = ((file_len + 1 + PAGE_SIZE - 1) & ~(PAGE_SIZE -1)) / PAGE_SIZE; 
    buf = malloc(needpage);
    memset(buf, 0, needpage * PAGE_SIZE);
    lseek(fd, 0, SEEK_SET);
    len = read(fd, buf, file_len);
    print("\ncontent:\n%s\n",buf);
    close(fd);
    return 0;
}

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
    //print("run_command %s\n",shell_internal_cmd[index].name);
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
	print("command not find!\n");
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
            i++;
        }
        else
        {
            argv[argv_count][k] = keyboard.buf[i];
            k++;
        }
    }
    argc = argv_count;
    //print("command argc = %d\n", argc);
    if(!argc)
    {
        return -1;
    }
    memset(&keyboard, 0, sizeof(keyboard));
    /*for(i = 0 ; i < argc; i++)
    {
        print("argv[%d]:%s'\t",i,argv[i]);
    }
    print("\n");*/
    i = find_cmd();
    if(i == -1)
    {
        return -1;
    }
    run_command(i);
    argc = 0;
    memset(argv, 0, MAXARGNUM * MAXARGSIZE);
    return 0;
}