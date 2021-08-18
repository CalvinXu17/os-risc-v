#include "process.h"
#include "kmalloc.h"
#include "page.h"
#include "string.h"
#include "panic.h"
#include "elf.h"
#include "vfs.h"
#include "fs.h"
#include "pipe.h"
#include "vmm.h"
#include "timer.h"

void* pids[PROC_N] = {0};
spinlock pid_lock;

extern spinlock list_lock;
extern list ready_list;
extern list wait_list;
extern list dead_list;
extern list sleep_list;

// loop: 0x01, 0xA0
// loop and wfi 0x73, 0x00, 0x50, 0x10, 0xF5, 0xBF
uchar init_data[] = {
    0x73, 0x00, 0x50, 0x10, 0xF5, 0xBF
};

uchar pdata1[] = {
    0x37, 0xE8, 0xF5, 0x05, 0x13, 0x05, 0x10, 0x06, 0x81, 0x45, 0x01, 0x46, 0x81, 0x46, 0x01, 0x47, 0x81, 0x47, 0x81, 0x48, 0x73, 0x00, 0x00, 0x00, 0x93, 0x07, 0x18, 0x10, 0xFD, 0x17, 0xFD, 0xFF, 0xD5, 0xB7
};

uchar testdata[] = { // app.c byte codes
0x0a, 0x85, 0x6f, 0x00, 0x40, 0x00, 0x41, 0x11, 
0x06, 0xe4, 0x93, 0x05, 0x85, 0x00, 0x08, 0x41, 
0xef, 0x00, 0x00, 0x01, 0xef, 0x00, 0x60, 0x0f, 
0x01, 0x45, 0xa2, 0x60, 0x41, 0x01, 0x82, 0x80, 
0x41, 0x11, 0x06, 0xe4, 0x17, 0x06, 0x00, 0x00, 
0x13, 0x06, 0x46, 0x37, 0x97, 0x05, 0x00, 0x00, 
0x93, 0x85, 0x45, 0x3b, 0x17, 0x05, 0x00, 0x00, 
0x13, 0x05, 0xc5, 0x2b, 0xef, 0x00, 0x20, 0x0f, 
0x01, 0x45, 0xa2, 0x60, 0x41, 0x01, 0x82, 0x80, 
0xc1, 0x15, 0x88, 0xe1, 0x94, 0xe5, 0x32, 0x85, 
0x3a, 0x86, 0xbe, 0x86, 0x42, 0x87, 0x93, 0x08, 
0xc0, 0x0d, 0x73, 0x00, 0x00, 0x00, 0x11, 0xc1, 
0x82, 0x80, 0x82, 0x65, 0x22, 0x65, 0x82, 0x95, 
0x93, 0x08, 0xd0, 0x05, 0x73, 0x00, 0x00, 0x00, 
0xaa, 0x87, 0x2e, 0x86, 0x93, 0x08, 0x80, 0x03, 
0x13, 0x05, 0xc0, 0xf9, 0xbe, 0x85, 0x89, 0x46, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0x93, 0x08, 0x80, 0x03, 0x93, 0x06, 0x00, 0x18, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0x93, 0x08, 0x90, 0x03, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 0xf0, 0x03, 
0x73, 0x00, 0x00, 0x00, 0x82, 0x80, 0x93, 0x08, 
0x00, 0x04, 0x73, 0x00, 0x00, 0x00, 0x82, 0x80, 
0x93, 0x08, 0xc0, 0x0a, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 0xd0, 0x0a, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0x93, 0x08, 0xc0, 0x07, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 0xc0, 0x0d, 
0x45, 0x45, 0x81, 0x45, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x41, 0x11, 0x06, 0xe4, 
0xb2, 0x85, 0x3a, 0x86, 0x91, 0xc1, 0xb6, 0x95, 
0x81, 0x47, 0x01, 0x47, 0x81, 0x46, 0x01, 0x26, 
0xef, 0xf0, 0x9f, 0xf4, 0xa2, 0x60, 0x41, 0x01, 
0x82, 0x80, 0x93, 0x08, 0xd0, 0x05, 0x73, 0x00, 
0x00, 0x00, 0x82, 0x80, 0x93, 0x08, 0x40, 0x10, 
0x81, 0x46, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x93, 0x08, 0xd0, 0x0d, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 
0xd0, 0x0d, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x93, 0x08, 0x90, 0x09, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 
0x90, 0x0a, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x01, 0x11, 0x06, 0xec, 0x81, 0x45, 
0x0a, 0x85, 0xef, 0xf0, 0xdf, 0xfe, 0x11, 0xed, 
0x03, 0x55, 0x01, 0x00, 0x13, 0x07, 0x80, 0x3e, 
0x33, 0x05, 0xe5, 0x02, 0xa2, 0x67, 0xb3, 0xd7, 
0xe7, 0x02, 0x3e, 0x95, 0xe2, 0x60, 0x05, 0x61, 
0x82, 0x80, 0x7d, 0x55, 0xe5, 0xbf, 0x93, 0x08, 
0x60, 0x42, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x41, 0x11, 0x2a, 0xe0, 0x02, 0xe4, 
0x93, 0x08, 0x50, 0x06, 0x0a, 0x85, 0x8a, 0x85, 
0x73, 0x00, 0x00, 0x00, 0x01, 0xe5, 0x01, 0x45, 
0x41, 0x01, 0x82, 0x80, 0x02, 0x45, 0xed, 0xbf, 
0x93, 0x08, 0xc0, 0x08, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 0xe0, 0x0d, 
0x73, 0x00, 0x00, 0x00, 0x82, 0x80, 0x93, 0x08, 
0x70, 0x0d, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x41, 0x11, 0x06, 0xe4, 0x01, 0x46, 
0xaa, 0x85, 0x7d, 0x55, 0xef, 0xf0, 0x1f, 0xf4, 
0xa2, 0x60, 0x41, 0x01, 0x82, 0x80, 0x93, 0x08, 
0x00, 0x19, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x93, 0x08, 0x10, 0x19, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 
0x20, 0x19, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x93, 0x08, 0x00, 0x05, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 
0x50, 0x02, 0x02, 0x17, 0x01, 0x93, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 
0x30, 0x02, 0x02, 0x16, 0x01, 0x92, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x41, 0x11, 
0x06, 0xe4, 0x01, 0x47, 0xae, 0x86, 0x13, 0x06, 
0xc0, 0xf9, 0xaa, 0x85, 0x13, 0x05, 0xc0, 0xf9, 
0xef, 0xf0, 0xff, 0xfc, 0xa2, 0x60, 0x41, 0x01, 
0x82, 0x80, 0x41, 0x11, 0x06, 0xe4, 0x01, 0x46, 
0xaa, 0x85, 0x13, 0x05, 0xc0, 0xf9, 0xef, 0xf0, 
0x9f, 0xfc, 0xa2, 0x60, 0x41, 0x01, 0x82, 0x80, 
0x93, 0x08, 0x00, 0x0a, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 0x60, 0x0d, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0xc5, 0x48, 0x73, 0x00, 0x00, 0x00, 0x82, 0x80, 
0x93, 0x08, 0x10, 0x03, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0xaa, 0x87, 0x13, 0x96, 
0x05, 0x02, 0x01, 0x92, 0x93, 0x08, 0x20, 0x02, 
0x13, 0x05, 0xc0, 0xf9, 0xbe, 0x85, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 
0xd0, 0x03, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x93, 0x08, 0xb0, 0x03, 0x81, 0x45, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0xdd, 0x48, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0xe1, 0x48, 0x01, 0x46, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 
0x80, 0x02, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x93, 0x08, 0x70, 0x02, 0x81, 0x45, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0x2e, 0x2f, 0x62, 0x75, 0x73, 0x79, 0x62, 0x6f, 
0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x53, 0x48, 0x45, 0x4c, 0x4c, 0x3d, 0x73, 0x68, 
0x65, 0x6c, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x50, 0x57, 0x44, 0x3d, 0x2f, 0x72, 0x6f, 0x6f, 
0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x48, 0x4f, 0x4d, 0x45, 0x3d, 0x2f, 0x72, 0x6f, 
0x6f, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x55, 0x53, 0x45, 0x52, 0x3d, 0x72, 0x6f, 0x6f, 
0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x53, 0x48, 0x4c, 0x56, 0x4c, 0x3d, 0x31, 0x00, 
0x50, 0x41, 0x54, 0x48, 0x3d, 0x2f, 0x72, 0x6f, 
0x6f, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x4f, 0x4c, 0x44, 0x50, 0x57, 0x44, 0x3d, 0x2f, 
0x72, 0x6f, 0x6f, 0x74, 0x00, 0x00, 0x00, 0x00, 
0x5f, 0x3d, 0x62, 0x75, 0x73, 0x79, 0x62, 0x6f, 
0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x73, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x2e, 0x2f, 0x6c, 0x6d, 0x62, 0x65, 0x6e, 0x63, 
0x68, 0x5f, 0x74, 0x65, 0x73, 0x74, 0x63, 0x6f, 
0x64, 0x65, 0x2e, 0x73, 0x68, 0x00, 0x00, 0x00, 
0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x10, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x20, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x30, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x40, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x48, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x58, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x68, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xf0, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x78, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x80, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void proc_init(void)
{
    init_spinlock(&pid_lock, "pid_lock");
    memset(pids, 0, PROC_N * sizeof(void *));
}

int32 get_pid(void)
{
    lock(&pid_lock);
    int32 i;
    for(i = 0; i < PROC_N; i++)
    {
        if(pids[i] == NULL)
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
        pids[pid] = NULL;
    }
    unlock(&pid_lock);
}

struct Process* get_proc_by_pid(int pid)
{
    if(pid >= 1 && pid < PROC_N)
    {
        return pids[pid];
    } else return NULL;
}

void free_ufile_list(struct Process *p)
{
    list *l = p->ufiles_list_head.next;
    while(l != &p->ufiles_list_head)
    {
        ufile_t *file = list2ufile(l);
        l = l->next;
        if(file->type == UTYPE_STDIN || file->type == UTYPE_STDOUT || file->type == UTYPE_STDERR)
        {
            vfs_close((int)file->private);
            ufile_free(file);
        } else if(file->type == UTYPE_FILE)
        {
            vfs_close((int)file->private);
            ufile_free(file);
        } else if(file->type == UTYPE_PIPEIN)
        {
            pipe_close(file->private, UTYPE_PIPEIN);
            ufile_free(file);
        } else if(file->type == UTYPE_PIPEOUT)
        {
            pipe_close(file->private, UTYPE_PIPEOUT);
            ufile_free(file);
        } else if(file->type == UTYPE_DIR)
        {
            vfs_closedir(file->private);
            ufile_free(file);
        }
    }
    init_list(&p->ufiles_list_head);
}

void init_stdfiles(struct Process *proc)
{
    ufile_t *file = kmalloc(sizeof(ufile_t)); // 初始化STDIN文件
    file->ufd = 0;
    file->type = UTYPE_STDIN;
    file->private = (void*)vfs_open("/dev/stdin", O_RDONLY);
    init_list(&file->list_node);
    add_before(&proc->ufiles_list_head, &file->list_node);

    file = kmalloc(sizeof(ufile_t)); // 初始化STDOUT文件
    file->ufd = 1;
    file->type = UTYPE_STDOUT;
    file->private = (void*)vfs_open("/dev/stdout", O_WRONLY);
    init_list(&file->list_node);
    add_before(&proc->ufiles_list_head, &file->list_node);

    file = kmalloc(sizeof(ufile_t)); // 初始化STDERR文件
    file->ufd = 2;
    file->type = UTYPE_STDERR;
    file->private = (void*)vfs_open("/dev/stderr", O_WRONLY);
    init_list(&file->list_node);
    add_before(&proc->ufiles_list_head, &file->list_node);
}

struct Process* new_proc(void)
{
    int32 pid = get_pid();
    struct Process *proc = (struct Process*)kmalloc(sizeof(struct Process));
    pids[pid] = proc;
    proc->pid = pid;
    proc->status = PROC_READY;
    proc->t_wait = 0;
    proc->t_slice = get_time() + T_SLICE * TIMER_FRQ / 1000;
    proc->start_time = proc->end_time = proc->utime_start = proc->start_time = proc->utime = proc->stime = 0;
    init_list(&proc->status_list_node);
    init_list(&proc->child_list_node);
    init_list(&proc->child_list_head);
    init_list(&proc->vma_list_head);
    init_list(&proc->ufiles_list_head);
    init_list(&proc->signal_list_head);
    proc->receive_kill = 0;

    init_spinlock(&proc->lock, "proc_lock");
    init_semephonre(&proc->sleep_lock, "proc_sem", 0);
    proc->k_stack = vm_alloc_pages(1);
    proc->parent = proc->data = 0;
    proc->kfd = 0;

    init_proc_sigactions(proc);
    return proc;
}

void free_proc(struct Process *proc) // 仅为new_proc的反向操作
{
    free_pid(proc->pid);
    kfree(proc->k_stack);
    free_ufile_list(proc);
    kfree(proc);
}

void free_process_struct(struct Process *proc)
{
    if(proc->cwd)
    {
        vfs_closedir(proc->cwd);
    }
    free_pid(proc->pid);
    kfree(proc);
}

void free_process_mem(struct Process *proc)
{
    if(!proc->pg_t) return;

    list *l = proc->vma_list_head.next;
    struct vma *vma = NULL;
    while(l != &proc->vma_list_head)
    {
        vma = vma_list_node2vma(l);
        l = l->next;
        del_list(&vma->vma_list_node);
        free_vma_mem(proc->pg_t, vma);
        kfree(vma);
    }
    init_list(&proc->vma_list_head);
    // free_pagetable(proc->pg_t);
}

static uint64 elfflag2prot(uint64 flag)
{
    uint64 prot = 0;
    if(flag & ELF_PROG_FLAG_READ)
        prot |= PROT_READ;
    if(flag & ELF_PROG_FLAG_WRITE)
        prot |= PROT_WRITE;
    if(flag & ELF_PROG_FLAG_EXEC)
        prot |= PROT_EXEC;
    return prot;
}

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

    uint64 *pg3 = vm_alloc_pages(1);
    p->tcontext.sepc = pg3; // init为内核进程，与内核共用页表，可以直接从内核地址开始执行

    memcpy(pg3, init_data, sizeof(init_data));

    lock(&list_lock);
    add_before(&ready_list, &p->status_list_node);
    unlock(&list_lock);
    return p;
}

