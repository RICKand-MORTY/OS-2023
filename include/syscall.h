#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <trap.h>

#define TOTAL_SYSCALLS      500


typedef long (*syscall_fun)(struct pt_regs *);

extern const syscall_fun syscall_table[TOTAL_SYSCALLS];
void syscall_handler(struct pt_regs *regs);

#endif