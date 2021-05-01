#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "type.h"
#include "cpu.h"

typedef struct _spinlock
{
    char *name;
    uint64 islocked;
    cpu *owner;
}spinlock;

void init_spinlock(spinlock *lock, char *name);
void lock(spinlock *lock);
void unlock(spinlock *lock);
int ishold(spinlock *lock);

#endif