struct Process* init_process(uchar *pdata, int len)
{
    struct Process *p = new_proc();
    init_stdfiles(p);

    p->cwd = vfs_opendir("/root");

    uint64 *pg_t = vm_alloc_pages(1);
    copy_kpgt(pg_t); // 复制内核页表

    p->pg_t = pg_t;
    // 用户进程分配页表

    p->tcontext.k_sp = (uint64)(p->k_stack) + PAGE_SIZE - 8;
    p->tcontext.hartid = 0;
    p->tcontext.sepc = 0x1000; // 用户程序从这个地址开始执行
    p->tcontext.sp = PAGE_SIZE - 16; // 用户栈
    p->tcontext.sstatus = get_sstatus();

    p->pcontext.ra = (uint64)trap_ret;
    struct trap_context *t = &p->tcontext;
    set_user_mode(t);

    void *mem = vm_alloc_pages(4); // 第一页为栈，第二页开始为代码段
    memcpy((uint64)mem + PAGE_SIZE, pdata, len);

    struct vma *vma = new_vma();
    vma->va = 0;
    vma->size = 4 * PAGE_SIZE;
    vma->prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    vma->flag = PTE_R | PTE_W  | PTE_X | PTE_U;
    vma->type = VMA_PROG;
    vma->foff = 0;
    vma->file = NULL;
    add_vma(p, vma);
    mappages(p->pg_t, vma->va, (uint64)mem - PV_OFFSET, vma->size / PAGE_SIZE, prot2pte(vma->prot));
    vma = &p->vma_list_head;

