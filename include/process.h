#ifndef _PROCESS_H
#define _PROCESS_H


#include "scheduler.h"
#include "LinkList.h"

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
    volatile long task_state;       //type of state
    unsigned long task_flags;       //type of process
    struct list_head task_list;
    unsigned long kernel_sp;        //save S mode sp
	unsigned long user_sp;          //save U mode sp
    int count;                      //time slice 
    int need_schedule;              //if need to scheduled
    int pid;
    int scramble;                   //if is scramble           
    int priority;
    struct task_struct *next_task;
    struct task_struct *prev_task;
}PCB;

struct ready_queue
{
    struct list_head queue_head;
    unsigned int task_num;          //number of tasks
    unsigned long switch_times;     //process switches times
    PCB *currrent;                  //point to current process
};


//kernel stack struct
union task_union {
	PCB task;
	unsigned long stack[THREAD_SIZE/sizeof(long)];
}__attribute__((aligned(sizeof(long))));

struct sched_class {
	const struct sched_class *next;     //point to next sched_class

	void (*task_fork)(struct task_struct *p);       //initial process method
	void (*enqueue_task)(struct ready_queue *rq, struct task_struct *p);  //add in ready queue
	void (*dequeue_task)(struct ready_queue *rq, struct task_struct *p);  //dequeue from ready queue
	void (*task_tick)(struct ready_queue *rq, struct task_struct *p);     //time interrupt
	struct task_struct * (*pick_next_task)(struct ready_queue *rq,        //choose next process
			struct task_struct *prev);
};

//use to initial init process(kernel_main)
#define TASK_INIT(task)                 \
{                                       \
    .task_state = TASK_RUNNING,         \
    .priority   = 1,                    \
    .task_flags = KERNEL_THREAD,        \
    .pid        = 0,                    \
}

/*
use to get address of struct according to member
ptr: address of member
type: type of struct
member: member in struct
return address of struct
*/
#define container_of(ptr,type,member)                                                           \
({                                                                                              \
    const typeof(((type *)0)->member) * p = (const typeof( ((type *)0)->member ) *)(ptr);       \
    (type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));                         \
})

PCB *get_current_task(void)
{
    register PCB *tp __asm__("tp");
    return tp;
}

void delay(int time)
{
    for(int i=0;i<time;i++);
}

extern const struct sched_class simple_sched_class;
extern PCB *g_task[TOTAL_TASK];
extern struct ready_queue g_queue;
extern void ret_from_kernel_thread(void);
extern struct task_struct *cpu_switch_to(PCB *prev, PCB *next);

int find_pid();
struct pt_regs *get_pt_reg(PCB *pcb);
int do_fork(unsigned long clone_flags, unsigned long callback_fun, unsigned long arg);
PCB* switch_to(PCB *prev,PCB *next);
void sched_init(void);
int create_user_place(unsigned long sepc);
void start_user_thread(struct pt_regs *regs, unsigned long sepc, unsigned long sp);
extern void ret_from_fork(void);
void sleep(int pid);
void wakeup(int pid);
#endif