#ifndef _SPINLOCK_H
#define _SPINLOCK_H


typedef struct _spinlock
{
    volatile unsigned long lock;
}spinlock, *spinlock_P;

void spin_init(spinlock_P lock);
void spin_lock(spinlock_P lock);
void spin_unlock(spinlock_P lock);
#endif