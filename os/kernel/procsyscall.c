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
#include "vmm.h"
#include "signal.h"

int sys_clone(uint flags, void *stack, int *ptid, uint64 tls, int *ctid)
{
    struct Process *parent = getcpu()->cur_proc;
    struct Process *child = new_proc();
    child->parent = parent;

    vfs_file_t *file = vfs_fd2file(parent->kfd);
    child->kfd = vfs_open(file->relative_path, file->flags);
    char abspath[VFS_PATH_MAX];
    memset(abspath, 0, VFS_PATH_MAX);
    vfs_dir_t *dir = parent->cwd;
    int len_inode = strlen(dir->inode->name);
    int len_relative = strlen(dir->relative_path);
    if(len_inode + len_relative >= VFS_PATH_MAX) return -1;
    strncpy(abspath, dir->inode->name, len_inode);
    strncpy(abspath+len_inode, dir->relative_path, len_relative);
    child->cwd = vfs_opendir(abspath);

    #ifdef _DEBUG
    printk("clone ppid %d pid %d childfd %d cwd %lx\n", parent->pid, child->pid, child->kfd, child->cwd);
    #endif
    if(!child->cwd)
    {
        printk("abspath %s\n", abspath);
        while(1) {}
    }
    memcpy(&child->tcontext, &parent->tcontext, sizeof(struct trap_context));
    child->tcontext.k_sp = (uint64)(child->k_stack) + PAGE_SIZE - 8;
    child->tcontext.sepc += 4; // 子进程pc+4执行ecall下一条指令
    child->pcontext.ra = (uint64)trap_ret; // 子进程从trap_ret开始执行从内核态转换为用户态
    if(stack)
        child->tcontext.sp = stack;

    uint64 *p_pg0, *p_pg1, *p_pg2, *p_pg3;
    uint64 *c_pg0, *c_pg1, *c_pg2, *c_pg3;
        
    p_pg0 = parent->pg_t;
    c_pg0 = vm_alloc_pages(1);
    copy_kpgt(c_pg0);
    child->pg_t = c_pg0;

    list *l = parent->vma_list_head.next;
    struct vma *vma = NULL;
    while(l != &parent->vma_list_head)
    {
        vma = vma_list_node2vma(l);
        struct vma *nvma = new_vma();

        if((vma->type & VMA_PROG) && (vma->prot & PROT_EXEC)) // 可执行部分才使用写时复制
        {
            nvma->va = vma->va;
            // vma->pa = (uint64)mem - PV_OFFSET;
            nvma->size = vma->size;
            
            uint prot = PROT_READ;
            if(vma->prot & PROT_EXEC) prot |= PROT_EXEC;
            nvma->prot = prot; // 取消可写属性

            nvma->flag = vma->flag;
            nvma->type = vma->type | VMA_CLONE;
            nvma->vmoff = vma->vmoff;
            nvma->fsize = vma->fsize;
            nvma->foff = vma->foff;
            nvma->file = NULL;

            uint64 vai;
            for(vai = vma->va; vai < vma->va + vma->size; vai += PAGE_SIZE)
            {
                uint64 pa = va2pa(parent->pg_t, vai);
                if(pa)
                {
                    mappages(child->pg_t, vai, pa, 1, prot2pte(nvma->prot) | PTE_F);
                }
            }
        } else {
            nvma->va = vma->va;
            nvma->size = vma->size;
            nvma->prot = vma->prot;
            nvma->flag = vma->flag;
            nvma->type = vma->type;
            nvma->vmoff = vma->vmoff;
            nvma->fsize = vma->fsize;
            nvma->foff = vma->foff;
            
            uint64 vai;
            for(vai = vma->va; vai < vma->va + vma->size; vai += PAGE_SIZE)
            {
                uint64 pa = va2pa(parent->pg_t, vai);
                if(pa)
                {
                    void *mem = vm_alloc_pages(1);
                    memcpy(mem, pa + PV_OFFSET, PAGE_SIZE);
                    mappages(child->pg_t, vai, (uint64)mem - PV_OFFSET, 1, prot2pte(nvma->prot));
                }
            }
        }
            
        if(vma->file)
        {
            nvma->file = vfs_fd2file(vfs_open(vma->file->relative_path, vma->file->flags));
        } else 
        {
            nvma->file = NULL;
        }
        add_before(&child->vma_list_head, &nvma->vma_list_node);
        l = l->next;
    }
    
    lock(&parent->lock);
    add_before(&parent->child_list_head, &child->child_list_node);

    // TODO ADD UFILE CLONE

    l = parent->ufiles_list_head.next;
    while(l != &parent->ufiles_list_head)
    {
        ufile_t *file = list2ufile(l);
        l = l->next;
        ufile_t *cfile = kmalloc(sizeof(ufile_t));
        cfile->ufd = file->ufd;
        cfile->type = file->type;
        cfile->private = NULL;
        
        switch(cfile->type)
        {
            case UTYPE_FILE:
            {
                vfs_file_t *vf = vfs_fd2file((int)file->private);
                int nfd = vfs_open(vf->relative_path, vf->flags);
                if(nfd < 0)
                {
                    ufile_free(cfile);
                    continue;
                }
                cfile->private = nfd;
                break;
            }
            case UTYPE_DIR:
            {
                cfile->type = UTYPE_DIR;
                char abspath[VFS_PATH_MAX];
                vfs_dir_t *dir = file->private;
                memset(abspath, 0, VFS_PATH_MAX);
                int len_inode = strlen(dir->inode->name);
                int len_relative = strlen(dir->relative_path);
                if(len_inode + len_relative >= VFS_PATH_MAX)
                {
                    ufile_free(cfile);
                    continue;
                }
                strcpy(abspath, dir->inode->name);
                strcpy(abspath+len_inode, dir->relative_path);
                vfs_dir_t *ndir = vfs_opendir(abspath);
                if(!ndir)
                {
                    ufile_free(cfile);
                    continue;
                }
                cfile->private = ndir;
                break;
            }
            case UTYPE_PIPEIN:
            {
                cfile->type = UTYPE_PIPEIN;
                cfile->private = file->private;
                pipe_t *pipin = cfile->private;
                lock(&pipin->mutex);
                pipin->r_ref++;
                unlock(&pipin->mutex);
                break;
            }
            case UTYPE_PIPEOUT:
            {
                cfile->type = UTYPE_PIPEOUT;
                cfile->private = file->private;
                pipe_t *pipin = cfile->private;
                lock(&pipin->mutex);
                pipin->w_ref++;
                unlock(&pipin->mutex);
                break;
            }
            case UTYPE_STDIN:
            {
                int nfd = vfs_open("/dev/stdin", O_RDONLY);
                if(nfd < 0)
                {
                    ufile_free(cfile);
                    continue;
                }
                cfile->private = (void*)nfd;
                break;
            }
            case UTYPE_STDOUT:
            {
                int nfd = vfs_open("/dev/stdout", O_WRONLY);
                if(nfd < 0)
                {
                    ufile_free(cfile);
                    continue;
                }
                cfile->private = (void*)nfd;
                break;
            }
            case UTYPE_STDERR:
            {
                int nfd = vfs_open("/dev/stderr", O_WRONLY);
                if(nfd < 0)
                {
                    ufile_free(cfile);
                    continue;
                }
                cfile->private = (void*)nfd;
                break;
            }
            default:
                break;
        }
        add_before(&child->ufiles_list_head, &cfile->list_node);
    }

    copy_proc_sigactions(child, parent);

    unlock(&parent->lock);
    child->tcontext.a0 = 0; // 子进程返回值为0
    lock(&list_lock);
    add_after(&ready_list, &child->status_list_node);
    unlock(&list_lock);
    return child->pid;
}

