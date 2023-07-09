#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H


typedef unsigned long _u64;

#define SYSCALL_0(n)													internal_syscall(n, 0, 0, 0, 0, 0, 0)
#define SYSCALL_1(n, arg0)												internal_syscall(n, arg0, 0, 0, 0, 0, 0)
#define SYSCALL_2(n, arg0, arg1)										internal_syscall(n, arg0, arg1, 0, 0, 0, 0)
#define SYSCALL_3(n, arg0, arg1, arg2)									internal_syscall(n, arg0, arg1, arg2, 0, 0, 0)
#define SYSCALL_4(n, arg0, arg1, arg2, arg3)							internal_syscall(n, arg0, arg1, arg2, arg3, 0, 0)
#define SYSCALL_5(n, arg0, arg1, arg2, arg3, arg4)						internal_syscall(n, arg0, arg1, arg2, arg3, arg4, 0)
#define SYSCALL_6(n, arg0, arg1, arg2, arg3, arg4, arg5)				internal_syscall(n, arg0, arg1, arg2, arg3, arg4, arg5)

_u64 internal_syscall(long n, _u64 _a0, _u64 _a1, _u64 _a2, _u64
		_a3, _u64 _a4, _u64 _a5);
unsigned long sleep_proc(int time);
unsigned long print(const char *fmt,...);
extern int __clone(int (*fn)(void *arg), void *child_stack,
		int flags, void *arg);
_u64 clone(int (*thread_callback)(void *arg), void *child_stack, int flag, void *arg);
_u64 malloc();
unsigned long open(char *pathname, int flags);
unsigned long close(int fd);
long read(int fd, void *buf, unsigned int count);
long write(int fd, void *buf, unsigned int count);
long lseek(int fd, long offset, int whereat);
long exec(char *path, char *argv, char *envp);
#endif