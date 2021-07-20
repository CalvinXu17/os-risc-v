#ifndef _VMM_H
#define _VMM_H

#include "type.h"
#include "page.h"
#include "list.h"
#include "vfs.h"

typedef enum {
    VMA_PROG = 1U,
    VMA_DATA = 2U,
    VMA_BRK = 4U,
    VMA_MMAP = 8U,
    VMA_CLONE = 16U
} type_vma_t;

struct vma
{
    uint64 va;      // 虚拟地址，按页对齐，vma按页分配管理内存
   // uint64 pa;      // 物理地址
    uint64 size;
    uint prot;   // 保护位
    uint flag;
    type_vma_t type;
    uint vmoff;
    uint64 fsize;
    uint64 foff;
    vfs_file_t *file;
    list vma_list_node;
};

#define vma_list_node2vma(l)    GET_STRUCT_ENTRY(l, struct vma, vma_list_node)

#define PAGEDOWN(size)  (((size)) & ~(PAGE_SIZE-1))
#define PAGEUP(size)    (((size)+PAGE_SIZE-1) & ~(PAGE_SIZE-1))

#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4
#define PROT_NONE   0x0

#define MAP_FILE    0
#define MAP_SHARED  0x01
#define MAP_PRIVATE 0X02

#define STACK_TOP   (0x4000000000 - PAGE_SIZE * 16) //保留16页防止栈上溢
#define STACK_LOW   0x3000000000

#define MAP_TOP     STACK_LOW
#define MAP_LOW     0x2000000000

struct Process;
struct vma* new_vma(void);
void free_vma_mem(uint64 *pgt, struct vma *vma);
struct vma* find_vma(struct Process *proc, uint64 va);
int add_vma(struct Process *proc, struct vma *vma);
uint64 prot2pte(uint64 prot);
void copy_kpgt(uint64 *pg_t);

void *vm_alloc_pages(uint64 pagecnt);
void vm_free_pages(void *kva, uint64 pagecnt);

int mappages(uint64 *pg0_t, uint64 va, uint64 pa, uint64 page_cnt, uint64 pte_flags);
int unmappages(uint64 *pg0_t, uint64 va, uint64 page_cnt);
void free_pagetable(uint64 *pg_t);
uint64* copy_pagetable(uint64 *pg_to, uint64 *pg_from);
void switch2kpgt(void);
uint64 va2pa(uint64 *pgt_kva, uint64 va);
uint64 va2pte(uint64 *pgt_kva, uint64 va);
int isforkmem(uint64 *pgt_kva, uint64 va);
void* userva2kernelva(uint64 *pgt_kva, uint64 userva);
int copy2user(uint64 *pgt_user, uchar *va_to_user, uchar *va_from, uint64 len);
struct vma* find_map_area(struct Process *proc, uint64 start, uint64 len);

void do_exec_fault(struct Process *proc, uint64 badaddr);
void do_page_fault(struct Process *proc, uint64 badaddr);

#endif