#include "process.h"
#include "kmalloc.h"
#include "page.h"
#include "string.h"
#include "panic.h"
#include "elf.h"
#include "vfs.h"
#include "fs.h"
#include "pipe.h"

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

void free_ufile_list(struct Process *p)
{
    list *l = p->ufiles_list.next;
    int pipn = 0;
    while(l != &p->ufiles_list)
    {
        ufile_t *file = list2ufile(l);
        l = l->next;

        if(file->type == UTYPE_STDIN || file->type == UTYPE_STDOUT)
        {
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
        }
    }
}

struct Process* new_proc(void)
{
    int32 pid = get_pid();
    struct Process *proc = (struct Process*)kmalloc(sizeof(struct Process));
    proc->pid = pid;
    proc->status = PROC_READY;
    proc->t_wait = 0;
    proc->t_slice = T_SLICE;
    proc->start_time = proc->end_time = proc->utime_start = proc->start_time = proc->utime = proc->stime = 0;
    init_list(&proc->status_list_node);
    init_list(&proc->child_list_node);
    init_list(&proc->child_list);

    init_list(&proc->ufiles_list);

    ufile_t *file = kmalloc(sizeof(ufile_t)); // 初始化STDIN文件
    file->ufd = 0;
    file->type = UTYPE_STDIN;
    init_list(&file->list_node);
    add_before(&proc->ufiles_list, &file->list_node);

    file = kmalloc(sizeof(ufile_t)); // 初始化STDOUT文件
    file->ufd = 1;
    file->type = UTYPE_STDOUT;
    init_list(&file->list_node);
    add_before(&proc->ufiles_list, &file->list_node);

    init_spinlock(&proc->lock, "proc_lock");
    init_semephonre(&proc->signal, "proc_sem", 0);
    proc->k_stack = (uint64*)kmalloc(PAGE_SIZE);
    proc->parent = proc->data = 0;
    return proc;
}

