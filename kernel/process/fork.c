#include <process.h>
#include <scheduler.h>
#include <csr.h>
#include <trap.h>
#include <memory.h>
#include "../../lib/printk.h"
#include "../../lib/lib.h"

#define __init_task_data __attribute__((__section__(".data.init_task")))

//kernel_main is init process
union task_union init_task_union __init_task_data = {TASK_INIT(task)};
struct ready_queue g_queue;

int total_forks = 0;
PCB *g_task[TOTAL_TASK] = {&init_task_union.task,};
PCB *current = &init_task_union.task;

int find_pid()
{
    for(int i = 0; i < TOTAL_TASK; i++)
    {
        if(g_task[i] == NULL)
        {
            return i;
        }
    }
    return -1;
}

struct pt_regs *get_pt_reg(PCB *pcb)
{
    struct pt_regs * where = (unsigned long)pcb + THREAD_SIZE - sizeof(struct pt_regs);
    return where;
}


//set context of sub thread
static int copy_thread(unsigned long clone_flags, PCB *pcb,
		unsigned long callback_fun, unsigned long arg)
{
    struct pt_regs *pt_reg = get_pt_reg(pcb);
    if(pt_reg == NULL)
    {
        return -1;
    }
    memset(pt_reg, 0, sizeof(struct pt_regs));
	memset(&pcb->context, 0, sizeof(cpu_context));
    if(clone_flags & KERNEL_THREAD)
    {
        const register unsigned long gp __asm__ ("gp");
        pt_reg->gp = gp;
        pt_reg->sstatus = SSTATUS_SPP | SSTATUS_SPIE;
        pcb->context.reg[0] = callback_fun;
        pcb->context.reg[1] = arg;
        pcb->context.ra = (unsigned long)ret_from_kernel_thread;
    }
    else
    {
        *pt_reg = *get_pt_reg(get_current_task());
		pt_reg->a0 = 0;     //sub process return 0 from fork
		if (callback_fun)
			pt_reg->sp = callback_fun;
		pcb->context.ra = (unsigned long)ret_from_fork;
    }
    pcb->context.sp = (unsigned long)pt_reg;
    return 0;
}

//create process
int do_fork(unsigned long clone_flags, unsigned long callback_fun, unsigned long arg)
{
    int pid = find_pid();
    if(pid == -1)
    {
        printk("can't get pid!\n");
        return -1;
    }
    PCB *pcb = (PCB*)page_alloc();
    if(pcb == 1)
    {
        printk("alloc page for pcb fail!\n");
        return -1;
    }
    if(copy_thread(clone_flags, pcb, callback_fun, arg) == -1)
    {
        return -1;
    }
    pcb->task_state = TASK_RUNNING;
    pcb->priority = 2;
    pcb->count = DEFAULT_SLICE + pcb->priority;
	get_current_task()->count >>= 1;
	pcb->need_schedule = 0;
	pcb->scramble = 1;
	total_forks++;
    pcb->pid = pid;
    g_task[pid] = pcb;
    simple_sched_class.enqueue_task(&g_queue, pcb);
    return pid;
}

PCB* switch_to(PCB *prev,PCB *next)
{
    if (prev == next)
    {
        printk("process is the same!\n");
        return NULL;
    }
	return cpu_switch_to(prev, next);
}

void start_user_thread(struct pt_regs *regs, unsigned long sepc, unsigned long sp)
{  
    memset((void *)regs, 0, sizeof(struct pt_regs));
    regs->sepc = sepc;
    regs->sp = sp;
    regs->sstatus = read_csr(sstatus) & ~SSTATUS_SPP;
    printk("sstatus 0x%lx sp 0x%lx  pc 0x%lx\n", regs->sstatus, regs->sp, regs->sepc);
}

int create_user_place(unsigned long sepc)
{
    struct pt_regs *regs = get_pt_reg(get_current_task());
	unsigned long stack = page_alloc();
    if(stack == 1)
    {
        printk("create user place fail! no enough pages for stack!\n");
        return 1;
    }
    memset((void *)stack, 0, PAGE_SIZE);
    start_user_thread(regs, sepc, stack + PAGE_SIZE);
    return 0;
}

PCB* sleep(int pid)
{
    PCB *pcb = g_task[pid];
    pcb->task_state = TASK_INTERRUPTIBLE;
    schedule();
    return pcb; 
}

void wakeup(int pid)
{
    g_task[pid]->task_state = TASK_RUNNING;
}