#include "syscall.h"
#include "printk.h"
#include "process.h"
#include "sched.h"
#include "console.h"
#include "string.h"
#include "page.h"
#include "kmalloc.h"
#include "timer.h"

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
        printk("sleep pid: %d\n", status_list_node2proc(sleep_list.next)->pid);
    }
    
    printk("hartid: %d, pid: %d, a0: %d:%c\n", gethartid(), getcpu()->cur_proc->pid, context->a0, context->a0);
}

int sys_read(int fd, char *buf, int len)
{
    if(fd==1) return -1;
    else if(fd==0)
    {
        return console_read(buf, len);
    }
}

int sys_write(int fd, const char *buf, int len)
{
    if(!fd) return -1;
    else if(fd==1)
    {
        return console_write(buf, len);
    }
}

int sys_clone(void)
{
    struct Process *parent = getcpu()->cur_proc;
    struct Process *child = new_proc();
    child->parent = parent;
    
    memcpy(&child->tcontext, &parent->tcontext, sizeof(struct trap_context));
    child->tcontext.k_sp = (uint64)(child->k_stack) + PAGE_SIZE;
    child->tcontext.sepc += 4; // 子进程pc+4执行ecall下一条指令
    child->pcontext.ra = (uint64)trap_ret; // 子进程从trap_ret开始执行从内核态转换为用户态

    int i, j, k;

    uint64 *p_pg0, *p_pg1, *p_pg2, *p_pg3;
    uint64 *c_pg0, *c_pg1, *c_pg2, *c_pg3;
    
    p_pg0 = parent->pg_t;
    c_pg0 = kmalloc(PAGE_SIZE);

    for(i = 0; i < 512; i++)
    {
        if(p_pg0[i] & PTE_V)
        {
            p_pg1 = PTE2PA(p_pg0[i]) + PV_OFFSET;
            c_pg1 = kmalloc(PAGE_SIZE);
            if(!(p_pg0[i] & PTE_R) && !(p_pg0[i] & PTE_W) && !(p_pg0[i] & PTE_X)) // 指向下一级
            {
                c_pg0[i] = PA2PTE((uint64)c_pg1 - PV_OFFSET) | PTE_V;
                for(j = 0; j < 512; j++)
                {
                    if(p_pg1[j] & PTE_V)
                    {
                        p_pg2 = PTE2PA(p_pg1[j]) + PV_OFFSET;
                        c_pg2 = kmalloc(PAGE_SIZE);
                        if(!(p_pg1[j] & PTE_R) && !(p_pg1[j] & PTE_W) && !(p_pg1[j] & PTE_X)) // 指向下一级
                        {
                            c_pg1[j] = PA2PTE((uint64)c_pg2 - PV_OFFSET) | PTE_V;
                            for(k = 0; k < 512; k++)
                            {
                                if(p_pg2[k] & PTE_V)
                                {
                                    p_pg3 = PTE2PA(p_pg2[k]) + PV_OFFSET;
                                    c_pg3 = kmalloc(PAGE_SIZE);
                                    memcpy(c_pg3, p_pg3, PAGE_SIZE);
                                    c_pg2[k] = PA2PTE((uint64)c_pg3 - PV_OFFSET) | PTE_V | PTE_R | PTE_W  | PTE_X | PTE_U;
                                } else c_pg2[k] = 0;
                            }
                        } else c_pg1[i] = PA2PTE((uint64)c_pg2 - PV_OFFSET) | PTE_V | PTE_R | PTE_W  | PTE_X | PTE_U;
                    } else c_pg1[j] = 0;
                }
            } else c_pg0[i] = PA2PTE((uint64)c_pg1 - PV_OFFSET) | PTE_V | PTE_R | PTE_W  | PTE_X | PTE_U;
        } else c_pg0[i] = 0;
    }
    child->pg_t = c_pg0;
    lock(&parent->lock);
    add_before(&parent->child_list, &child->child_list_node);
    unlock(&parent->lock);
    child->tcontext.a0 = 0; // 子进程返回值为0
    lock(&list_lock);
    add_after(&ready_list, &child->status_list_node);
    unlock(&list_lock);
    return child->pid;
}

