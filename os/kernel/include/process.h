#ifndef PROCESS_H
#define PROCESS_H

#include "type.h"
#include "list.h"
#include "spinlock.h"
#include "sem.h"
#include "intr.h"
#include "vfs.h"
#include "signal.h"

#define PROC_N  100

#define PROC_DEAD   0
#define PROC_RUN    1
#define PROC_READY  2
#define PROC_WAIT   3
#define PROC_SLEEP  4

#define T_SLICE     100 // 默认时间片大小

#define PROC_FILE_MAX 10

struct proc_context
{
    uint64 ra;
    uint64 sp;
  // callee-saved
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};

struct Process
{
    uint64 *k_stack;
    uint64 *pg_t;
    struct trap_context tcontext;
    struct proc_context pcontext;
    
    int32 pid;
    int32 status;

    int64 t_wait;
    int64 t_slice;

    char code;
    struct semephore sleep_lock;

    struct Process *parent;
    list status_list_node;

    list child_list_node;
    list child_list_head;

    list vma_list_head;
    
    void *data;
    spinlock lock;

    uint64 start_time;
    uint64 end_time;
    uint64 utime_start;
    uint64 stime_start;
    uint64 utime;
    uint64 stime;

    vfs_dir_t *cwd;
    int kfd;
    list ufiles_list_head;
    
    void *minbrk;
    void *brk;

    struct sigaction sigactions[SIGRTMIN];
    uint do_signal;
    uint sigmask;
    list signal_list_head;

    uint receive_kill;
};

#define status_list_node2proc(l)    GET_STRUCT_ENTRY(l, struct Process, status_list_node)
#define child_list_head2proc(l)          GET_STRUCT_ENTRY(l, struct Process, child_list_head)
#define child_list_node2proc(l)     GET_STRUCT_ENTRY(l, struct Process, child_list_node)

#define set_user_mode(p) ({ \
    uint64 sstatus = p->sstatus; \
    sstatus &= (~SPP); \
    sstatus |= SPIE; \
    p->sstatus = sstatus; \
})

#define AT_NULL 0      /* end of vector */
#define AT_IGNORE 1    /* entry should be ignored */
#define AT_EXECFD 2    /* file descriptor of program */
#define AT_PHDR 3      /* program headers for program */
#define AT_PHENT 4     /* size of program header entry */
#define AT_PHNUM 5     /* number of program headers */
#define AT_PAGESZ 6    /* system page size */
#define AT_BASE 7      /* base address of interpreter */
#define AT_FLAGS 8     /* flags */
#define AT_ENTRY 9     /* entry point of program */
#define AT_NOTELF 10   /* program is not ELF */
#define AT_UID 11      /* real uid */
#define AT_EUID 12     /* effective uid */
#define AT_GID 13      /* real gid */
#define AT_EGID 14     /* effective gid */
#define AT_PLATFORM 15 /* string identifying CPU for optimizations */
#define AT_HWCAP 16    /* arch dependent hints at CPU capabilities */
#define AT_CLKTCK 17   /* frequency at which times() increments */
/* AT_* values 18 through 22 are reserved */
#define AT_SECURE 23 /* secure mode boolean */
#define AT_BASE_PLATFORM 24 /* string identifying real platform, may differ from AT_PLATFORM. */
#define AT_RANDOM 25 /* address of 16 random bytes */

#define AT_EXECFN 31 /* filename of program */

#define AT_VECTOR_SIZE_BASE 19 /* ADD_AUXV entries in auxiliary table */
/* number of "#define AT_.*" above, minus {AT_NULL, AT_IGNORE, AT_NOTELF} */


void proc_init(void);
int32 get_pid(void);
void free_pid(int32 pid);
struct Process* get_proc_by_pid(int pid);

struct Process* new_proc(void);
void free_process_mem(struct Process *proc);
void free_process_struct(struct Process *proc);
void* build_pgt(uint64 *pg0_t, uint64 page_begin_va, uint64 page_cnt);
void free_ufile_list(struct Process *p);

void kill_current(void);

void print_user_stack(struct Process *proc);
struct Process* create_proc_by_elf(char *absolute_path, char *const argv[], char *const envp[]);

void user_init(int hartid);
#endif