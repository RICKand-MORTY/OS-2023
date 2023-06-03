#ifndef _SLEEPLOCK_H
#define _SLEEPLOCK_H


#include "spinlock.h"

typedef struct _sleeplock
{
    int locked;         //whether is lock or not, locked = 1 unlocked = 0
    spinlock splock;
    char *name;
    int pid;            // Process holding lock

}sleeplock,*sleeplock_P;

void init_sleeplock(sleeplock_P slock, char* name);
void acquire_sleeplock(sleeplock_P slock);
void release_sleeplock(sleeplock_P slock);
int holding_sleeplock(sleeplock_P slock);
#endif 