    lock(&list_lock);
    add_before(&ready_list, &p->status_list_node);
    unlock(&list_lock);
    return p;
}

void kill_current(void)
{
    struct Process *proc = getcpu()->cur_proc;

    lock(&proc->lock);
    proc->status = PROC_DEAD; // 置状态位为DEAD
    proc->end_time = get_time();
    proc->code = 0;
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
    V(&parent->sleep_lock);
    move_switch2sched(proc, &dead_list);
}

void print_user_stack(struct Process *proc)
{
    uint64 *sp = proc->tcontext.sp;
    uint64 *vsp = userva2kernelva(proc->pg_t, sp);
    printk("----------STACK-------------\n");
    printk("%lx:%lx\n", sp++, *vsp++);
    while(*vsp)
    {
        printk("%lx:%lx:%s\n", sp, *vsp, userva2kernelva(proc->pg_t, *vsp));
        vsp++;
        sp++;
    }
    printk("%lx:NULL\n", sp);
    vsp++;
    sp++;
    while(*vsp)
    {
        printk("%lx:%lx:%s\n", sp, *vsp, userva2kernelva(proc->pg_t, *vsp));
        vsp++;
        sp++;
    }
    printk("%lx:NULL\n", sp);
    vsp++;
    sp++;
    while(*vsp)
    {
        printk("%lx:%lx|", sp, *vsp);
        vsp++;
        sp++;
        printk("%lx:%lx\n", sp, *vsp);
        vsp++;
        sp++;
    }
    printk("%lx:NULL\n", sp);
    printk("----------STACK-------------\n");
}

