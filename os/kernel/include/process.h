#ifndef PROCESS_H
#define PROCESS_H

#include "type.h"
#include "intr.h"
#include "list.h"

#define list2proc(l)   GET_STRUCT_ENTRY(l, Process, child_list)
#define PROC_N  100

#define PROC_DEAD   0
#define PROC_RUN    1
#define PROC_READY  2
#define PROC_WAIT   3

#define T_SLICE     100 // 默认时间片为100毫秒

typedef struct _proc_context
{
    uint64 ra;
    uint64 sp;
    uint64 gp;
    uint64 tp;
    uint64 t0;
    uint64 t1;
    uint64 t2;
    uint64 s0; // fp
    uint64 s1;
    uint64 a0;
    uint64 a1;
    uint64 a2;
    uint64 a3;
    uint64 a4;
    uint64 a5;
    uint64 a6;
    uint64 a7;
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
    uint64 t3;
    uint64 t4;
    uint64 t5;
    uint64 t6;
} proc_context;

typedef struct _Process
{
    uint64 k_sp;
    uint64 *k_stack;
    uint64 *pg_t;
    proc_context context;
    
    int32 pid;
    int32 status;

    int64 t_wait;
    int64 t_slice;

    list *parent;
    list child_list;
} Process;

int32 get_pid(void);
void free_pid(int32 pid);

#endif