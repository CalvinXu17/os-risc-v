#include "syscall.h"
#include "printk.h"
#include "process.h"

void* syscall_table[SYS_NR];

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

void sys_test(struct trap_context *context)
{
    printk("hartid: %d, pid: %d, a0: %d:%c\n", gethartid(), getcpu()->cur_proc->pid, context->a0, context->a0);
}

void sys_fork(struct trap_context *context)
{

}

void syscall_init(void)
{
    int i;
    for(i=0; i<SYS_NR; i++)
    {
        syscall_table[i] = unknown_syscall;
    }
    syscall_table[SYS_test] = sys_test;
    syscall_table[SYS_fork] = sys_fork;
}