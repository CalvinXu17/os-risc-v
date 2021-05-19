#include "panic.h"

void panic(const char *msg)
{
    printk("\nPANIC: %s", msg);
    while(1) {}
}