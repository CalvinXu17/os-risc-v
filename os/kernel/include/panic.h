#ifndef PANIC_H
#define PANIC_H

#include "printk.h"
void panic(const char *msg);

#define assert(x) \
{ \
    if(!(x)) { printk("assert error: "); panic(#x); }\
}

#endif