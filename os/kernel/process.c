#include "process.h"
#include "kmalloc.h"
#include "page.h"

uchar pids[PROC_N] = {0};
list ready;
list wait;
list dead;


int32 get_pid(void)
{
    int32 i;
    for(i = 1; i < PROC_N; i++)
    {
        if(pids[i] == 0)
        {
            pids[i] = 1;
            return i;
        }
    }
    return -1;
}

void free_pid(int32 pid)
{
    if(pid >= 1 && pid < PROC_N)
    {
        pids[pid] = 0;
    }
}

static void init_proc_list(void)
{
    init_list(&ready);
    init_list(&wait);
    init_list(&dead);
}

Process* new_proc(void)
{
    int32 pid = get_pid();
    Process *proc = (Process*)kmalloc(sizeof(Process));
    proc->pid = pid;
    proc->status = PROC_READY;
    proc->t_wait = 0;
    proc->t_slice = T_SLICE;
    init_list(&(proc->child_list));
    proc->k_stack = (uint64*)kmalloc(PAGE_SIZE);
    proc->k_sp = (uint64)proc->k_stack + PAGE_SIZE;
    return proc;
}

void free_proc(Process *proc)
{
    free_pid(proc->pid);
    kfree(proc->k_stack);
    kfree(proc);
}
