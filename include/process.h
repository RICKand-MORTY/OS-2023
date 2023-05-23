#ifndef _PROCESS_H
#define _PROCESS_H


#include "scheduler.h"

//store cpu context 
typedef struct _cpu_context
{
    unsigned long ra;           //return address
    unsigned long sp;           //point to stack top
    unsigned long reg[12];      //General-Purpose Registers  
}cpu_context;

typedef struct task_struct
{
    cpu_context context; 
    volatile long task_state;    
    unsigned long task_flags;
    long count;         //time slice
    long pid;
    long priority;
}PCB;

//use to initial init process(kernel_main)
#define TASK_INIT(task)                 \
{                                       \
    .task_state = TASK_RUNNING,         \
    .priority   = 1,                    \
    .task_flags = KERNEL_THREAD,        \
    .pid        = 0,                    \
}

//kernel stack struct
union task_union {
	PCB task;
	unsigned long stack[THREAD_SIZE/sizeof(long)];
}__attribute__((aligned(sizeof(long))));

PCB *get_current_task(void)
{
    register PCB *tp __asm__("tp");
    return tp;
}

extern PCB *g_task[TOTAL_TASK];

int find_pid();
struct pt_regs *get_pt_reg(PCB *pcb);
int do_fork(unsigned long clone_flags, unsigned long callback_fun, unsigned long arg);
void switch_to(PCB *next);
extern void ret_from_kernel_thread(void);
extern struct task_struct *cpu_switch_to(PCB *prev, PCB *next);
#endif