int sys_waitpid(int pid, int *code, int options)
{
    struct Process *proc = getcpu()->cur_proc;
    list *l;
    struct Process *p;
    if(is_empty_list(&proc->child_list))
        return -1;
    if(options & WNOHANG) // 不阻塞
    {
        if(pid == -1) // 任意子进程
        {
            lock(&proc->lock);
            l = proc->child_list.next;
            while(l != &proc->child_list)
            {
                p = child_list_node2proc(l);
                if(p->status == PROC_DEAD)
                {
                    del_list(&p->child_list_node);
                    del_list(&p->status_list_node); // 摘除
                    if(code)
                    {
                        uint32 ccode = p->code;
                        ccode = ccode << 8;
                        *code = ccode;
                    }
                    pid = p->pid;
                    // 释放进程结构体
                    free_process_struct(p);
                    unlock(&proc->lock);
                    return pid;
                }
                l = l->next;
            }
            unlock(&proc->lock);
            return 0; // 非阻塞未找到结束的进程直接返回0
        } else // 指定子进程
        {
            lock(&proc->lock);
            l = proc->child_list.next;
            while(l != &proc->child_list)
            {
                p = child_list_node2proc(l);
                if(p->pid == pid) // 指定进程存在
                {
                    if(p->status == PROC_DEAD)
                    {
                        del_list(&p->child_list_node);
                        del_list(&p->status_list_node); // 摘除
                        if(code)
                        {
                            uint32 ccode = p->code;
                            ccode = ccode << 8;
                            *code = ccode;
                        }
                        // 释放进程结构体
                        free_process_struct(p);
                        unlock(&proc->lock);
                        return pid;
                    } else
                    {
                        unlock(&proc->lock);
                        return 0;
                    }
                }
                l = l->next;
            }
            unlock(&proc->lock);
            return -1; // 指定进程不存在
        }
    } else { // 阻塞
        if(pid == -1) // 任意子进程
        {
            while(1)
            {
                P(&proc->signal); // 阻塞
                lock(&proc->lock);
                l = proc->child_list.next;
                while(l != &proc->child_list)
                {
                    p = child_list_node2proc(l);
                    if(p->status == PROC_DEAD)
                    {
                        del_list(&p->child_list_node);
                        del_list(&p->status_list_node); // 摘除
                        if(code)
                        {
                            uint32 ccode = p->code;
                            ccode = ccode << 8;
                            *code = ccode;
                        }
                        pid = p->pid;
                        // 释放进程结构体
                        free_process_struct(p);
                        unlock(&proc->lock);
                        return pid;
                    }
                    l = l->next;
                }
                unlock(&proc->lock);
            }
        } else // 指定子进程
        {
            lock(&proc->lock);
            l = proc->child_list.next;
            while(l != &proc->child_list)
            {
                p = child_list_node2proc(l);
                if(p->pid == pid) // 指定进程存在
                {
                    while(1)
                    {
                        unlock(&proc->lock);
                        P(&proc->signal);
                        lock(&proc->lock);
                        if(p->status == PROC_DEAD)
                        {
                            del_list(&p->child_list_node);
                            del_list(&p->status_list_node); // 摘除
                            if(code)
                            {
                                uint32 ccode = p->code;
                                ccode = ccode << 8;
                                *code = ccode;
                            }
                            // 释放进程结构体
                            free_process_struct(p);
                            unlock(&proc->lock);
                            return pid;
                        } else V(&proc->signal); // 说明此信号是其他子进程发出的，因此需要V还原信号继续等待
                    }
                }
                l = l->next;
            }
            unlock(&proc->lock);
            return -1; // 指定进程不存在
        }
    }
    return -1;
}

