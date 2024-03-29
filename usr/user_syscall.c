#include "user_syscall.h"
#include <syscall.h>
#include "../lib/printk.h"
#include "../lib/lib.h"
#include "syscall_num.h"
#include <process.h>
#include <elf_loader.h>
#include <sysflags.h>

//char buf[4096]={0};
typedef void (entry_t)(void);

_u64 internal_syscall(long n, _u64 _a0, _u64 _a1, _u64 _a2, _u64
		_a3, _u64 _a4, _u64 _a5) {
	register _u64 a0 asm("a0") = _a0;
	register _u64 a1 asm("a1") = _a1;
	register _u64 a2 asm("a2") = _a2;
	register _u64 a3 asm("a3") = _a3;
	register _u64 a4 asm("a4") = _a4;
	register _u64 a5 asm("a5") = _a5;
	register long syscall_id asm("a7") = n;
	asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"
			(a5), "r"(syscall_id));
	return a0;
}

unsigned long sleep_proc(int time)
{
	return SYSCALL_1(SYS_nanosleep, time);
}

unsigned long print(const char *fmt,...)
{
	int num=0;
    va_list args;
    va_start(args,fmt);
    num=vsprintf(buf,fmt,args);
    va_end(args);
	return SYSCALL_1(SYS_STDOUT, (char *)buf);
}

unsigned long printf(const char *fmt,...)
{
	int num=0;
    va_list args;
    va_start(args,fmt);
    num=vsprintf(buf,fmt,args);
    va_end(args);
	return SYSCALL_1(SYS_STDOUT, (char *)buf);
}

_u64 clone(int (*thread_callback)(void *arg), void *child_stack, int flag, void *arg)
{
	return __clone(thread_callback, child_stack, flag, arg);
}

_u64 malloc(int count)
{
	return SYSCALL_1(SYS_MALLOC, count);
}

_u64 free(void * addr, int count)
{
	return SYSCALL_2(SYS_free, addr, count);
}

//not achieve
unsigned long need_to_schedule()
{
	SYSCALL_1(SYS_sched_yield,get_current_task());
}


unsigned long open(char *pathname, int flags)
{
	return SYSCALL_2(SYS_open, pathname, flags);
}

unsigned long close(int fd)
{
	return SYSCALL_1(SYS_close, fd);
}

long read(int fd, void *buf, unsigned int count)
{
	return SYSCALL_3(SYS_read, fd, buf, count);
}

long write(int fd, void *buf, unsigned int count)
{
	return SYSCALL_3(SYS_write, fd, buf, count);
}

//设置文件指针位置
long lseek(int fd, long offset, int whereat)
{
	return SYSCALL_3(SYS_lseek, fd, offset, whereat);
}

long exec(char *path, char *argv, char *envp)
{
	ELFExec_t *exec = NULL;
	unsigned long filesize = 0;
	void * stack = NULL;
	unsigned long point = 0;
	unsigned long pos = 0;
	exec = SYSCALL_3(SYS_execve, path, argv, envp);
	if(exec == -1)
	{
		return -1;
	}

	entry_t *entry = (entry_t *)(elfseg.entry);
	jumpTo(entry);
	print("exec finish!\n");
	free(elfseg.segment[0], elfseg.needpage);
	return 0;
}

int getdents(int fd,struct dirent *buf,long count)
{
	return SYSCALL_3(SYS_getdents64, fd, (void *)buf, count);
}

char * getcwd(char *buf, unsigned int size)
{
	return SYSCALL_2(SYS_getcwd, buf, size);
}

long chdir(char * path)
{
	return SYSCALL_1(SYS_chdir, path);
}