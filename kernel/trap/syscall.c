#include <syscall.h>
#include <process.h>
#include "../../usr/syscall_num.h"
#include <memory.h>

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
};
