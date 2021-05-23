#include "sched.h"
#include "mm.h"
#include "printk.h"

struct proc_context sched_context[CPU_N]; // 保存scheduler调度器执行的上下文环境，切换到这里即切换到调度器运行
spinlock list_lock;
list ready_list;
list wait_list;
list dead_list;
list sleep_list;

void sched_init(void)
{
    init_spinlock(&list_lock, "list_lock");
    init_list(&ready_list);
    init_list(&wait_list);
    init_list(&dead_list);
    init_list(&sleep_list);
}

void scheduler(void)
{
    struct cpu *cpu = getcpu();
    int hartid = gethartid();
    uint64 sapt = 0;
    struct Process *p;
    list *l;

    while(1)
    {
        lock(&list_lock);
        l = ready_list.next;
        if(!is_empty_list(l))
        {
            p = status_list_node2proc(l);
            del_list(l);
            lock(&p->lock);
            p->status = PROC_RUN;
            p->t_slice = T_SLICE;
            p->tcontext.hartid = hartid; // 更新被调度进程的hartid为当前cpu的id
            cpu->cur_proc = p;
            sapt = ((((uint64)p->pg_t) - PV_OFFSET) >> 12) | (8L << 60);
            set_sscratch(&p->tcontext); // 设置sscratch
            set_satp(sapt);
            sfence_vma(); // 刷新页表寄存器
            unlock(&p->lock);
            unlock(&list_lock);

            #ifdef _DEBUG
            // printk("hart %d sched switch to pid %d addr 0x%lx\n", hartid, p->pid, p->pcontext.ra);
            #endif

            direct_switch(&sched_context[hartid], &p->pcontext);
        } else {
            unlock(&list_lock);
            wfi(); // cpu休眠
        }
        cpu->cur_proc = 0;
    }
}

void switch2sched(void)
{
    struct cpu *c = getcpu();
    int hartid = gethartid();
    struct Process *p = c->cur_proc;

    direct_switch(&p->pcontext, &sched_context[hartid]); // 切换到scheduler()
}

extern void _move_switch(struct proc_context *pcontext, spinlock *proc_lock, spinlock *list_lock, list *ready_list, list *status_list_node, struct proc_context *sched_context);
void _add_before(list *l, list *newl)
{
    add_after(l->prev, newl);
}

void move_switch2sched(struct Process *proc, list *l)
{
    _move_switch(&proc->pcontext, &proc->lock, &list_lock, l, &proc->status_list_node, &sched_context[gethartid()]);
}

void sleep(struct semephore *s)
{
    struct Process *p = getcpu()->cur_proc;
    lock(&p->lock);
    p->status = PROC_SLEEP;
    p->data = s;
    move_switch2sched(p, &sleep_list);
}

void wakeup(struct semephore *s)
{
    // remove p from sleep_list and add it to ready_list;
    lock(&list_lock);
    list *l = sleep_list.next;
    list *t = 0;
    while(l != &sleep_list)
    {
        t = l;
        struct Process *p = status_list_node2proc(t);
        lock(&p->lock);
        if(p->data == s)
        {
            p->data = 0;
            p->status = PROC_READY;
            del_list(t);
            add_after(&ready_list, t); // 加到头优先被调度
            unlock(&p->lock);
            break;
        }

        unlock(&p->lock);
        l = l->next;
    }
    unlock(&list_lock);
}