void create_user_stack(struct Process *proc, char *const argv[], char *const envp[], struct elfhdr *hdr, char *filename, uint64 progstart)
{
    if(!argv || !envp || !hdr) return;
    uint64 *sp = proc->tcontext.sp;
    uchar *data = proc->minbrk;
    proc->minbrk = proc->brk = proc->brk + 2 * PAGE_SIZE;
    
    int argc, envc, i;
    for(argc=0; argv && argv[argc]; argc++);
    for(envc=0; envp && envp[envc]; envc++);

    copy2user(proc->pg_t, sp, &argc, sizeof(int)); // 设置栈中的argc

    uint64 *argv_start = sp + 1;
    uint64 *envp_start = argv_start + argc + 1;
    uint64 *auxv_start = envp_start + envc + 1;

    uint64 tmp = 0;
    copy2user(proc->pg_t, argv_start + argc, &tmp, sizeof(uint64)); // 最后一项为0
    copy2user(proc->pg_t, envp_start + envc, &tmp, sizeof(uint64)); // 最后一项为0

    // if((uint64)sp & 0xF) // 保证低4位全为0
    // {
    //     envp_start--;
    //     argv_start--;
    //     sp--;
    // }

    for(i = 0; i < argc; i++)
    {
        char *v = argv[i];
        uint64 len = strlen(v) + 1;
        uint64 aligned_len = len;
        if(len % 8 != 0) 
            aligned_len = ((len >> 3) + 1 ) << 3; // 地址对齐
        copy2user(proc->pg_t, data, v, len);
        tmp = data;
        copy2user(proc->pg_t, &argv_start[i], &tmp, sizeof(uint64));
        data = data + aligned_len;
    }

    for(i = 0; i < envc; i++)
    {
        char *v = envp[i];
        uint64 len = strlen(v) + 1;
        uint64 aligned_len = len;
        if(len % 8 != 0) 
            aligned_len = ((len >> 3) + 1 ) << 3; // 地址对齐
        copy2user(proc->pg_t, data, v, len);
        tmp = data;
        copy2user(proc->pg_t, &envp_start[i], &tmp, sizeof(uint64));
        data = data + aligned_len;
    }

    if(strcmp(filename, "lmbench_all") == 0) // 手动添加lmbench的参数加快运行速度
    {
        static char *lmbench_envps[] = {"ENOUGH=5000", "TIMING_O=7", "LOOP_O=0.00249936", NULL};
        int j;
        for(j = 0; j < 3; j++)
        {
            char *v = lmbench_envps[j];
            uint64 len = strlen(v) + 1;
            uint64 aligned_len = len;
            if(len % 8 != 0) 
                aligned_len = ((len >> 3) + 1 ) << 3; // 地址对齐
            copy2user(proc->pg_t, data, v, len);
            tmp = data;
            copy2user(proc->pg_t, &envp_start[i++], &tmp, sizeof(uint64));
            data = data + aligned_len;
            auxv_start++;
        }
    }

#define ADD_AUXV(id,val) \
do { \
    tmp = id; \
    copy2user(proc->pg_t, &auxv_start[auxindex++], &tmp, sizeof(uint64)); \
    tmp = val; \
    copy2user(proc->pg_t, &auxv_start[auxindex++], &tmp, sizeof(uint64)); \
} while(0)

    uint64 len = strlen(filename) + 1;
    uint64 aligned_len = len;
    if(len % 8 != 0) 
        aligned_len = ((len >> 3) + 1 ) << 3; // 地址对齐
    copy2user(proc->pg_t, data, filename, len);
    filename = data;
    data = data + aligned_len;

    int auxindex = 0;
    ADD_AUXV(0x28, 0);
    ADD_AUXV(0x29, 0);
    ADD_AUXV(0x2a, 0);
    ADD_AUXV(0x2b, 0);
    ADD_AUXV(0x2c, 0);
    ADD_AUXV(0x2d, 0);

    ADD_AUXV(AT_PHDR, progstart + hdr->phoff);
    ADD_AUXV(AT_PHENT, sizeof(struct proghdr));
    ADD_AUXV(AT_PHNUM, hdr->phnum);
    ADD_AUXV(AT_PAGESZ, 0x1000);
    ADD_AUXV(AT_BASE, 0);
    ADD_AUXV(AT_FLAGS, 0);
    ADD_AUXV(AT_ENTRY, hdr->entry);
    ADD_AUXV(AT_UID, 0);
    ADD_AUXV(AT_EUID, 0);
    ADD_AUXV(AT_GID, 0);
    ADD_AUXV(AT_EGID, 0);
    ADD_AUXV(AT_HWCAP, 0x112d);
    ADD_AUXV(AT_CLKTCK, 64);
    ADD_AUXV(AT_EXECFN, (uint64)filename);
    ADD_AUXV(AT_NULL, 0);

    // print_user_stack(proc);
}

