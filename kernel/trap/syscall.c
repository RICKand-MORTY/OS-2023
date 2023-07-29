#include <syscall.h>
#include <process.h>
#include "../../usr/syscall_num.h"
#include "../../lib/lib.h"
#include "../../lib/printk.h"
#include <memory.h>
#include <fat32.h>
#include <VFS.h>
#include <sysflags.h>
#include <error.h>
#include <buf.h>
#include <elf.h>
#include <elf_loader.h>
#include "../../usr/shell.h"

void syscall_handler(struct pt_regs *regs)
{
	return deal_syscall(regs, regs->a7);
}

void deal_syscall(struct pt_regs *regs, unsigned long syscall_num)
{
	syscall_fun callback;
    long ret;
	
	if(syscall_num < TOTAL_SYSCALLS)
	{
		callback = syscall_table[syscall_num];
		ret = callback(regs);
	}
	else
	{
		ret = -1;
	}
	regs->a0 = ret;
}

long callback_sys_sleep(struct pt_regs *regs)
{
	delay(regs->a0);
}

long callback_sys_stdout(struct pt_regs *regs)
{
	printk("%s",(char *)regs->a0);
}

//not achieve
long callback_sys_sched_yield(PCB  *task_struct)
{
	
}

//need rewrite
long callback_sys_clone(struct pt_regs *regs)
{
	return do_fork(regs->a0, regs->a1,regs->a2);
}
int fopen(char *pathname, int flags)
{
	int size = 0;
	char * path = NULL;
	long pathlen = 0;
	long error = 0;
	struct dir_entry * dentry = NULL;
	struct file * filp = NULL;
	struct file ** f = NULL;
	int fd = -1;
	int i;

	//printk("sys_open!\n");
	path = alloc_pgtable();
	if(path == 1)
	{
		return -ENOMEM;
	}
	pathlen = strlen(pathname);
	if(pathlen == 0)
	{
		page_free_addr(path);
		return -EFAULT;
	}
	else if(pathlen >= PAGE_SIZE)
	{
		page_free_addr(path);
		return -ENAMETOOLONG;
	}
	strcpy(path, pathname);
	dentry = path_walk(path,0);
	page_free_addr(path);
	if(dentry != NULL)
	{
		printk("find %s!\n", pathname);
	}
	else
	{
		printk("can't find %s\n", pathname);
		return -ENOENT;
	}
	if((flags & O_DIRECTORY) && (dentry->dir_inode->attribute != FS_ATTR_DIR))
		return -ENOTDIR;
	if(!(flags & O_DIRECTORY) && (dentry->dir_inode->attribute == FS_ATTR_DIR))
		return -EISDIR;
	
	filp = alloc_pgtable();
	if(filp == 1)
	{
		return -ENOMEM;
	}
	filp->dentry = dentry;
	filp->mode = flags;
	filp->f_ops = dentry->dir_inode->f_ops;
	//filp->dentry->dir_inode->sb = root_sb;
	if(filp->f_ops && filp->f_ops->open)
		error = filp->f_ops->open(dentry->dir_inode,filp);
	if(error != 1)
	{
		page_free_addr(filp);
		return -EFAULT;
	}
	if(filp->mode & O_TRUNC)
	{
		filp->dentry->dir_inode->file_size = 0;
	}
	if(filp->mode & O_APPEND)
	{
		filp->position = filp->dentry->dir_inode->file_size;
	}
	f = get_current_task()->file_struct;
	//1 is STDIN 2 is STDOUT 3 is STDERR
	for(i = 3;i < TASK_FILE_MAX;i++)
		if(f[i] == NULL)
		{
			fd = i;
			break;
		}
	if(i == TASK_FILE_MAX)
	{
		page_free_addr(filp);
		return -EMFILE;
	}
	f[fd] = filp;

	return fd;
}
long callback_sys_open(struct pt_regs *regs)
{
	char *pathname = (char *)regs->a0;
	int flags = (int)regs->a1;
	long fd = fopen(pathname, flags);
	if(fd < 0)
	{
		return -1;
	}
	else
	{
		return fd;
	}
}

long callback_sys_close(struct pt_regs *regs)
{
	int fd = (int)regs->a0;		//file desc
	struct file * filp = NULL;

	int size = 0;
	if(fd < 0 || fd >= TASK_FILE_MAX)
	{
		//return -EBADF;
		return -1;
	}
	filp = get_current_task()->file_struct[fd];
	if(filp->f_ops && filp->f_ops->close)
	{
		filp->f_ops->close(filp->dentry->dir_inode, filp);
	}
	page_free_addr(filp);
	get_current_task()->file_struct[fd] = NULL;
	return 0;
}

long callback_sys_read(struct pt_regs *regs)
{
	int fd = regs->a0;
	unsigned char *buf = (unsigned char *)regs->a1;
	size_t count = (size_t)regs->a2;
	struct file* filp = NULL;
	long ret = 0;
	//printk("sys_read:%d\n",fd);
	if(fd < 0 || fd >= TASK_FILE_MAX)
	{
		return -1;
	}
	if(count < 0)
	{
		return -1;
	}
	filp = get_current_task()->file_struct[fd];
	if(filp->f_ops && filp->f_ops->read)
	{
		ret = filp->f_ops->read(filp, buf, count, &filp->position);
	}
	return ret;
}

long callback_sys_write(struct pt_regs *regs)
{
	int fd = (int)regs->a0;
	unsigned char* buf = regs->a1;
	unsigned int count = regs->a2;

	struct file * filp = NULL;
	long ret = 0;

	//printk("sys_write:%d\n",fd);
	if(fd < 0 || fd >= TASK_FILE_MAX)
		return -EBADF;
	if(count < 0)
		return -EINVAL;

	if(fd == STDOUT)
	{
		printk("%s",buf);
		return 0;
	}
	filp = get_current_task()->file_struct[fd];
	if(filp->f_ops && filp->f_ops->write)
		ret = filp->f_ops->write(filp,buf,count,&filp->position);
	if(ret < 0)
	{
		return -1;
	}
	else
	{
		return ret;
	}
}