int sys_wait4(int pid, int *code, int options)
{
    struct Process *proc = getcpu()->cur_proc;
    list *l;
    struct Process *p;
    if(is_empty_list(&proc->child_list_head))
        return -1;
    if(options & WNOHANG) // 不阻塞
    {
        if(pid == -1) // 任意子进程
        {
            lock(&proc->lock);
            l = proc->child_list_head.next;
            while(l != &proc->child_list_head)
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
            l = proc->child_list_head.next;
            while(l != &proc->child_list_head)
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
                P(&proc->sleep_lock); // 阻塞
                if(proc->receive_kill > 0)
                    break;
                lock(&proc->lock);
                l = proc->child_list_head.next;
                while(l != &proc->child_list_head)
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
            return -1;
        } else // 指定子进程
        {
            lock(&proc->lock);
            l = proc->child_list_head.next;
            while(l != &proc->child_list_head)
            {
                p = child_list_node2proc(l);
                if(p->pid == pid) // 指定进程存在
                {
                    while(1)
                    {
                        unlock(&proc->lock);
                        P(&proc->sleep_lock);
                        if(proc->receive_kill > 0)
                            break;
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
                        } else V(&proc->sleep_lock); // 说明此信号是其他子进程发出的，因此需要V还原信号继续等待
                    }
                    return -1;
                }
                l = l->next;
            }
            unlock(&proc->lock);
            return -1; // 指定进程不存在
        }
    }
    return -1;
}
extern int timerprint;