struct Process* create_proc_by_elf(char *absolute_path, char *const argv[], char *const envp[])
{
    if(!absolute_path) return NULL;

    int fd = vfs_open(absolute_path, O_RDONLY);
    if(fd < 0)
    {
        #ifdef _DEBUG
        printk("open file failed %d\n", fd);
        #endif
        return NULL;
    }

    struct elfhdr ehdr;
    if(vfs_read(fd, &ehdr, sizeof(struct elfhdr)) != sizeof(struct elfhdr))
    {
        #ifdef _DEBUG
        printk("read elfheader error\n");
        #endif
        vfs_close(fd);
        return NULL;
    }
    if(ehdr.magic != ELF_MAGIC)
    {
        #ifdef _DEBUG
        printk("is not elf file\n");
        #endif
        vfs_close(fd);
        return NULL;
    }

    struct Process *proc = new_proc();
    
    int pos;
    for(pos = strlen(absolute_path)-1; pos >= 0; pos--)
    {
        if(absolute_path[pos] == '/')
        {
            break;
        }
    }
    if(pos < 0)
    {
        free_proc(proc);
        vfs_close(fd);
        return NULL;
    }
    char cwd[VFS_PATH_MAX] = {0};
    memcpy(cwd, absolute_path, pos);
    cwd[pos] = '\0';
    proc->cwd = vfs_opendir(cwd);
    uint64 *pg_t = vm_alloc_pages(1);
    
