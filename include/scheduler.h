#ifndef _SHEDULER_H
#define _SHEDULER_H


#include "page_table.h"

//offset in PCB
#define CONTEXT_OFFSET              0 
#define SCRAMBLE_OFFSET             172
#define NEED_SCHEDULE_OFFSET        164
#define KERNEL_SP_OFFSET            144
#define USER_SP_OFFSET              152

#define TOTAL_TASK                  128
#define THREAD_SIZE                 PAGE_SIZE

//task_state
#define TASK_RUNNING                0
#define TASK_INTERRUPTIBLE          1
#define TASK_UNINTERRUPTIBLE        2
#define TASK_ZOMBIE                 3
#define TASK_STOPPED                4

//task flag
#define PROCESS                     0
#define THREAD                      1
#define KERNEL_PROCESS              2
#define KERNEL_THREAD               3

//default time slice
#define DEFAULT_SLICE               10

#endif