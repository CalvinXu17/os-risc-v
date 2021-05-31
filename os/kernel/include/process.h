#ifndef PROCESS_H
#define PROCESS_H

#include "type.h"
#include "list.h"
#include "spinlock.h"
#include "sem.h"
#include "intr.h"
#include "vfs.h"

#define PROC_N  100

#define PROC_DEAD   0
#define PROC_RUN    1
#define PROC_READY  2
#define PROC_WAIT   3
#define PROC_SLEEP  4

#define T_SLICE     50 // 默认时间片大小

#define PROC_FILE_MAX 10

struct proc_context
{
    uint64 ra;
    uint64 sp;
  // callee-saved
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};

struct Process
{
    uint64 *k_stack;
    uint64 *pg_t;
    struct trap_context tcontext;
    struct proc_context pcontext;
    
    int32 pid;
    int32 status;

    int64 t_wait;
    int64 t_slice;

    char code;
    struct semephore signal;

    struct Process *parent;
    list status_list_node;

    list child_list_node;
    list child_list;
    
    void *data;
    spinlock lock;

    uint64 start_time;
    uint64 end_time;
    uint64 utime_start;
    uint64 stime_start;
    uint64 utime;
    uint64 stime;

    vfs_dir_t *cwd;
    list ufiles_list;
    
    void *minbrk;
    void *brk;
};

#define status_list_node2proc(l)    GET_STRUCT_ENTRY(l, struct Process, status_list_node)
#define child_list2proc(l)          GET_STRUCT_ENTRY(l, struct Process, child_list)
#define child_list_node2proc(l)     GET_STRUCT_ENTRY(l, struct Process, child_list_node)

#define set_user_mode(p) ({ \
    uint64 sstatus = p->sstatus; \
    sstatus &= (~SPP); \
    sstatus |= SPIE; \
    p->sstatus = sstatus; \
})

void proc_init(void);
int32 get_pid(void);
void free_pid(int32 pid);
void user_init(int hartid);

struct Process* new_proc(void);
void free_process_mem(struct Process *proc);
void free_process_struct(struct Process *proc);
void* build_pgt(uint64 *pg0_t, uint64 page_begin_va, uint64 page_cnt);
struct Process* create_proc_by_elf(char *absolute_path);
void free_ufile_list(struct Process *p);
void* userva2kernelva(uint64 *pgtva, uint64 uaddr);

#endif