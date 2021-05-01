#ifndef SLOB_H
#define SLOB_H

#include "type.h"

typedef struct _small_block
{
    int64 unit_n;
    struct _small_block *next;
} small_block; // 用于分配小于1页面的小内存

typedef struct _big_block
{
    int64 page_n;
    void *pages;
    struct _big_block *next;
} big_block; // 用于分配大内存

#define UNIT_SIZE (sizeof(small_block))
#define get_unit_n(size) ((size + UNIT_SIZE - 1) / UNIT_SIZE) // 向上取整

extern small_block free_small_blocks;
extern small_block *p_free_small_blocks;
extern big_block *p_big_blocks;

void* slob_get_pages(uint64 page_n);
void slob_free_pages(uint64 va, uint64 page_n);
void* small_alloc(uint64 size);
void small_free(void *block, uint64 size);

#endif