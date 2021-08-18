#include "signal.h"
#include "process.h"
#include "printk.h"
#include "vmm.h"
#include "string.h"
#include "kmalloc.h"
#include "spinlock.h"

uchar sigrt_code[] = { 0x93, 0x08, 0xB0, 0x08, 0x73, 0x00, 0x00, 0x00 };
void *dfl_sig_handler[SIGRTMIN] = {0};

static void default_signal(struct sigpending *pending)
{
    return;
}

static void dfl_sigkill(struct sigpending *pending)
{
    kill_current();
}

static void dfl_sigterm(struct sigpending *pending)
{
    kill_current();
}

static void dfl_sigchld(struct sigpending *pending)
{
    struct Process *proc = getcpu()->cur_proc;
    struct Process *p;
    lock(&proc->lock);
    list *l = proc->child_list_head.next;
    
    while(l != &proc->child_list_head)
    {
        p = child_list_node2proc(l);
        if(p->status == PROC_DEAD && p->pid == pending->senderpid)
        {
            del_list(&p->child_list_node);
            del_list(&p->status_list_node); // 摘除
            // 释放进程结构体
            free_process_struct(p);
            break;
        }
        l = l->next;
    }
    
    unlock(&proc->lock);
}

void signal_init(void)
{
    int i;
    for(i=0;i<SIGRTMIN;i++)
        dfl_sig_handler[i] = default_signal;
    dfl_sig_handler[SIGKILL-1] = dfl_sigkill;
    dfl_sig_handler[SIGTERM-1] = dfl_sigterm;
    dfl_sig_handler[SIGCHLD-1] = dfl_sigchld;
    dfl_sig_handler[SIGUSR1-1] = dfl_sigkill;
    dfl_sig_handler[SIGUSR2-1] = dfl_sigkill;
}

void init_proc_sigactions(struct Process *proc)
{
    int i;
    for(i=0; i<SIGRTMIN; i++)
    {
        proc->sigactions[i].sa_handler = SIG_DFL;
        proc->sigactions[i].sa_flags = 0;
        proc->sigactions[i].sa_mask = 0;
    }
}

void copy_proc_sigactions(struct Process *to, struct Process *from)
{
    int i;
    for(i=0; i<SIGRTMIN; i++)
    {
        to->sigactions[i].sa_handler = from->sigactions[i].sa_handler;
        to->sigactions[i].sa_flags = from->sigactions[i].sa_flags;
        to->sigactions[i].sa_mask = from->sigactions[i].sa_mask;
    }
}

void make_sigreturn_code(struct Process *proc)
{
    struct vma *vma = new_vma();
    vma->va = SIG_CODE_START;
    vma->size = PAGE_SIZE;
    vma->prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    vma->flag = PTE_R | PTE_W  | PTE_X | PTE_U;
    vma->type = VMA_DATA;
    vma->vmoff = 0;
    vma->fsize = 0;
    vma->foff = 0;
    vma->file = NULL;
    void *mem = vm_alloc_pages(1);
    memcpy(mem, sigrt_code, 8);
    mappages(proc->pg_t, vma->va, (uint64)mem - PV_OFFSET, 1, prot2pte(vma->prot));
    add_vma(proc, vma);
}

void make_sigframe(struct Process *proc, int signo, void (*sa_handler)(int))
{
    void *sp = proc->tcontext.sp;
    struct trap_context *frame = (uint64)sp - sizeof(struct trap_context);
    memcpy(frame, &proc->tcontext, sizeof(struct trap_context));
    sp = frame;
    proc->tcontext.sp = sp;
    proc->tcontext.ra = SIG_CODE_START;
    proc->tcontext.sepc = sa_handler;
    proc->tcontext.a0 = signo;
}

struct sigpending* new_sigpending(void)
{
    struct sigpending *pending = kmalloc(sizeof(struct sigpending));
    if(!pending) return NULL;
    init_list(&pending->sigpending_list_node);
    pending->senderpid = 0;
    pending->signo = 0;
    return pending;
}

void free_sigpending(struct sigpending *pending)
{
    if(!pending) return;
    kfree(pending);
}

void free_process_sigpendings(struct Process *proc)
{
    list *l = proc->signal_list_head.next;
    struct sigpending *pending;

    while(l != &proc->signal_list_head)
    {
        pending = sigpending_list_node2sigpending(l);
        l = l->next;
        free_sigpending(pending);
    }
    init_list(&proc->signal_list_head);
}

void send_signal(struct Process *proc, int signo)
{
    if(!(signo >= 1 && signo <= SIGRTMIN))
        return;

    struct sigpending *pending = new_sigpending();
    if(!pending)
        return;
    lock(&proc->lock);
    pending->senderpid = getcpu()->cur_proc->pid;
    pending->signo = signo;
    if(signo == SIGKILL || signo == SIGTERM) // 进程退出信号优先被响应，加入队首
        add_after(&proc->signal_list_head, &pending->sigpending_list_node);
    else add_before(&proc->signal_list_head, &pending->sigpending_list_node);
    unlock(&proc->lock);
}

int sys_rt_sigaction(int signo, struct sigaction *act, struct sigaction *oldact, uint64 size)
{
    if(!(signo >= 1 && signo <= SIGRTMIN))
        return -1;
    
    struct Process *proc = getcpu()->cur_proc;
    struct sigaction *cur_sigaction = &proc->sigactions[signo-1];

    if(oldact)
    {
        oldact->sa_handler = cur_sigaction->sa_handler;
        oldact->sa_flags = cur_sigaction->sa_flags;
        oldact->sa_mask = cur_sigaction->sa_mask;
    }

    if(act)
    {
        cur_sigaction->sa_handler = act->sa_handler;
        cur_sigaction->sa_flags = act->sa_flags;
        cur_sigaction->sa_mask = act->sa_mask;
    }
    return 0;
}

void sys_rt_sigreturn(void)
{
    struct Process *proc = getcpu()->cur_proc;
    if(!proc->do_signal) return;

    // 恢复进入signal处理函数之前的trap状态
    struct trap_context *frame = proc->tcontext.sp;
    memcpy(&proc->tcontext, frame, sizeof(struct trap_context));
    proc->tcontext.sepc -= 4; // 因为sys_rt_sigreturn返回后还会加4
    proc->do_signal = 0;
}

// 处理进程接收到的信号
int signal_handler(struct Process *proc, struct sigpending *pending)
{
    struct sigaction *action = &proc->sigactions[pending->signo-1];
    int rt = 0;
    if(pending->signo == SIGKILL || pending->signo == SIGTERM)
        proc->receive_kill--;
    if(action->sa_handler == SIG_DFL) // 采取默认方式
    {
        void (*dfl_handler)(struct sigpending *pending);
        dfl_handler = dfl_sig_handler[pending->signo - 1];
        dfl_handler(pending);
    } else
    {
        make_sigframe(proc, pending->signo, action->sa_handler);
        proc->do_signal = 1;
        rt = 1;
    }
    free_sigpending(pending);
    return rt;
}
