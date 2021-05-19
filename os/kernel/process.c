#include "process.h"
#include "kmalloc.h"
#include "page.h"
#include "string.h"

uchar pids[PROC_N] = {0};
spinlock pid_lock;

extern spinlock list_lock;
extern list ready_list;
extern list wait_list;
extern list dead_list;
extern list sleep_list;

void proc_init(void)
{
    init_spinlock(&pid_lock, "pid_lock");
    memset(pids, 0, PROC_N);
}

int32 get_pid(void)
{
    lock(&pid_lock);
    int32 i;
    for(i = 1; i < PROC_N; i++)
    {
        if(pids[i] == 0)
        {
            pids[i] = 1;
            unlock(&pid_lock);
            return i;
        }
    }
    unlock(&pid_lock);
    return -1;
}

void free_pid(int32 pid)
{
    lock(&pid_lock);
    if(pid >= 1 && pid < PROC_N)
    {
        pids[pid] = 0;
    }
    unlock(&pid_lock);
}

struct Process* new_proc(void)
{
    int32 pid = get_pid();
    struct Process *proc = (struct Process*)kmalloc(sizeof(struct Process));
    proc->pid = pid;
    proc->status = PROC_READY;
    proc->t_wait = 0;
    proc->t_slice = T_SLICE;
    init_list(&proc->child_list);
    init_list(&proc->status_list);
    init_spinlock(&proc->lock, "proc_lock");
    proc->k_stack = (uint64*)kmalloc(PAGE_SIZE);
    proc->parent = proc->data = 0;
    return proc;
}

void free_proc(struct Process *proc)
{
    free_pid(proc->pid);
    kfree(proc->k_stack);
    kfree(proc);
}

extern void trap_ret(void);

#include "xfat.h"
#include "panic.h"

uchar pdata[] = {
    0x37, 0xE8, 0xF5, 0x05, 0x13, 0x05, 0x10, 0x06, 0x81, 0x45, 0x01, 0x46, 0x81, 0x46, 0x01, 0x47, 0x81, 0x47, 0x81, 0x48, 0x73, 0x00, 0x00, 0x00, 0x93, 0x07, 0x18, 0x10, 0xFD, 0x17, 0xFD, 0xFF, 0xD5, 0xB7
};

void init_process(void)
{
    struct Process *p = new_proc();

    uint64 *pg_t = kmalloc(PAGE_SIZE);
    memset(pg_t, 0, PAGE_SIZE);
    pg_t[508] = k_pg_t[508];
    pg_t[509] = k_pg_t[509];
    pg_t[510] = k_pg_t[510]; // 复制内核页表

    p->pg_t = pg_t;
    // 用户进程分配页表

    p->tcontext.k_sp = (uint64)(p->k_stack) + PAGE_SIZE;
    p->tcontext.hartid = 0;
    p->tcontext.sepc = 0; // 用户程序从这个地址开始执行
    p->tcontext.sp = PAGE_SIZE; // 用户栈
    p->tcontext.sstatus = get_sstatus();

    p->pcontext.ra = (uint64)trap_ret;
    // p->pcontext.sp = (uint64)p->k_stack + PAGE_SIZE;
    struct trap_context *t = &p->tcontext;
    set_user_mode(t);

    uint64 *pg1 = kmalloc(PAGE_SIZE);
    memset(pg1, 0, PAGE_SIZE);
    pg_t[0] = PA2PTE((uint64)pg1 - PV_OFFSET) | PTE_V;

    uint64 *pg2 = kmalloc(PAGE_SIZE);
    memset(pg2, 0, PAGE_SIZE);
    pg1[0] = PA2PTE((uint64)pg2 - PV_OFFSET) | PTE_V;
    

    uint64 *pg3 = kmalloc(PAGE_SIZE);
    memset(pg3, 0, PAGE_SIZE);
    pg2[0] = PA2PTE((uint64)pg3 - PV_OFFSET) | PTE_V | PTE_W | PTE_R | PTE_X | PTE_U;
    int i;
    uchar *pd=pg3;
    for(i=0; i < sizeof(pdata); i++)
    {
        pd[i] = pdata[i];
    }

    lock(&list_lock);
    add_before(&ready_list, &p->status_list);
    unlock(&list_lock);
}
void user_init(int hartid)
{
    // 允许S态读写U态内存，K210与新版RISCV不一样
    #ifdef _K210
    uint64 sstatus = get_sstatus();
    sstatus |= SUM;
    set_sstatus(sstatus);
    #else
    uint64 sstatus = get_sstatus();
    sstatus &= (~SUM);
    set_sstatus(sstatus);
    #endif

    init_process();
    init_process();
    init_process();
    #ifdef _DEBUG
    printk("user init success\n");
    #endif
}