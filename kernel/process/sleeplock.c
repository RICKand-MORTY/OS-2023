#include "sleeplock.h"
#include <process.h>


void init_sleeplock(sleeplock_P slock, char* name)
{
    spin_init(&slock->splock);
    slock->name = name;
    slock->pid = 0;
    slock->locked = 0;
}

void accquire_sleeplock(sleeplock_P slock)
{
    spin_lock(&slock->splock);
    while (slock->locked)
    {
        sleep(get_current_task()->pid);
    }
    slock->locked = 1;
    slock->pid = get_current_task()->pid;
    spin_unlock(&slock->splock);
}

void release_sleeplock(sleeplock_P slock)
{
    spin_lock(&slock->splock);
    slock->locked = 0;
    wakeup(slock->pid);
    slock->pid = 0;
    spin_unlock(&slock->splock);
}