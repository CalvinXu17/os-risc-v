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
}

int ishold(spinlock *lock)
{
    return lock->islocked && lock->owner==getcpu();
}