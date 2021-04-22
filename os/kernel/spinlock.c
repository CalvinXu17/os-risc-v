#include "spinlock.h"
#include "intr.h"

void init_spinlock(spinlock *lock, char *name)
{
    lock->islocked=0;
    lock->owner=0;
    lock->name=name;
}

void lock(spinlock *lock)
{
    uint64 old_intr = is_intr_open();
    intr_close();
    cpu *cur_cpu = getcpu();
    if(cur_cpu->locks_n==0)
    {
        cur_cpu->old_intr = old_intr;
    }
    cur_cpu->locks_n++;

    // lock
    while(__sync_lock_test_and_set(&(lock->islocked),1)!=0) {}
    __sync_synchronize();

    lock->owner = getcpu();
}

void unlock(spinlock *lock)
{
    // unlock
    lock->owner = 0;

    __sync_synchronize();
    __sync_lock_release(&(lock->islocked));


    cpu *cur_cpu = getcpu();
    cur_cpu->locks_n--;
    if(!cur_cpu->locks_n && cur_cpu->old_intr)
        intr_open();
}