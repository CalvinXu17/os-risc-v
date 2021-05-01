#ifndef KMALLOC_H
#define KMALLOC_H

#include "type.h"

void *kmalloc(uint64 size);
void kfree(void *addr);
uint64 ksize(const void *addr);

#endif