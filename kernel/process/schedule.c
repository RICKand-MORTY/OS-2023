#include <process.h>
#include "../../lib/printk.h"
#include <csr.h>

void dequeue_task_simple(struct ready_queue *rq, PCB* task)
{
    rq->task_num--;
    list_delete_node(task->task_list.prev, task->task_list.next);
}

void enqueue_task_simple(struct ready_queue *rq, PCB* task)
{
    rq->task_num++;
    list_add(&rq->queue_head, &task->task_list, ADD_TAIL);
}

void task_tick_simple(struct ready_queue *rq, PCB *task)
{
    task->count--;
    if(task->count <= 0)
    {
        task->count = 0;
        //printk("pid:%d need to schedule!\n",task->pid);
        task->need_schedule = 1;
    }
}

void reset_score(struct ready_queue *rq)
{
    PLinkList head = &rq->queue_head;
    PLinkList node = head->next;
    PCB* task = NULL;
    while(node != head)
    {
        task = container_of(node, PCB, task_list);
        task->count = DEFAULT_SLICE + task->priority;
        node = node->next;
    }
}

PCB* pick_next_task_simple(struct ready_queue *rq, PCB* prev)
{
    PLinkList head = &rq->queue_head;
    PLinkList node = head->next;
    PCB* task = NULL;
    PCB* next = NULL;
    int c = 0;
    repeat:
    node = head->next;
    c = -1000;
    while(node != head)
    {
        task = container_of(node, PCB, task_list);
        if(task->count > c)
        {
            c = task->count;
            next = task;
        }
        node =  node -> next;
    }
    if(c <= 0)
    {
        reset_score(rq);
        goto repeat;
    }
    //printk("%s: pick next thread (pid %d)\n", __func__, next->pid);
	return next;
}

const struct sched_class simple_sched_class = {
	.next = NULL,
    .task_fork = NULL,
	.dequeue_task = dequeue_task_simple,
	.enqueue_task = enqueue_task_simple,
	.task_tick = task_tick_simple,
	.pick_next_task = pick_next_task_simple,
};

//can't scramble
void preempt_disable(void)
{
    PCB *task = get_current_task();
	task->scramble = 0;
}

void preempt_enable(void)
{
    PCB *task = get_current_task();
	task->scramble = 1;
}

//check if schedule happen at interrupt context
void schedule_debug(PCB *p)
{
    PCB *task = get_current_task();
	if(task->scramble == 1)
		printk("BUG: scheduling while atomic: %d, 0x%x\n",
				p->pid, p->scramble);
}

//deal with work after schedule
void schedule_tail(struct task_struct *prev)
{
	irq_enable();
}

void __schedule(void)
{
    PCB *prev, *next, *last;
    struct ready_queue *rq = &g_queue;
    prev = get_current_task();
    schedule_debug(prev);
    irq_disable();
    if(prev->task_state != TASK_RUNNING)
    {
        //preemptive scheduling happened
        simple_sched_class.dequeue_task(rq, prev);
    }
    next = simple_sched_class.pick_next_task(rq, prev);
    prev->need_schedule = 0;
    if(next != prev)
    {
        last = switch_to(prev, next);       //last = prev
        rq->switch_times++;
        rq->currrent = get_current_task();
        schedule_tail(last);
    }
}

void schedule(void)
{
	//to avoid nested scheduling preemption
	preempt_disable();
	__schedule();
	preempt_enable();
}

void preempt_schedule(void)
{
    if(get_current_task()->scramble == 0)
    {
        printk("BUG: scramble is 0, can't preempt");
    }
    preempt_disable();
    irq_enable();
    __schedule();
    irq_disable();
    preempt_enable();
}

void sched_init(void)
{
	struct ready_queue *rq = &g_queue;
	list_init(&rq->queue_head);
	rq->switch_times = 0;
	rq->task_num = 0;
	rq->currrent = NULL;
}