void sys_exit(int code)
{
    struct Process *proc = getcpu()->cur_proc;
    lock(&proc->lock);
    proc->status = PROC_DEAD; // 置状态位为DEAD
    proc->code = code;
    struct Process *parent = proc->parent;
    lock(&parent->lock);
    list *l = proc->child_list.next;
    while(l!=&proc->child_list) // 将子进程加入到父进程的子进程队列中
    {
        struct Process *child = child_list_node2proc(l);
        l = l->next;
        if(child->status == PROC_DEAD) // 有未释放的子进程pcb则释放，否则将子进程加入其父进程
        {
            free_process_struct(child);
        } else add_before(&parent->child_list, &child->child_list_node);
    }
    unlock(&parent->lock);
    free_process_mem(proc);
    V(&parent->signal);
    move_switch2sched(proc, &dead_list);
}

int sys_getppid(void)
{
    struct Process *proc = getcpu()->cur_proc;
    if(proc->parent)
        return proc->parent->pid;
    else return 0;
}

int sys_getpid(void)
{
    return getcpu()->cur_proc->pid;
}

int sys_uname(struct utsname *uts)
{
    strcpy(uts->sysname, "LinanOS");
    strcpy(uts->nodename, "Calvin");
    strcpy(uts->release, "1.0");
    strcpy(uts->version, "1.0");
    strcpy(uts->machine, "risc-v");
    strcpy(uts->domainname, "gitlab.com");
    return 0;
}

void sys_sched_yield(void)
{
    struct Process *proc = getcpu()->cur_proc;
    lock(&proc->lock);
    proc->status = PROC_READY;
    move_switch2sched(proc, &ready_list);
}

int sys_gettimeofday(struct TimeVal *time, int tz)
{
    uint64 t = get_time();
    time->sec = t / TIMER_FRQ;
    time->usec = (t % TIMER_FRQ) * 1000000 / TIMER_FRQ;
    return 0;
}

int sys_sleep(struct TimeVal *timein, struct TimeVal *timeout)
{
    struct Process *proc = getcpu()->cur_proc;
    lock(&proc->lock);
    proc->t_wait = get_time() + timein->sec * TIMER_FRQ + timein->usec * TIMER_FRQ / 1000000;
    proc->status = PROC_WAIT;
    move_switch2sched(proc, &wait_list);
    return 0;
}

/******************************************************************/

void do_sys_read(struct trap_context *context)
{
    context->a0 = sys_read(context->a0, context->a1, context->a2);
}

void do_sys_write(struct trap_context *context)
{
    context->a0 = sys_write(context->a0, context->a1, context->a2);
}

void do_sys_clone(struct trap_context *context)
{
    context->a0 = sys_clone();
}

void do_sys_waitpid(struct trap_context *context)
{
    context->a0 = sys_waitpid(context->a0, context->a1, context->a2);
}

void do_sys_exit(struct trap_context *context)
{
    sys_exit(context->a0);
}

void do_sys_getppid(struct trap_context *context)
{
    context->a0 = sys_getppid();
}

void do_sys_getpid(struct trap_context *context)
{
    context->a0 = sys_getpid();
}

void do_sys_uname(struct trap_context *context)
{
    context->a0 = sys_uname(context->a0);
}

void do_sys_sched_yield(struct trap_context *context)
{
    sys_sched_yield();
}

void do_sys_gettimeofday(struct trap_context *context)
{
    context->a0 = sys_gettimeofday(context->a0, context->a1);
}

void do_sys_sleep(struct trap_context *context)
{
    context->a0 = sys_sleep(context->a0, context->a1);
}


void syscall_init(void)
{
    int i;
    for(i = 0; i < SYS_NR; i++)
    {
        syscall_table[i] = unknown_syscall;
    }
    syscall_table[SYS_test] = sys_test;
    syscall_table[SYS_read] = do_sys_read;
    syscall_table[SYS_write] = do_sys_write;
    syscall_table[SYS_clone] = do_sys_clone;
    syscall_table[SYS_wait4] = do_sys_waitpid;
    syscall_table[SYS_exit] = do_sys_exit;
    syscall_table[SYS_getppid] = do_sys_getppid;
    syscall_table[SYS_getpid] = do_sys_getpid;
    syscall_table[SYS_uname] = do_sys_uname;
    syscall_table[SYS_sched_yield] = do_sys_sched_yield;
    syscall_table[SYS_gettimeofday] = do_sys_gettimeofday;
    syscall_table[SYS_nanosleep] = do_sys_sleep;

}