void free_proc(struct Process *proc) // 仅为new_proc的反向操作
{
    free_pid(proc->pid);
    kfree(proc->k_stack);
    free_ufile_list(proc);
    kfree(proc);
}

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
0xef, 0x00, 0xa0, 0x01, 0xef, 0x00, 0x80, 0x11, 
0x01, 0x45, 0xa2, 0x60, 0x41, 0x01, 0x82, 0x80, 
0x81, 0x48, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x01, 0x11, 0x06, 0xec, 0x22, 0xe8, 
0x26, 0xe4, 0x17, 0x04, 0x00, 0x00, 0x13, 0x04, 
0x64, 0x40, 0x97, 0x14, 0x00, 0x00, 0x93, 0x84, 
0xe4, 0xbf, 0x21, 0xa0, 0x21, 0x04, 0x63, 0x0c, 
0x94, 0x00, 0x08, 0x60, 0xef, 0xf0, 0x5f, 0xfd, 
0xe3, 0x5a, 0xa0, 0xfe, 0x01, 0x46, 0x81, 0x45, 
0xef, 0x00, 0xe0, 0x0d, 0xe5, 0xb7, 0x01, 0x45, 
0xe2, 0x60, 0x42, 0x64, 0xa2, 0x64, 0x05, 0x61, 
0x82, 0x80, 0xc1, 0x15, 0x88, 0xe1, 0x94, 0xe5, 
0x32, 0x85, 0x3a, 0x86, 0xbe, 0x86, 0x42, 0x87, 
0x93, 0x08, 0xc0, 0x0d, 0x73, 0x00, 0x00, 0x00, 
0x11, 0xc1, 0x82, 0x80, 0x82, 0x65, 0x22, 0x65, 
0x82, 0x95, 0x93, 0x08, 0xd0, 0x05, 0x73, 0x00, 
0x00, 0x00, 0xaa, 0x87, 0x2e, 0x86, 0x93, 0x08, 
0x80, 0x03, 0x13, 0x05, 0xc0, 0xf9, 0xbe, 0x85, 
0x89, 0x46, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x93, 0x08, 0x80, 0x03, 0x93, 0x06, 
0x00, 0x18, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x93, 0x08, 0x90, 0x03, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 
0xf0, 0x03, 0x73, 0x00, 0x00, 0x00, 0x82, 0x80, 
0x93, 0x08, 0x00, 0x04, 0x73, 0x00, 0x00, 0x00, 
0x82, 0x80, 0x93, 0x08, 0xc0, 0x0a, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 
0xd0, 0x0a, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x93, 0x08, 0xc0, 0x07, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 
0xc0, 0x0d, 0x45, 0x45, 0x81, 0x45, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x41, 0x11, 
0x06, 0xe4, 0xb2, 0x85, 0x3a, 0x86, 0x91, 0xc1, 
0xb6, 0x95, 0x81, 0x47, 0x01, 0x47, 0x81, 0x46, 
0x01, 0x26, 0xef, 0xf0, 0x9f, 0xf4, 0xa2, 0x60, 
0x41, 0x01, 0x82, 0x80, 0x93, 0x08, 0xd0, 0x05, 
0x73, 0x00, 0x00, 0x00, 0x82, 0x80, 0x93, 0x08, 
0x40, 0x10, 0x81, 0x46, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 0xd0, 0x0d, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0x93, 0x08, 0xd0, 0x0d, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 0x90, 0x09, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0x93, 0x08, 0x90, 0x0a, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x01, 0x11, 0x06, 0xec, 
0x81, 0x45, 0x0a, 0x85, 0xef, 0xf0, 0xdf, 0xfe, 
0x11, 0xed, 0x03, 0x55, 0x01, 0x00, 0x13, 0x07, 
0x80, 0x3e, 0x33, 0x05, 0xe5, 0x02, 0xa2, 0x67, 
0xb3, 0xd7, 0xe7, 0x02, 0x3e, 0x95, 0xe2, 0x60, 
0x05, 0x61, 0x82, 0x80, 0x7d, 0x55, 0xe5, 0xbf, 
0x93, 0x08, 0x60, 0x42, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x41, 0x11, 0x2a, 0xe0, 
0x02, 0xe4, 0x93, 0x08, 0x50, 0x06, 0x0a, 0x85, 
0x8a, 0x85, 0x73, 0x00, 0x00, 0x00, 0x01, 0xe5, 
0x01, 0x45, 0x41, 0x01, 0x82, 0x80, 0x02, 0x45, 
0xed, 0xbf, 0x93, 0x08, 0xc0, 0x08, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 
0xe0, 0x0d, 0x73, 0x00, 0x00, 0x00, 0x82, 0x80, 
0x93, 0x08, 0x70, 0x0d, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x41, 0x11, 0x06, 0xe4, 
0x01, 0x46, 0xaa, 0x85, 0x7d, 0x55, 0xef, 0xf0, 
0x1f, 0xf4, 0xa2, 0x60, 0x41, 0x01, 0x82, 0x80, 
0x93, 0x08, 0x00, 0x19, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 0x10, 0x19, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0x93, 0x08, 0x20, 0x19, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 0x00, 0x05, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0x93, 0x08, 0x50, 0x02, 0x02, 0x17, 0x01, 0x93, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0x93, 0x08, 0x30, 0x02, 0x02, 0x16, 0x01, 0x92, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0x41, 0x11, 0x06, 0xe4, 0x01, 0x47, 0xae, 0x86, 
0x13, 0x06, 0xc0, 0xf9, 0xaa, 0x85, 0x13, 0x05, 
0xc0, 0xf9, 0xef, 0xf0, 0xff, 0xfc, 0xa2, 0x60, 
0x41, 0x01, 0x82, 0x80, 0x41, 0x11, 0x06, 0xe4, 
0x01, 0x46, 0xaa, 0x85, 0x13, 0x05, 0xc0, 0xf9, 
0xef, 0xf0, 0x9f, 0xfc, 0xa2, 0x60, 0x41, 0x01, 
0x82, 0x80, 0x93, 0x08, 0x00, 0x0a, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 
0x60, 0x0d, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0xc5, 0x48, 0x73, 0x00, 0x00, 0x00, 
0x82, 0x80, 0x93, 0x08, 0x10, 0x03, 0x73, 0x00, 
0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 0xaa, 0x87, 
0x13, 0x96, 0x05, 0x02, 0x01, 0x92, 0x93, 0x08, 
0x20, 0x02, 0x13, 0x05, 0xc0, 0xf9, 0xbe, 0x85, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0x93, 0x08, 0xd0, 0x03, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 0xb0, 0x03, 
0x81, 0x45, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0xdd, 0x48, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0xe1, 0x48, 0x01, 0x46, 
0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 0x82, 0x80, 
0x93, 0x08, 0x80, 0x02, 0x73, 0x00, 0x00, 0x00, 
0x01, 0x25, 0x82, 0x80, 0x93, 0x08, 0x70, 0x02, 
0x81, 0x45, 0x73, 0x00, 0x00, 0x00, 0x01, 0x25, 
0x82, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x2f, 0x63, 0x6c, 0x6f, 0x6e, 0x65, 0x00, 0x00, 
0x2f, 0x65, 0x78, 0x69, 0x74, 0x00, 0x00, 0x00, 
0x2f, 0x66, 0x6f, 0x72, 0x6b, 0x00, 0x00, 0x00, 
0x2f, 0x67, 0x65, 0x74, 0x70, 0x69, 0x64, 0x00, 
0x2f, 0x67, 0x65, 0x74, 0x70, 0x70, 0x69, 0x64, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x2f, 0x67, 0x65, 0x74, 0x74, 0x69, 0x6d, 0x65, 
0x6f, 0x66, 0x64, 0x61, 0x79, 0x00, 0x00, 0x00, 
0x2f, 0x75, 0x6e, 0x61, 0x6d, 0x65, 0x00, 0x00, 
0x2f, 0x77, 0x61, 0x69, 0x74, 0x00, 0x00, 0x00, 
0x2f, 0x73, 0x6c, 0x65, 0x65, 0x70, 0x00, 0x00, 
0x2f, 0x77, 0x61, 0x69, 0x74, 0x70, 0x69, 0x64, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x2f, 0x79, 0x69, 0x65, 0x6c, 0x64, 0x00, 0x00, 
0x2f, 0x74, 0x69, 0x6d, 0x65, 0x73, 0x00, 0x00, 
0x2f, 0x67, 0x65, 0x74, 0x63, 0x77, 0x64, 0x00, 
0x2f, 0x63, 0x68, 0x64, 0x69, 0x72, 0x00, 0x00, 
0x2f, 0x6f, 0x70, 0x65, 0x6e, 0x61, 0x74, 0x00, 
0x2f, 0x6f, 0x70, 0x65, 0x6e, 0x00, 0x00, 0x00, 
0x2f, 0x72, 0x65, 0x61, 0x64, 0x00, 0x00, 0x00, 
0x2f, 0x77, 0x72, 0x69, 0x74, 0x65, 0x00, 0x00, 
0x2f, 0x63, 0x6c, 0x6f, 0x73, 0x65, 0x00, 0x00, 
0x2f, 0x6d, 0x6b, 0x64, 0x69, 0x72, 0x5f, 0x00, 
0x2f, 0x70, 0x69, 0x70, 0x65, 0x00, 0x00, 0x00, 
0x2f, 0x64, 0x75, 0x70, 0x00, 0x00, 0x00, 0x00, 
0x2f, 0x64, 0x75, 0x70, 0x32, 0x00, 0x00, 0x00, 
0x2f, 0x67, 0x65, 0x74, 0x64, 0x65, 0x6e, 0x74, 
0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x2f, 0x65, 0x78, 0x65, 0x63, 0x76, 0x65, 0x00, 
0x2f, 0x62, 0x72, 0x6b, 0x00, 0x00, 0x00, 0x00, 
0x2f, 0x66, 0x73, 0x74, 0x61, 0x74, 0x00, 0x00, 
0x2f, 0x75, 0x6e, 0x6c, 0x69, 0x6e, 0x6b, 0x00, 
0x2f, 0x6d, 0x6f, 0x75, 0x6e, 0x74, 0x00, 0x00, 
0x2f, 0x75, 0x6d, 0x6f, 0x75, 0x6e, 0x74, 0x00, 
0x2f, 0x6d, 0x6d, 0x61, 0x70, 0x00, 0x00, 0x00, 
0x2f, 0x6d, 0x75, 0x6e, 0x6d, 0x61, 0x70, 0x00, 
0x18, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x20, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x28, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x30, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x38, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x48, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x58, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x60, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x68, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x70, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x80, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x88, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x90, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x98, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xa0, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xa8, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xb0, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xb8, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xc0, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xc8, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xd0, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xd8, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xe0, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xe8, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xf8, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x08, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x10, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x18, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x20, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x28, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x30, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
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
    p->tcontext.sp = PAGE_SIZE - 16; // 用户栈
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
    if(proc->k_stack)
        kfree(proc->k_stack);
    if(!proc->pg_t) return;

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

