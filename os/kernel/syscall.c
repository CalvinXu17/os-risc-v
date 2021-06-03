#include "syscall.h"
#include "printk.h"
#include "process.h"
#include "sched.h"
#include "console.h"
#include "string.h"
#include "page.h"
#include "kmalloc.h"
#include "timer.h"
#include "fs.h"
#include "pipe.h"
#include "page.h"

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

extern int sys_test(const char *path);
void do_sys_test(struct trap_context *context)
{
    context->a0 = sys_test(context->a0);
}

int sys_clone(uint flags, void *stack, int ptid, int tls, int ctid)
{
    if(flags & SIGCHLD)
    {
        struct Process *parent = getcpu()->cur_proc;
        struct Process *child = new_proc();
        child->parent = parent;
        
        memcpy(&child->tcontext, &parent->tcontext, sizeof(struct trap_context));
        child->tcontext.k_sp = (uint64)(child->k_stack) + PAGE_SIZE;
        child->tcontext.sepc += 4; // 子进程pc+4执行ecall下一条指令
        child->pcontext.ra = (uint64)trap_ret; // 子进程从trap_ret开始执行从内核态转换为用户态
        if(stack)
            child->tcontext.sp = stack;

        int i, j, k;

        uint64 *p_pg0, *p_pg1, *p_pg2, *p_pg3;
        uint64 *c_pg0, *c_pg1, *c_pg2, *c_pg3;
        
        p_pg0 = parent->pg_t;
        c_pg0 = kmalloc(PAGE_SIZE);
        
        c_pg0[508] = k_pg_t[508];
        c_pg0[509] = k_pg_t[509];
        c_pg0[510] = k_pg_t[510];

        for(i = 0; i < 256; i++)
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

        // TODO ADD UFILE CLONE
        list *l = parent->ufiles_list.next;
        while(l != &parent->ufiles_list)
        {
            ufile_t *file = list2ufile(l);
            l = l->next;
            if(file->type == UTYPE_PIPEIN)
            {
                ufile_t *cfile = ufile_alloc_by_fd(file->ufd, child);
                if(cfile)
                {
                    cfile->type = UTYPE_PIPEIN;
                    cfile->private = file->private;
                    pipe_t *pipin = cfile->private;
                    lock(&pipin->mutex);
                    pipin->r_ref++;
                    unlock(&pipin->mutex);
                }
            } else if(file->type == UTYPE_PIPEOUT)
            {
                ufile_t *cfile = ufile_alloc_by_fd(file->ufd, child);
                if(cfile)
                {
                    cfile->type = UTYPE_PIPEOUT;
                    cfile->private = file->private;
                    pipe_t *pipin = cfile->private;
                    lock(&pipin->mutex);
                    pipin->w_ref++;
                    unlock(&pipin->mutex);
                }
            }
        }

        unlock(&parent->lock);
        child->tcontext.a0 = 0; // 子进程返回值为0
        lock(&list_lock);
        add_after(&ready_list, &child->status_list_node);
        unlock(&list_lock);
        return child->pid;
    }
    return -1;
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
    proc->end_time = get_time();
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
            kfree(child->k_stack);
            free_process_struct(child);
        } else add_before(&parent->child_list, &child->child_list_node);
    }
    unlock(&parent->lock);
    free_process_mem(proc);
    free_ufile_list(proc);
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

int sys_times(struct tms *tms)
{
    if(!tms) return -1;
    struct Process *proc = getcpu()->cur_proc;
    tms->tms_utime = proc->utime;
    tms->tms_stime = (proc->stime + get_time() - proc->stime_start);
    tms->tms_cstime = 0;
    tms->tms_cutime = 0;
    lock(&proc->lock);
    list *l = proc->child_list.next;
    while(l != &proc->child_list)
    {
        struct Process *child = child_list_node2proc(l);
        if(child->status == PROC_DEAD)
        {
            tms->tms_cutime += child->utime;
            tms->tms_cstime += child->stime;
        }
    }
    unlock(&proc->lock);
    return 0;
}

int sys_uname(struct utsname *uts)
{
    if(!uts) return -1;
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
    if(!time) return -1;
    uint64 t = get_time();
    time->sec = t / TIMER_FRQ;
    time->usec = (t % TIMER_FRQ) * 1000000 / TIMER_FRQ;
    return 0;
}