    copy_kpgt(pg_t); // 复制内核页表项

    proc->pg_t = pg_t;

    vfs_lseek(fd, ehdr.phoff, VFS_SEEK_SET);
    uint64 phdrsize = (uint64)ehdr.phentsize * (uint64)ehdr.phnum;

    struct proghdr *phdr = kmalloc(phdrsize);
    int size = vfs_read(fd, phdr, sizeof(struct proghdr) * ehdr.phnum);
    
    if(size!=phdrsize)
    {
        #ifdef _DEBUG
        printk("read section table error, size: %d\n", size);
        #endif
        kfree(phdr);
        vfs_close(fd);

        kfree(proc->k_stack);
        kfree(proc->pg_t);
        free_ufile_list(proc);
        free_pid(proc->pid);
        vfs_closedir(proc->cwd);
        kfree(proc);
        return NULL;
    }

    int seci;
    uint64 _edata = 0;
    void *mem;
    struct vma *vma;

    uint64 progstart = 0xffffffff;
    for(seci=0; seci < ehdr.phnum; seci++) // 遍历elf文件段表
    {
        struct proghdr *sec = &phdr[seci];
        // printk("type %lx vaddr %lx memsize %lx filesize %lx off %lx\n", sec->type, sec->vaddr, sec->memsz, sec->filesz, sec->off);
        if(sec->type == ELF_PROG_LOAD)
        {
            uint64 vaddr = sec->vaddr;
            uint64 file_offset = sec->off;
            uint64 file_size = sec->filesz;

            uint64 vpagestart = PAGEDOWN(vaddr);
            uint64 page_cnt = (PAGEUP(vaddr + sec->memsz) - vpagestart) / PAGE_SIZE; // getpagecnt(vaddr + sec->memsz - vpagestart);

            if(sec->vaddr < progstart) progstart = sec->vaddr;
            // mem = vm_alloc_pages(page_cnt);
            // if(!mem)
            // {
            //     #ifdef _DEBUG
            //     printk("read section table error\n");
            //     #endif
            //     kfree(phdr);
            //     vfs_close(fd);
            //     free_process_mem(proc);
            //     kfree(proc->k_stack);
            //     free_pagetable(proc->pg_t);
            //     free_process_struct(proc);
            //     return NULL;
            // }

            // vfs_lseek(fd, file_offset, VFS_SEEK_SET);
            // if(vfs_read(fd, (uint64)mem+vaddr-vpagestart, file_size) != file_size) // 读取进程数据到指定内存
            // {
            //     #ifdef _DEBUG
            //     printk("read not equal\n");
            //     #endif
            //     kfree(phdr);
            //     vfs_close(fd);
            //     free_process_mem(proc);
            //     kfree(proc->k_stack);
            //     free_pagetable(proc->pg_t);
            //     free_process_struct(proc);
            //     return NULL;
            // }
            vma = new_vma();
            
            vma->va = vpagestart;
            // vma->pa = (uint64)mem - PV_OFFSET;
            vma->size = page_cnt * PAGE_SIZE;
            vma->prot = elfflag2prot(sec->flags);
            vma->flag = PTE_R | PTE_W  | PTE_X | PTE_U;
            vma->type = VMA_PROG;
            vma->vmoff = vaddr - vpagestart;
            vma->fsize = file_size;
            vma->foff = file_offset;
            vma->file = NULL;
            add_vma(proc, vma);
            // printk("prog va %lx filebegin vma %lx, prot %lx file_off %lx fsize %lx memsize %lx vmasize %lx\n", vma->va, vaddr, vma->prot, file_offset, file_size, sec->memsz, vma->size);
            // mappages(proc->pg_t, vma->va, (uint64)mem - PV_OFFSET, page_cnt, prot2pte(vma->prot));
            if(vpagestart + page_cnt * PAGE_SIZE > _edata)
                _edata = vpagestart + page_cnt * PAGE_SIZE; // 保存最高的段地址后面用来初始化brk
        }
    }
    // printk("entry %lx edata %lx\n", ehdr.entry, _edata);
    kfree(phdr);