void* userva2kernelva(uint64 *pgtva, uint64 uaddr)
{
    uint64 pg0_index = (uaddr >> 30) & 0x1FF;
    uint64 pg1_index = (uaddr >> 21) & 0x1FF;
    uint64 pg2_index = (uaddr >> 12) & 0x1FF;
    uint64 offset = uaddr & 0xfff;
    uint64 *pg=pgtva;
    pg = PTE2PA(pg[pg0_index]) + PV_OFFSET;
    pg = PTE2PA(pg[pg1_index]) + PV_OFFSET;
    return (void*)((uint64)(PTE2PA(pg[pg2_index]) + PV_OFFSET) + offset);
}

void* build_pgt(uint64 *pg0_t, uint64 page_begin_va, uint64 page_cnt)
{
    if(!pg0_t || !page_cnt) return NULL;

    uint64 *pg0, *pg1, *pg2;
    int cnt;
    void *rt;
    
    for(cnt = 0; cnt < page_cnt; cnt++)
    {
        uint64 va = page_begin_va + PAGE_SIZE * cnt;
        uint64 pg0_index = (va >> 30) & 0x1FF;
        uint64 pg1_index = (va >> 21) & 0x1FF;
        uint64 pg2_index = (va >> 12) & 0x1FF;

        pg0 = pg0_t;
        if(!(pg0[pg0_index] & PTE_V))
        {
            pg1 = kmalloc(PAGE_SIZE);
            if(!pg1)
                return NULL;
            memset(pg1, 0, PAGE_SIZE);
            pg0[pg0_index] = PA2PTE((uint64)pg1 - PV_OFFSET) | PTE_V;
        } else pg1 = PTE2PA(pg0[pg0_index]) + PV_OFFSET;

        if(!(pg1[pg1_index] & PTE_V))
        {
            pg2 = kmalloc(PAGE_SIZE);
            if(!pg2)
                return NULL;
            memset(pg2, 0, PAGE_SIZE);
            pg1[pg1_index] = PA2PTE((uint64)pg2 - PV_OFFSET) | PTE_V;
        } else pg2 = PTE2PA(pg1[pg1_index]) + PV_OFFSET;
        void *mem = kmalloc(PAGE_SIZE);
        if(cnt == 0) rt = mem;
        pg2[pg2_index] = PA2PTE((uint64)mem - PV_OFFSET) | PTE_V | PTE_R | PTE_W  | PTE_X | PTE_U;
    }
    return rt;
}

