#include "syscall.h"
#include "printk.h"
#include "process.h"
#include "sched.h"
#include "console.h"

void* syscall_table[SYS_NR];

void sys_test(struct trap_context *context)
{
    if(cpus[0].cur_proc)
    {
        printk("hart0 pid %d\n", cpus[0].cur_proc->pid);
    }
    if(cpus[1].cur_proc)
    {
        printk("hart1 pid %d\n", cpus[1].cur_proc->pid);
    }

    if(!is_empty_list(&sleep_list))
    {
        printk("sleep pid: %d\n", list2proc(sleep_list.next)->pid);
    }
    
    printk("hartid: %d, pid: %d, a0: %d:%c\n", gethartid(), getcpu()->cur_proc->pid, context->a0, context->a0);
}

int sys_read(int fd, char *buf, int len)
{
    if(fd==0)
    {
        return console_read(buf, len);
    }
}

int sys_write(int fd, const char *buf, int len)
{
    if(fd==1)
    {
        return console_write(buf, len);
    }
}

int sys_getpid(void)
{
    return getcpu()->cur_proc->pid;
}

void sys_sched_yield(void)
{
    struct Process *proc = getcpu()->cur_proc;
    lock(&proc->lock);
    proc->status = PROC_READY;
    move_switch2sched(proc, &ready_list);
}

int sys_waitpid(int pid, int *code, int options)
{
    return 0;
}

int sys_sleep(struct TimeVal *timein, struct TimeVal *timeout)
{
    struct Process *proc = getcpu()->cur_proc;
    lock(&proc->lock);
    proc->t_wait = timein->sec * 1000;
    proc->status = PROC_WAIT;
    move_switch2sched(proc, &wait_list);
    return 0;
}

void syscall_handler(struct trap_context *context)
{
    int (*syscall)(struct trap_context *);
    syscall = syscall_table[context->a7];
    context->a0 = syscall(context);
}

void unknown_syscall(struct trap_context *context)
{
    #ifdef _DEBUG
    printk("unknown syscall %d!\n", context->a7);
    #endif
}

void do_sys_read(struct trap_context *context)
{
    context->a0 = sys_read(context->a0, context->a1, context->a2);
}

void do_sys_write(struct trap_context *context)
{
    context->a0 = sys_write(context->a0, context->a1, context->a2);
}

void do_sys_getpid(struct trap_context *context)
{
    context->a0 = sys_getpid();
}

void do_sys_sched_yield(struct trap_context *context)
{
    sys_sched_yield();
}

void do_sys_wait(struct trap_context *context)
{
    context->a0 = sys_waitpid(context->a0, context->a1, context->a2);
}

int do_sys_sleep(struct trap_context *context)
{
    context->a0 = sys_sleep(context->a0, context->a1);
}
void syscall_init(void)
{
    int i;
    for(i=0; i<SYS_NR; i++)
    {
        syscall_table[i] = unknown_syscall;
    }
    syscall_table[SYS_test] = sys_test;
    syscall_table[SYS_read] = do_sys_read;
    syscall_table[SYS_write] = do_sys_write;
    syscall_table[SYS_getpid] = do_sys_getpid;
    syscall_table[SYS_sched_yield] = do_sys_sched_yield;
    syscall_table[SYS_wait4] = do_sys_wait;
    syscall_table[SYS_nanosleep] = do_sys_sleep; 
}