void sys_exit(int code)
{
    struct Process *proc = getcpu()->cur_proc;
    #ifdef _DEBUG
    printk("\n\nexit pid %d code %d\n\n\n", proc->pid, code);
    #endif
    if(proc->pid == 1)
        timerprint = 0;
    lock(&proc->lock);
    proc->status = PROC_DEAD; // 置状态位为DEAD
    proc->end_time = get_time();
    proc->code = code;
    struct Process *parent = proc->parent;
    lock(&parent->lock);
    list *l = proc->child_list_head.next;

    while(l!=&proc->child_list_head) // 将子进程加入到父进程的子进程队列中
    {
        struct Process *child = child_list_node2proc(l);
        l = l->next;
        child->parent = NULL;
        if(child->status == PROC_DEAD) // 有未释放的子进程pcb则释放，否则将子进程加入其父进程
        {
            if(child->k_stack)
                vm_free_pages(child->k_stack, 1);
            free_process_struct(child);
        } else add_before(&parent->child_list_head, &child->child_list_node);
    }
    unlock(&parent->lock);

    switch2kpgt(); // 先切换回内核页表再释放当前进程页表
    free_process_mem(proc);
    free_pagetable(proc->pg_t);
    free_ufile_list(proc);
    free_process_sigpendings(proc);
    if(proc->cwd) vfs_closedir(proc->cwd);
    if(proc->kfd) vfs_close(proc->kfd);

    send_signal(parent, SIGCHLD); // 向父进程发送子进程退出的信号
    V(&parent->sleep_lock); // 唤醒父进程
    move_switch2sched(proc, &dead_list); // 切换到调度程序
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
    list *l = proc->child_list_head.next;
    while(l != &proc->child_list_head)
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
    strcpy(uts->sysname, "Linux");
    strcpy(uts->nodename, "debian");
    strcpy(uts->release, "5.10.0-7-riscv64");
    strcpy(uts->version, "#1 SMP Debian 5.10.40-1 (2021-05-28)");
    strcpy(uts->machine, "riscv64");
    strcpy(uts->domainname, "(none)");
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

int sys_nanosleep(struct TimeSpec *timein, struct TimeSpec *timeout)
{
    if(!timein || !timeout) return -1;
    struct Process *proc = getcpu()->cur_proc;
    lock(&proc->lock);
    proc->t_wait = get_time() + timein->sec * TIMER_FRQ + timein->nsec * TIMER_FRQ / 1000000000;
    proc->status = PROC_WAIT;
    move_switch2sched(proc, &wait_list);
    timeout->sec = 0;
    timeout->nsec = 0;
    return 0;
}

uint64 sys_brk(void* addr)
{
    struct Process *proc = getcpu()->cur_proc;
    if(!addr)
        return proc->brk;
    
    if((uint64)addr < (uint64)proc->minbrk) return -1;
    if((uint64)addr == (uint64)proc->brk) return addr;
    else if((uint64)addr > (uint64)proc->brk) // 扩展堆
    {
        uint64 addr_begin = PAGEDOWN((uint64)proc->brk);
        uint64 addr_end = PAGEDOWN((uint64)addr + PAGE_SIZE);

        int pagen = (addr_end >> 12) - (addr_begin >> 12) - 1;
        if(!pagen)
        {
            proc->brk = addr;
            return addr;
        }
        uint64 addri;
        for(addri = addr_begin + PAGE_SIZE; addri < addr_end; addri += PAGE_SIZE) // 建立新页
        {
            if(!find_vma(proc, addri))
            {
                // void *mem = kmalloc(PAGE_SIZE);
                struct vma *vma = new_vma();
                vma->type = VMA_BRK;
                vma->foff = 0;
                vma->file = NULL;
                vma->flag = PTE_R | PTE_W | PTE_U;
                vma->prot = PROT_READ | PROT_WRITE;
                vma->va = addri;
                vma->size = PAGE_SIZE;
                add_vma(proc, vma);
                // mappages(proc->pg_t, vma->va, vma->pa, 1, prot2pte(vma->prot));
                // sfence_vma();
            }
        }
        proc->brk = addr;
        return addr;
    } else { // 减小堆
        uint64 addr_begin = PAGEDOWN((uint64)addr + PAGE_SIZE);
        uint64 addr_end = PAGEDOWN((uint64)proc->brk + PAGE_SIZE);
        uint64 addri;
        for(addri = addr_begin; addri < addr_end; addri += PAGE_SIZE)
        {
            struct vma *vma = find_vma(proc, addri);
            del_list(&vma->vma_list_node);
            free_vma_mem(proc->pg_t, vma);
            // unmappages(proc->pg_t, vma->va, vma->size / PAGE_SIZE);
            sfence_vma();
            kfree(vma);
        }
        proc->brk = addr;
        return addr;
    }
    return -1;
}

uint64 sys_mmap(uint64 start, uint64 len, uint64 prot, uint64 flags, uint64 ufd, uint64 off)
{
    struct Process *proc = getcpu()->cur_proc;
    len = PAGEUP(len); // 长度自动调整为按页对齐
    if(prot == PROT_NONE) return start;

    if(!len) {
        return -1;
    }
    uint64 addr;
    struct vma *vma = find_map_area(proc, start, len);

    if(!vma) return -1; // 找不到符合条件的内存空间
    
    vma->size = len;
    vma->prot = prot;
    vma->flag = flags;
    vma->type = VMA_MMAP;

    ufile_t *file = ufd2ufile(ufd, proc);
    if(file)
    {
        vma->file = vfs_fd2file((int)file->private);
        vma->foff = off;
    } else
    {
        vma->file = NULL;
    }
    add_vma(proc, vma);
    return vma->va;
}

int sys_munmap(void *start, int len)
{
    struct Process *proc = getcpu()->cur_proc;
    struct vma *vma = find_vma(proc, start);
    if(!vma) return -1;
    del_list(&vma->vma_list_node);
    free_vma_mem(proc->pg_t, vma);
    kfree(vma);
    sfence_vma();
    return 0;
}

int sys_mprotect(void *addr, uint64 len, uint prot)
{
    // printk("addr: %lx len %lx prot %lx\n", addr, len, prot);
    return 0;
    if(!addr && !len)
    {
        addr = MAP_LOW;
        len = MAP_TOP - MAP_LOW;
    }
    if(PAGEDOWN((uint64)addr) != (uint64)addr) return -1;
    if(len % PAGE_SIZE != 0) return -1;

    uint64 begin = (uint64)addr;
    uint64 end = (uint64)addr + len;
    struct Process *proc = getcpu()->cur_proc;
    list *l = proc->vma_list_head.next;
    struct vma *vma = NULL;
    lock(&proc->lock);
    while(l != &proc->vma_list_head)
    {
        vma = vma_list_node2vma(l);
        if(vma->va >= end) break;
        else if(vma->va >= begin && (vma->va + vma->size) < end)
        {
            vma->prot = prot;
            uint64 vai, pa;
            for(vai = vma->va; vai < vma->va + vma->size; vai += PAGE_SIZE)
            {
                pa = va2pa(proc->pg_t, vai);
                if(pa)
                {
                    mappages(proc->pg_t, vai, pa, 1, prot2pte(vma->prot));
                }
            }
            
        }
        l = l->next;
    }
    unlock(&proc->lock);
    sfence_vma();
    return 0;
}

int sys_set_tid_address(void *addr)
{
    return getcpu()->cur_proc->pid;
}

int sys_getuid(void)
{
    return 0;
}

int sys_geteuid(void)
{
    return 0;
}

int sys_getgid(void)
{
    return 0;
}

int sys_getegid(void)
{
    return 0;
}

// /etc/localtime
// /proc/mounts
// readlinkat /proc/self/exe

int sys_exit_group(void)
{
    return 0;
}

int sys_clock_gettime(int clockid, struct TimeSpec *time)
{
    if(!time) return -1;
    uint64 t = get_time();
    struct Process *proc = getcpu()->cur_proc;

    if (clockid == CLOCK_PROCESS_CPUTIME_ID || clockid == CLOCK_THREAD_CPUTIME_ID)
        t = t - proc->start_time;
    t = proc->stime;
    time->sec = t / TIMER_FRQ;
    time->nsec = (t % TIMER_FRQ) * 1000000000 / TIMER_FRQ; // 注意这里返回的是纳秒而不是微秒
    return 0;
}

int sys_clock_nanosleep(int clockid, int flags, struct TimeSpec *request, struct TimeSpec *remain)
{
    if(!request || !remain) return -1;
    struct Process *proc = getcpu()->cur_proc;
    lock(&proc->lock);
    proc->t_wait = get_time() + request->sec * TIMER_FRQ + request->nsec * TIMER_FRQ / 1000000000;
    proc->status = PROC_WAIT;
    move_switch2sched(proc, &wait_list);
    remain->sec = 0;
    remain->nsec = 0;
    return 0;
}

int sys_getrusage(int who, struct rusage *usage)
{
    if(!usage) return -1;
    return 0;
    if(who == RUSAGE_SELF)
    {
        struct Process *proc = getcpu()->cur_proc;
        usage->ru_utime.sec = proc->utime / TIMER_FRQ;
        usage->ru_utime.usec = (proc->utime % TIMER_FRQ) * 1000000 / TIMER_FRQ;
        usage->ru_stime.sec = proc->stime / TIMER_FRQ;
        usage->ru_stime.usec = (proc->stime % TIMER_FRQ) * 1000000 / TIMER_FRQ;

        return 0;
    }

    return -1;
}

int sys_kill(int pid, int signo)
{
    struct Process *proc = get_proc_by_pid(pid);
    if(!proc) return -1;
    send_signal(proc, signo);

    if(signo == SIGKILL || signo == SIGTERM)
        proc->receive_kill++;
    
    if(proc->status == PROC_SLEEP)
    {
        lock(&list_lock);
        lock(&proc->lock);
        proc->data = 0;
        proc->status = PROC_READY;
        del_list(&proc->status_list_node);
        add_after(&ready_list, &proc->status_list_node); // 加到头优先被调度
        unlock(&proc->lock);
        unlock(&list_lock);
    }
    return 0;
}