uint64 getpagecnt(uint64 size)
{
    if((size & 0xfff)==0)
        return size>>12;
    else return ((size>>12)+1);
}

struct Process* create_proc_by_elf(char *absolute_path)
{
    if(!absolute_path) return NULL;

    int fd = vfs_open(absolute_path, VFS_OFLAG_READ);
    if(fd < 0)
    {
        #ifdef _DEBUG
        printk("open file failed\n");
        #endif
        return NULL;
    }

    struct elfhdr ehdr;
    if(vfs_read(fd, &ehdr, sizeof(struct elfhdr)) != sizeof(struct elfhdr))
    {
        #ifdef _DEBUG
        printk("read elfheader error\n");
        #endif
        return NULL;
    }
    if(ehdr.magic != ELF_MAGIC)
    {
        #ifdef _DEBUG
        printk("is not elf file\n");
        #endif
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
        return NULL;
    }
    char cwd[VFS_PATH_MAX] = {0};
    memcpy(cwd, absolute_path, pos);
    cwd[pos] = '\0';
    proc->cwd = vfs_opendir(cwd);
    uint64 *pg_t = kmalloc(PAGE_SIZE);
    memset(pg_t, 0, PAGE_SIZE);
    pg_t[508] = k_pg_t[508];
    pg_t[509] = k_pg_t[509];
    pg_t[510] = k_pg_t[510]; // 复制内核页表

    proc->pg_t = pg_t;