int sys_sleep(struct TimeVal *timein, struct TimeVal *timeout)
{
    if(!timein || !timeout) return -1;
    struct Process *proc = getcpu()->cur_proc;
    lock(&proc->lock);
    proc->t_wait = get_time() + timein->sec * TIMER_FRQ + timein->usec * TIMER_FRQ / 1000000;
    proc->status = PROC_WAIT;
    move_switch2sched(proc, &wait_list);
    return 0;
}

uint64 sys_brk(void* addr)
{
    struct Process *proc = getcpu()->cur_proc;
    if(!addr)
    {
        return proc->brk;
    }
    if((uint64)addr < (uint64)proc->minbrk) return -1;
    if((uint64)addr == (uint64)proc->brk) return 0;
    else if((uint64)addr > (uint64)proc->brk) // 扩展堆
    {
        uint64 pageaddrend = (uint64)addr;
        if(pageaddrend & 0xfff)
        {
            pageaddrend = ((pageaddrend >> 12) + 1) << 12;
        } else pageaddrend = (pageaddrend >> 12) << 12;

        int pagen = (pageaddrend >> 12) - ((uint64)proc->brk >> 12) - 1;
        if(!pagen)
        {
            proc->brk = addr;
            return 0;
        }
        if(pagen > free_page_pool.page_num) return -1;
        if(!build_pgt(proc->pg_t, (((uint64)proc->brk >> 12) + 1) << 12, pagen)) return -1;
        proc->brk = addr;
        return 0;
    } else { // 减小堆
        uint64 pageaddrstart = (uint64)addr;
        if(pageaddrstart & 0xfff)
        {
            pageaddrstart = ((pageaddrstart >> 12) + 1) << 12;
        } else pageaddrstart = (pageaddrstart >> 12) << 12;

        int pagen = ((uint64)proc->brk >> 12) + 1 - (pageaddrstart >> 12);
        if(!pagen)
        {
            proc->brk = addr;
            return 0;
        }
        
        // 开始释放
        uint64 *pg0, *pg1, *pg2, *pg3;
        uint64 pg0_index = (pageaddrstart >> 30) & 0x1FF;
        uint64 pg1_index = (pageaddrstart >> 21) & 0x1FF;
        uint64 pg2_index = (pageaddrstart >> 12) & 0x1FF;
        int i, j, k;
        int cnt = 0;
        pg0 = proc->pg_t;
        for(i = pg0_index; i < 256; i++)
        {
            if(pg0[i] & PTE_V)
            {
                pg1 = PTE2PA(pg0[i]) + PV_OFFSET;
                if(!(pg0[i] & PTE_R) && !(pg0[i] & PTE_W) && !(pg0[i] & PTE_X)) // 指向下一级
                {
                    for(j = pg1_index; j < 256; j++)
                    {
                        if(pg1[j] & PTE_V)
                        {
                            pg2 = PTE2PA(pg1[j]) + PV_OFFSET;
                            if(!(pg1[j] & PTE_R) && !(pg1[j] & PTE_W) && !(pg1[j] & PTE_X)) // 指向下一级
                            {
                                for(k = pg2_index; k < 256; k++)
                                {
                                    if(pg2[k] & PTE_V)
                                    {
                                        pg3 = PTE2PA(pg2[k]) + PV_OFFSET;
                                        kfree(pg3);
                                        pg2[k] = 0;
                                        cnt++;
                                        if(cnt == pagen)
                                        {
                                            proc->brk = addr;
                                            return 0;
                                        }
                                    }
                                }
                            }
                            kfree(pg2);
                            pg1[j] = 0;
                        }
                    }
                }
                kfree(pg1);
                pg0[i] = 0;
            }
        }
    }
    return -1;
}

uint64 sys_mmap(uint64 start, uint64 len, uint64 prot, uint64 flags, uint64 ufd, uint64 off)
{
    struct Process *proc = getcpu()->cur_proc;
    
    ufile_t *file = ufd2ufile(ufd, proc);
    if(!file) return -1;
    if(file->type != UTYPE_FILE) return -1;

    int fd = (int)file->private;

    if(!start)
    {
        vfs_lseek(fd, off, VFS_SEEK_SET);
        void *brk = proc->brk;
        if(sys_brk((uint64)brk + len) == -1) return -1;
        vfs_read(fd, brk, len);
        return brk;
    } else
    {

    }
    return -1;
}

