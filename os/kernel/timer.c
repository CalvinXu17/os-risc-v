#include "timer.h"
#include "sbi.h"
#include "cpu.h"
#include "sched.h"
#include "process.h"
#include "spinlock.h"

static uint64 tbase=100000;
void timer_init(uint64 stime_value)
{
    tbase = stime_value;
    sbi_set_timer(get_time() + tbase);
    // enable clock
    uint64 sie = get_sie();
    sie |= STIE;
    set_sie(sie);
}

void timer_handler(struct trap_context *p)
{
    sbi_set_timer(get_time() + tbase); // 设置下一次中断事件
    struct Process *proc;
    list *w = wait_list.next;
    
    lock(&list_lock);
    while(w != &wait_list)
    {
        proc = list2proc(w);
        lock(&proc->lock);
        proc->t_wait -= 10;
        if(proc->t_wait < 0)
        {
            list *next = w->next;
            proc->t_wait = 0;
            del_list(w);
            add_before(&ready_list, w);
            w = next;
        } else     
            w = w->next;
        unlock(&proc->lock);
    }
    unlock(&list_lock);

    proc = getcpu()->cur_proc;
    if(proc)
    {
        lock(&proc->lock);
        proc->t_slice -= 10;
        if(proc->t_slice < 0)
        {
            proc->status = PROC_READY;
            move_switch2sched(proc, &ready_list);
        } else unlock(&proc->lock);
    }
}