    vfs_lseek(fd, ehdr.phoff, VFS_SEEK_SET);
    uint64 phdrsize = (uint64)ehdr.phentsize * ehdr.phnum;

    struct proghdr *phdr = kmalloc(phdrsize);
    int size = vfs_read(fd, phdr, sizeof(struct proghdr) * ehdr.phnum);
    if(size!=phdrsize)
    {
        #ifdef _DEBUG
        printk("read section table error, size: %d\n", size);
        #endif
        kfree(phdr);
        free_process_mem(proc);
        free_process_struct(proc);
        return NULL;
    }

    int seci;
    int secaddrmin = 0;

    // 切换到新进程内存空间
    uint64 sapt = ((((uint64)proc->pg_t) - PV_OFFSET) >> 12) | (8L << 60);
    set_satp(sapt);
    sfence_vma();

    for(seci=0; seci < ehdr.phnum; seci++) // 遍历elf文件段表
    {
        struct proghdr *sec = &phdr[seci];
        uint64 vaddr = sec->vaddr;
        uint64 file_offset = sec->off;
        uint64 file_size = sec->filesz;

        // printk("section%d vaddr:0x%lx fileoff: 0x%lx\n", seci, vaddr, file_offset);

        uint64 vpagestart = (vaddr >> 12) << 12;
        uint64 page_cnt = getpagecnt(vaddr + sec->memsz - vpagestart);

        void *mem = build_pgt(proc->pg_t, vpagestart, page_cnt); // 分配页面并设置页表
        if(!mem)
        {
            #ifdef _DEBUG
            printk("read section table error\n");
            #endif
            kfree(phdr);
            free_process_mem(proc);
            free_process_struct(proc);
            return NULL;
        }
        sfence_vma(); // 刷新页表寄存器
        vfs_lseek(fd, file_offset, VFS_SEEK_SET);
        uint64 ss = vfs_read(fd, vaddr, file_size); // 读取进程数据到指定内存
        if(vpagestart + page_cnt * PAGE_SIZE> secaddrmin)
            secaddrmin = vpagestart + page_cnt * PAGE_SIZE; // 保存最高的段地址后面用来初始化brk
    
    }
    kfree(phdr);

    build_pgt(proc->pg_t, 0, 1);
    build_pgt(proc->pg_t, secaddrmin, 3);
    sfence_vma();

    proc->tcontext.k_sp = (uint64)(proc->k_stack) + PAGE_SIZE;
    proc->tcontext.hartid = 0;
    proc->tcontext.sepc = ehdr.entry; // 用户程序从这个地址开始执行
    proc->tcontext.sp = secaddrmin + PAGE_SIZE * 2 - 8; // 用户栈
    proc->tcontext.sstatus = get_sstatus();
    proc->brk = proc->minbrk = secaddrmin + PAGE_SIZE * 2;

    uint64 *sp = proc->tcontext.sp;
    *sp = 0; // 栈顶第一个元素argc置为0

    proc->pcontext.ra = (uint64)trap_ret;
    struct trap_context *t = &proc->tcontext;
    set_user_mode(t);
    vfs_close(fd);
    // 切换为原进程页表
    sapt = ((((uint64)getcpu()->cur_proc->pg_t) - PV_OFFSET) >> 12) | (8L << 60);
    set_satp(sapt);
    sfence_vma();
    return proc;
}

int sys_test(const char *path)
{
    struct Process *proc = create_proc_by_elf(path);
    if(!proc) return -1;
    proc->parent = getcpu()->cur_proc;
    lock(&getcpu()->cur_proc->lock);
    add_before(&proc->parent->child_list, &proc->child_list_node);
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

    struct Process *p0 = init_pid0(); // 初始化init 0号进程
    getcpu()->cur_proc = p0;
    // struct Process *p1 = create_proc_by_elf("/sleep");
    // if(p1)
    // {
    //     p1->parent = p0;
    //     add_before(&p0->child_list, &p1->child_list_node);
    //     lock(&list_lock);
    //     add_before(&ready_list, &p1->status_list_node);
    //     unlock(&list_lock);
    // } else printk("file not exist\n");

    struct Process *p1 = init_process(testdata, sizeof(testdata));
    p1->parent = p0;
    add_before(&p0->child_list, &p1->child_list_node);
    
    #ifdef _DEBUG
    printk("user init success\n");
    #endif
}