int sys_munmap(void *start, int len)
{
    uchar *p = start;
    while(p < (uint64)start+len)
    {
        *p = 0;
        p++;
    }
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
    context->a0 = sys_clone(context->a0, context->a1, context->a2, context->a3, context->a4);
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

void do_sys_times(struct trap_context *context)
{
    context->a0 = sys_times(context->a0);
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

void do_sys_brk(struct trap_context *context)
{
    context->a0 = sys_brk(context->a0);
}

void do_sys_mmap(struct trap_context *context)
{
    context->a0 = sys_mmap(context->a0, context->a1, context->a2, context->a3, context->a4, context->a5);
}

void do_sys_munmap(struct trap_context *context)
{
    context->a0 = sys_munmap(context->a0, context->a1);
}

void do_sys_getcwd(struct trap_context *context)
{
    context->a0 = sys_getcwd(context->a0, context->a1);
}

void do_sys_chdir(struct trap_context *context)
{
    context->a0 = sys_chdir(context->a0);
}

void do_sys_openat(struct trap_context *context)
{
    context->a0 = sys_openat(context->a0, context->a1, context->a2, context->a3);
}

void do_sys_close(struct trap_context *context)
{
    context->a0 = sys_close(context->a0);
}

void do_sys_mkdirat(struct trap_context *context)
{
    context->a0 = sys_mkdirat(context->a0, context->a1, context->a2);
}

void do_sys_pipe2(struct trap_context *context)
{
    context->a0 = sys_pipe2(context->a0);
}

void do_sys_dup(struct trap_context *context)
{
    context->a0 = sys_dup(context->a0);
}

void do_sys_dup2(struct trap_context *context)
{
    context->a0 = sys_dup2(context->a0, context->a1);
}

void do_sys_getdents64(struct trap_context *context)
{
    context->a0 = sys_getdents64(context->a0, context->a1, context->a2);
}

void do_sys_execve(struct trap_context *context)
{
    context->a0 = sys_execve(context->a0, context->a1, context->a2);
}

void do_sys_fstat(struct trap_context *context)
{
    context->a0 = sys_fstat(context->a0, context->a1);
}

void do_sys_unlinkat(struct trap_context *context)
{
    context->a0 = sys_unlinkat(context->a0, context->a1, context->a2);
}

void do_sys_mount(struct trap_context *context)
{
    context->a0 = sys_mount(context->a0, context->a1, context->a2, context->a3, context->a4);
}

void do_sys_umount2(struct trap_context *context)
{
    context->a0 = sys_umount2(context->a0, context->a1);
}


void syscall_init(void)
{
    int i;
    for(i = 0; i < SYS_NR; i++)
    {
        syscall_table[i] = unknown_syscall;
    }
    syscall_table[SYS_test] = do_sys_test;
    syscall_table[SYS_read] = do_sys_read;
    syscall_table[SYS_write] = do_sys_write;
    syscall_table[SYS_clone] = do_sys_clone;
    syscall_table[SYS_wait4] = do_sys_waitpid;
    syscall_table[SYS_exit] = do_sys_exit;
    syscall_table[SYS_getppid] = do_sys_getppid;
    syscall_table[SYS_getpid] = do_sys_getpid;
    syscall_table[SYS_times] = do_sys_times;
    syscall_table[SYS_uname] = do_sys_uname;
    syscall_table[SYS_sched_yield] = do_sys_sched_yield;
    syscall_table[SYS_gettimeofday] = do_sys_gettimeofday;
    syscall_table[SYS_nanosleep] = do_sys_sleep;
    syscall_table[SYS_brk] = do_sys_brk;
    syscall_table[SYS_mmap] = do_sys_mmap;
    syscall_table[SYS_munmap] = do_sys_munmap;

    syscall_table[SYS_getcwd] = do_sys_getcwd;
    syscall_table[SYS_chdir] = do_sys_chdir;
    syscall_table[SYS_openat] = do_sys_openat;
    syscall_table[SYS_close] = do_sys_close;
    syscall_table[SYS_mkdirat] = do_sys_mkdirat;
    syscall_table[SYS_pipe2] = do_sys_pipe2;
    syscall_table[SYS_dup] = do_sys_dup;
    syscall_table[SYS_dup3] = do_sys_dup2;
    syscall_table[SYS_getdents64] = do_sys_getdents64;
    syscall_table[SYS_execve] = do_sys_execve;
    syscall_table[SYS_fstat] = do_sys_fstat;
    syscall_table[SYS_unlinkat] = do_sys_unlinkat;
    syscall_table[SYS_mount] = do_sys_mount;
    syscall_table[SYS_umount2] = do_sys_umount2;
}