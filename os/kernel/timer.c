#include "timer.h"
#include "sbi.h"
#include "cpu.h"
#include "sched.h"
#include "process.h"
#include "spinlock.h"

static uint64 tbase=100000;
static uint64 last_timer[CPU_N] = {0};

int timerprint = 1;
uint64 cnt = 0;

int set_next_time(void)
{
    uint64 t = get_time();
    int hartid = gethartid();
    struct Process *proc;
    int rt = 0;

    if(t >= last_timer[hartid])
    {
        if(timerprint)
        {
            cnt++;
            if(cnt % 100 == 0)
                if(cnt < 16500)
                    printk("timer %ld\n", cnt);
                else while(1) {}
        }
    
        last_timer[hartid] = t + tbase;
        sbi_set_timer(last_timer[hartid]);
        
        lock(&list_lock);
        list *w = wait_list.next;
        while(w != &wait_list)
        {
            proc = status_list_node2proc(w);
            if(t >= proc->t_wait)
            {
                list *next = w->next;
                proc->t_wait = 0;
                del_list(w);
                add_before(&ready_list, w);
                w = next;
            } else     
                w = w->next;
        }
        unlock(&list_lock);

        rt = 1;
    }

    proc = getcpu()->cur_proc;
    if(proc)
    {
        lock(&proc->lock);
        if(t >= proc->t_slice)
        {
            proc->status = PROC_READY;
            move_switch2sched(proc, &ready_list);
        } else unlock(&proc->lock);
    }

    return rt;
}

void timer_init(uint64 stime_value)
{
    tbase = stime_value;
    int i;
    for(i=0; i<CPU_N;i++)
        last_timer[i] = 0;
    set_next_time();
    // enable clock
    uint64 sie = get_sie();
    sie |= STIE;
    set_sie(sie);
}

void timer_handler(struct trap_context *p)
{
    struct Process *proc;
    lock(&list_lock);
    list *w = wait_list.next;
    while(w != &wait_list)
    {
        proc = status_list_node2proc(w);
        if(get_time() >= proc->t_wait)
        {
            list *next = w->next;
            proc->t_wait = 0;
            del_list(w);
            add_before(&ready_list, w);
            w = next;
            break;
        } else     
            w = w->next;
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