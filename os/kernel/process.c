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
    for(i = 0; i < PROC_N; i++)
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
    init_list(&proc->status_list_node);
    init_list(&proc->child_list_node);
    init_list(&proc->child_list);
    init_spinlock(&proc->lock, "proc_lock");
    init_semephonre(&proc->signal, "proc_sem", 0);
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

#include "xfat.h"
#include "panic.h"

// loop: 0x01, 0xA0
// loop and wfi 0x73, 0x00, 0x50, 0x10, 0xF5, 0xBF
uchar init_data[] = {
    0x73, 0x00, 0x50, 0x10, 0xF5, 0xBF
};

uchar pdata1[] = {
    0x37, 0xE8, 0xF5, 0x05, 0x13, 0x05, 0x10, 0x06, 0x81, 0x45, 0x01, 0x46, 0x81, 0x46, 0x01, 0x47, 0x81, 0x47, 0x81, 0x48, 0x73, 0x00, 0x00, 0x00, 0x93, 0x07, 0x18, 0x10, 0xFD, 0x17, 0xFD, 0xFF, 0xD5, 0xB7
};

uchar pdata2[] = { // app.c byte codes

};

struct Process* init_pid0(void)
{
    struct Process *p = new_proc();
    p->pg_t = k_pg_t;

    p->tcontext.k_sp = (uint64)(p->k_stack) + PAGE_SIZE;
    p->tcontext.hartid = 0;
    p->tcontext.sp = PAGE_SIZE; // 用户栈
    p->tcontext.sstatus = get_sstatus();

    p->pcontext.ra = (uint64)trap_ret;
    struct trap_context *t = &p->tcontext;
    
    uint64 sstatus = t->sstatus;
    sstatus |= SPP;     // init进程与普通用户进程不同，其为S特权级来执行wfi指令，SPP置为S特权级为1，用户进程置为0
    sstatus |= SPIE;
    t->sstatus = sstatus;

    uint64 *pg3 = kmalloc(PAGE_SIZE);
    p->tcontext.sepc = pg3; // init为内核进程，与内核共用页表，可以直接从内核地址开始执行

    int i;
    uchar *pt=pg3;
    for(i=0; i < sizeof(init_data); i++)
    {
        pt[i] = init_data[i];
    }

    lock(&list_lock);
    add_before(&ready_list, &p->status_list_node);
    unlock(&list_lock);
    return p;
}

struct Process* init_process(uchar *pdata, int len)
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
    p->tcontext.sepc = 0x1000; // 用户程序从这个地址开始执行
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
    
    uint64 *pg3 = kmalloc(PAGE_SIZE*4); // 第一页为栈，第二页开始为代码段
    memset(pg3, 0, PAGE_SIZE*4);
    pg2[0] = PA2PTE((uint64)pg3 + PAGE_SIZE * 0 - PV_OFFSET) | PTE_V | PTE_R | PTE_W  | PTE_X | PTE_U;
    pg2[1] = PA2PTE((uint64)pg3 + PAGE_SIZE * 1 - PV_OFFSET) | PTE_V | PTE_R | PTE_W  | PTE_X | PTE_U;
    pg2[2] = PA2PTE((uint64)pg3 + PAGE_SIZE * 2 - PV_OFFSET) | PTE_V | PTE_R | PTE_W  | PTE_X | PTE_U;
    pg2[3] = PA2PTE((uint64)pg3 + PAGE_SIZE * 3 - PV_OFFSET) | PTE_V | PTE_R | PTE_W  | PTE_X | PTE_U;

    int i;
    uchar *pt=(uint64)pg3 + PAGE_SIZE;
    for(i=0; i < len; i++)
    {
        pt[i] = pdata[i];
    }
    lock(&list_lock);
    add_before(&ready_list, &p->status_list_node);
    unlock(&list_lock);
    return p;
}

void free_process_mem(struct Process *proc)
{
    kfree(proc->k_stack);
    uint64 *pg0, *pg1, *pg2, *pg3;
    int i, j, k;
    pg0 = proc->pg_t;
    for(i = 0; i < 256; i++)
    {
        if(pg0[i] & PTE_V)
        {
            pg1 = PTE2PA(pg0[i]) + PV_OFFSET;
            if(!(pg0[i] & PTE_R) && !(pg0[i] & PTE_W) && !(pg0[i] & PTE_X)) // 指向下一级
            {
                for(j = 0; j < 256; j++)
                {
                    if(pg1[j] & PTE_V)
                    {
                        pg2 = PTE2PA(pg1[j]) + PV_OFFSET;
                        if(!(pg1[j] & PTE_R) && !(pg1[j] & PTE_W) && !(pg1[j] & PTE_X)) // 指向下一级
                        {
                            for(k = 0; k < 256; k++)
                            {
                                if(pg2[k] & PTE_V)
                                {
                                    pg3 = PTE2PA(pg2[k]) + PV_OFFSET;
                                    kfree(pg3);
                                }
                            }
                        }
                        kfree(pg2);
                    }
                }
            }
            kfree(pg1);
        }
    }
    kfree(pg0);
    free_pid(proc->pid);
}

void free_process_struct(struct Process *proc)
{
    kfree(proc);
}

uint64 getpagecnt(uint64 size)
{
    if((size & 0xfff)==0)
        return size>>12;
    else return ((size>>12)+1);
}

#include "xfat.h"

struct Process* test_app(char *path)
{
    xfile_t file;
    if(xfile_open(&file, path) < 0)
    {
        printk("open file error");
        return NULL;
    }
    uint64 pagecnt = getpagecnt(file.size);
    uchar *page = kmalloc(PAGE_SIZE * (pagecnt+1));
    memset(page, 0, pagecnt+1);
    xfile_read(page+PAGE_SIZE, 1, file.size, &file);

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
    p->tcontext.sepc = 0x1000; // 用户程序从这个地址开始执行
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
    
    uint64 *pg3 = page; // 第一页为栈，第二页开始为代码段
    int i;
    for(i = 0; i <= pagecnt; i++)
    {
        pg2[i] = PA2PTE((uint64)pg3 + PAGE_SIZE * i - PV_OFFSET) | PTE_V | PTE_R | PTE_W  | PTE_X | PTE_U;
    }
    lock(&list_lock);
    add_before(&ready_list, &p->status_list_node);
    unlock(&list_lock);
    return p;
}

void user_init(int hartid)
{
    // 允许S态读写U态内存，K210与新版RISCV不一样
    #ifdef _K210
    uint64 sstatus = get_sstatus();
    sstatus &= (~SUM);
    set_sstatus(sstatus);
    #else
    uint64 sstatus = get_sstatus();
    sstatus |= SUM;
    set_sstatus(sstatus);
    #endif

    struct Process *p0 = init_pid0(); // 初始化init 0号进程
    struct Process *app = test_app("/root/fork.bin");
    if(app)
    {
        #ifdef _DEBUG
        printk("app create success\n");
        #endif
        app->parent = p0;
        add_before(&p0->child_list, &app->child_list_node);
    }

    #ifdef _DEBUG
    printk("user init success\n");
    #endif
}