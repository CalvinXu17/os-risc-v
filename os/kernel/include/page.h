#ifndef PAGE_H
#define PAGE_H

#include "list.h"
#include "type.h"
#include "mm.h"

#define PAGE_SIZE    0x1000
#define PAGE_NUM     (PM_SIZE/PAGE_SIZE)

#define PAGE_RESERVE 0
#define PAGE_USED    1
#define PAGE_FREE    2

typedef struct _Page
{
    uint32 flag;
    int ref_cnt;
    int free_page_num;
    list page_list;
}Page;

typedef struct _Page_Pool
{
    list pages_list;
    int page_num;
}Page_Pool;

extern Page pages_map[PAGE_NUM];
extern Page_Pool free_page_pool;

#define list2page(l)   GET_STRUCT_ENTRY(l, Page, page_list)

uint64 align4k(uint64 s);
void page_init(void);
Page* alloc_pages(uint64 n);
void free_pages(Page *page, uint64 n);
static inline Page* get_page_by_pm_addr(uint64 addr) __attribute__((always_inline));
static inline uint64 get_pm_addr_by_page(Page *page) __attribute__((always_inline));
static inline Page* get_page_by_vm_addr(uint64 addr) __attribute__((always_inline));
static inline uint64 get_vm_addr_by_page(Page *page) __attribute__((always_inline));

// 根据物理地址获取Page
static inline Page* get_page_by_pm_addr(uint64 addr)
{
    return &pages_map[(addr >> 12)-(PM_START>>12)];
}

// 根据Page获取物理地址
static inline uint64 get_pm_addr_by_page(Page *page)
{
    uint64 offset = page - pages_map;
    uint64 addr = PM_START + 0x1000 * offset;
    return addr;
}

// 根据虚拟地址获取Page
static inline Page* get_page_by_vm_addr(uint64 addr)
{
    return &pages_map[(addr >> 12)-((VM_START)>>12)];
}

// 根据Page获取内核虚拟地址
static inline uint64 get_vm_addr_by_page(Page *page)
{
    uint64 offset = page - pages_map;
    return VM_START + 0x1000 * offset;
}

extern uint64 k_pg_t[PAGE_SIZE];

#endif