//设置文件描述符的访问位置
long callback_sys_lseek(struct pt_regs *regs)
{
	int fd = (int)regs->a0;
	long offset = (long)regs->a1;
	int whereat = (int)regs->a2;

	struct file * filp = get_current_task()->file_struct[fd];
	long ret = 0;

	//printk("sys_lseek:%d\n", fd);
	if(fd < 0 || fd >= TASK_FILE_MAX)
		return -EBADF;
	if(whereat < 0 || whereat >= SEEK_MAX)
		return -EINVAL;
	if(filp->f_ops && filp->f_ops->lseek)
	{
		ret = filp->f_ops->lseek(filp, offset, whereat);
	}
	return ret;
}

long callback_sys_openat(int fd, char *filename, int flags, int mode)
{
	return 1;
}

long callback_sys_free(struct pt_regs *regs)
{
	void * addr = regs->a0;
	int count = regs->a1;
	return more_page_free(addr, count);
}

long callback_sys_malloc(struct pt_regs *regs)
{
	int need = regs->a0;
	return more_page_alloc(need);
}

long callback_sys_execve(struct pt_regs *regs)
{
	char *path = (char *)regs->a0;
	char *argv = (char *)regs->a1;
	char *envp = (char *)regs->a2;

	loader_env_t env;
	ELFExec_t *exec;
	unsigned int need_page = 0;
	unsigned long filesize = 0;
	struct file * filp =NULL;
	int fd = fopen(path, 0);

	if(fd < 0)
	{
		return -1;
	}
	filp = get_current_task()->file_struct[fd];
	filesize = filp->dentry->dir_inode->file_size;
	need_page = ((filesize + PAGE_SIZE - 1) & ~(PAGE_SIZE -1)) / PAGE_SIZE;
	memset(&env, NULL, sizeof(env));
	exec = more_page_alloc(1);
	memset(exec, 0, 1 * PAGE_SIZE);
	if(exec == 1)
	{
		return -1;
	}
	exec->file_size = filesize;
	load_elf(path, env, exec, NULL, NULL, fd);
	return (long)exec;
}
/*读取指定目录下的目录项*/
long callback_sys_getdents64(struct pt_regs *regs)
{
	int fd = (int)regs->a0;
	void * dirent = (void*)regs->a1;
	unsigned long count = (regs->a2);

	struct file * filp = NULL;
	unsigned long ret = 0;
	if(fd < 0 || fd > TASK_FILE_MAX)
	{
		return -1;
	}
	if(count < 0)
	{
		return -1;
	}
	filp = get_current_task()->file_struct[fd];
	if(filp->f_ops && filp->f_ops->readdir)
	{
		ret = filp->f_ops->readdir(filp, dirent, &fill_dentry);
	}
	return ret;
}

long callback_sys_getcwd(struct pt_regs *regs)
{
	char *buf = regs->a0;
	unsigned int size = regs->a1;
	if(size < strlen(current_dir))
	{
		return NULL;
	} 
	strcpy(buf, current_dir);
	return current_dir;
}

long callback_sys_exit(struct pt_regs *regs)
{
	int ec = regs->a0;
	printk("ec: %d\n",ec);
	return 0;
}

long callback_sys_times(struct pt_regs *regs)
{

}

long callback_sys_chdir(struct pt_regs *regs)
{
	char * filename = (char *)regs->a0;
	unsigned long pathlen = 0;
	struct dir_entry * dentry = NULL;
	char * path = more_page_alloc(1);
	if(path == 1)
	{
		return -ENOMEM;
	}
	memset(path, 0, PAGE_SIZE);
	pathlen = strlen(filename);
	if(pathlen == 0 || pathlen >= PAGE_SIZE)
	{
		return -1;
	}
	strcpy(path, filename);
	dentry = path_walk(path, 0);
	more_page_free(path, 1);
	if(dentry == NULL)
	{
		printk("No such file or directory!\n");
		return -ENOENT;
	}
	if(dentry->dir_inode->attribute != FS_ATTR_DIR)
	{
		printk("%s not a directory!\n", filename);
		return -ENOTDIR;
	}
	return 0;
}

#define __SYSCALL(nr, sym) [nr] = (syscall_fun)callback_##sym,

const syscall_fun syscall_table[TOTAL_SYSCALLS] = {
	__SYSCALL(SYS_STDOUT, sys_stdout)
	__SYSCALL(SYS_nanosleep, sys_sleep)
	__SYSCALL(SYS_sched_yield, sys_sched_yield)
	__SYSCALL(SYS_clone, sys_clone)
	__SYSCALL(SYS_MALLOC, sys_malloc)
	__SYSCALL(SYS_free, sys_free)
	__SYSCALL(SYS_open, sys_open)
	__SYSCALL(SYS_openat, sys_openat)
	__SYSCALL(SYS_close, sys_close)
	__SYSCALL(SYS_read, sys_read)
	__SYSCALL(SYS_write, sys_write)
	__SYSCALL(SYS_execve, sys_execve)
	__SYSCALL(SYS_lseek, sys_lseek)
	__SYSCALL(SYS_getdents64, sys_getdents64)
	__SYSCALL(SYS_getcwd, sys_getcwd)
	__SYSCALL(SYS_exit, sys_exit)
	__SYSCALL(SYS_times, sys_times)
	__SYSCALL(SYS_chdir, sys_chdir)
};