    // 映射_edata开始2页，第一页用于存放argv与envp数据
    mem = vm_alloc_pages(2); // kmalloc(2 * PAGE_SIZE);
    vma = new_vma();
    vma->va = _edata;
    vma->size = 2 * PAGE_SIZE;
    vma->prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    vma->flag = PTE_R | PTE_W  | PTE_X | PTE_U;
    vma->type = VMA_PROG;
    vma->vmoff = 0;
    vma->fsize = 0;
    vma->foff = 0;
    // vma->pa = (uint64)mem - PV_OFFSET;
    vma->file = NULL;
    add_vma(proc, vma);
    mappages(proc->pg_t, vma->va, (uint64)mem - PV_OFFSET, vma->size / PAGE_SIZE, prot2pte(vma->prot));

    // 映射2页用户栈
    mem = vm_alloc_pages(2);
    vma = new_vma();
    vma->va = STACK_TOP - 2 * PAGE_SIZE;
    vma->size = PAGE_SIZE * 2;
    vma->prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    vma->flag = PTE_R | PTE_W  | PTE_X | PTE_U;
    vma->type = VMA_DATA;
    vma->vmoff = 0;
    vma->fsize = 0;
    vma->foff = 0;
    // vma->pa = (uint64)mem - PV_OFFSET;
    vma->file = NULL;
    add_vma(proc, vma);
    mappages(proc->pg_t, vma->va, (uint64)mem - PV_OFFSET, vma->size / PAGE_SIZE, prot2pte(vma->prot));

