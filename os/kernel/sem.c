#include "sem.h"
#include "sched.h"

void init_semephonre(struct semephore *s, char *name, int value)
{
    init_spinlock(&(s->lck), "semephore");
    s->name = name;
    s->value = value;
}

void P(struct semephore *s)
{
    lock(&(s->lck));
    s->value--;
    if(s->value < 0)
    {
        unlock(&(s->lck));
        sleep(s);
    } else unlock(&(s->lck));
}

void V(struct semephore *s)
{
    lock(&(s->lck));

    s->value++;
    if(s->value <= 0)
    {
        unlock(&(s->lck));
        wakeup(s);
    } else unlock(&(s->lck));
}
