#ifndef SEM_H
#define SEM_H

#include "spinlock.h"
#include "cpu.h"

struct semephore
{
    int value;
    char *name;
    spinlock lck;
};

void init_semephonre(struct semephore *s, char *name, int value);
void P(struct semephore *s);
void V(struct semephore *s);

#endif