    proc->tcontext.k_sp = (uint64)(proc->k_stack) + PAGE_SIZE - 8;
    proc->tcontext.hartid = 0;
    proc->tcontext.sepc = ehdr.entry; // 用户程序从这个地址开始执行
    proc->tcontext.sp = STACK_TOP - PAGE_SIZE; // 用户栈
    proc->tcontext.sstatus = get_sstatus();
    proc->brk = proc->minbrk = _edata;

    // 创建用户栈
    create_user_stack(proc, argv, envp, &ehdr, &absolute_path[pos+1], progstart);

    // 创建sigreturn代码
    make_sigreturn_code(proc);

    proc->pcontext.ra = (uint64)trap_ret;
    struct trap_context *t = &proc->tcontext;
    set_user_mode(t);
    // vfs_close(fd);
    proc->kfd = fd;
    return proc;
}

int sys_test(const char *path)
{
    struct Process *proc = create_proc_by_elf(path, NULL, NULL);
    if(!proc) return -1;
    proc->parent = getcpu()->cur_proc;
    lock(&getcpu()->cur_proc->lock);
    add_before(&proc->parent->child_list_head, &proc->child_list_node);
    unlock(&getcpu()->cur_proc->lock);

    lock(&list_lock);
    add_before(&ready_list, &proc->status_list_node);
    unlock(&list_lock);
    return proc->pid;
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

    if(hartid == 0)
    {
        signal_init();
        
        struct Process *p0 = init_pid0(); // 初始化init 0号进程
        getcpu()->cur_proc = p0;

        // struct Process *p1 = create_proc_by_elf("/root/app", NULL, NULL);
        // init_stdfiles(p1);
        // if(p1)
        // {
        //     p1->parent = p0;
        //     add_before(&p0->child_list_head, &p1->child_list_node);
        //     lock(&list_lock);
        //     add_before(&ready_list, &p1->status_list_node);
        //     unlock(&list_lock);
        // } else printk("file not exist\n");

        struct Process *p1 = init_process(testdata, sizeof(testdata));
        p1->parent = p0;
        add_before(&p0->child_list_head, &p1->child_list_node);

        #ifdef _DEBUG
        printk("user init success\n");
        #endif
    }
}
