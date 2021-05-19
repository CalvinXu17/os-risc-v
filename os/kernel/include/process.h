#ifndef PROCESS_H
#define PROCESS_H

#include "type.h"
#include "list.h"
#include "spinlock.h"
#include "intr.h"

#define PROC_N  100

#define PROC_DEAD   0
#define PROC_RUN    1
#define PROC_READY  2
#define PROC_WAIT   3
#define PROC_SLEEP  4

#define T_SLICE     50 // 默认时间片大小

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

    list *parent;
    list child_list;
    list status_list;
    void *data;
    spinlock lock;
};

#define list2proc(l)   GET_STRUCT_ENTRY(l, struct Process, status_list)
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

#endif