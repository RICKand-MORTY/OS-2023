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

long callback_sys_open(struct pt_regs *regs)
{
	char *pathname = (char *)regs->a0;
	int flags = (int)regs->a1;
	
	char * path = NULL;
	long pathlen = 0;
	long error = 0;
	struct dir_entry * dentry = NULL;
	struct file * filp = NULL;
	struct file ** f = NULL;
	int fd = -1;
	int i;
	printk("sys_open!\n");
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
	if(dentry->dir_inode->attribute == FS_ATTR_DIR && flags & O_DIRECTORY == 0)
	{
		return -EISDIR;
	}
	filp = alloc_pgtable();
	if(filp == 1)
	{
		return -ENOMEM;
	}
	filp->dentry = dentry;
	filp->mode = flags;
	filp->f_ops = dentry->dir_inode->f_ops;
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
	for(i = 0;i < TASK_FILE_MAX;i++)
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

long callback_sys_close(struct pt_regs *regs)
{
	int fd = (int)regs->a0;		//file desc
	struct file * filp = NULL;

	printk("sys_close:%d\n",fd);
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
	printk("sys_read:%d\n",fd);
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

long callback_sys_openat(int fd, char *filename, int flags, int mode)
{
	return 1;
}

long callback_sys_malloc()
{
	return page_alloc();
}

#define __SYSCALL(nr, sym) [nr] = (syscall_fun)callback_##sym,

const syscall_fun syscall_table[TOTAL_SYSCALLS] = {
	__SYSCALL(SYS_STDOUT, sys_stdout)
	__SYSCALL(SYS_nanosleep, sys_sleep)
	__SYSCALL(SYS_sched_yield, sys_sched_yield)
	__SYSCALL(SYS_clone, sys_clone)
	__SYSCALL(SYS_MALLOC, sys_malloc)
	__SYSCALL(SYS_open, sys_open)
	__SYSCALL(SYS_openat, sys_openat)
	__SYSCALL(SYS_close, sys_close)
	__SYSCALL(SYS_read, sys_read)
};
