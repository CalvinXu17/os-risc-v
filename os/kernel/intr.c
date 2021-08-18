#include "intr.h"
#include "printk.h"
#include "sbi.h"
#include "timer.h"
#include "mm.h"
#include "plic.h"
#include "console.h"
#include "syscall.h"
#include "process.h"
#include "vmm.h"

static void ext_intr_init(void)
{
    #ifdef _K210
    uint64 sie = get_sie();
    sie = sie | SEIE | SSIE; // open extern intr and soft intr
    set_sie(sie);
    #else
    uint64 sie = get_sie();
    sie = sie | SEIE; // open extern intr
    set_sie(sie);
    uint64 sstatus = get_sstatus();
    sstatus |= SFS;
    set_sstatus(sstatus);
    #endif
}

void ext_intr_handler(struct trap_context *p)
{
    int irq;
    #ifdef _K210
    uint64 stval;
    stval = get_stval();
    if(9 == stval)
    {
    #endif
        irq = plic_claim();
        if(irq == UART_IRQ)
        {
            console_intr();
        } else if(irq == DISK_IRQ)
        {
            #ifdef _DEBUG
            printk("SDCARD INTR\n");
            #endif

        }
        #ifdef _DEBUG
        else if(irq) printk("unknown extern intr: %d\n", irq);
        #endif
        if(irq) plic_complete(irq);
    
    #ifdef _K210
    }
    set_sip(get_sip() & ~2);
    sbi_set_mie();
    #endif
}

void intr_init(void)
{
    set_stvec((uint64)trap_entry);
    ext_intr_init();
    #ifdef _DEBUG
    printk("timer frequence: %ld\n", TIMER_FRQ);
    #endif
    timer_init(TIMER_FRQ/100); // 1s/100 = 10ms触发一次时钟中断
}

void intr_handler(struct trap_context *p)
{
    uint64 cause = (p->scause << 1) >> 1; // 最高位置0
    switch (cause)
    {
    case INT_S_SOFT:
    {
        #ifdef _K210
        ext_intr_handler(p);
        #endif
        break;
    }
    case INT_M_SOFT:
        
        break;
    case INT_S_TIMER:
        // timer_handler(p);
        //printk("INT_S_TIMER %ld\n", gethartid());
        break;
    case INT_M_TIMER:
        #ifdef _DEBUG
        printk("INT_M_TIMER\n");
        #endif
        break;
    case INT_S_EXTERNAL:
        #ifdef _DEBUG
        printk("INT_S_EXTERNAL\n");
        #endif
        #ifdef _QEMU
        ext_intr_handler(p);
        #endif
        break;
    case INT_M_EXTERNAL:
        
        break;
    default:
        break;
    }
}

void exception_handler(struct trap_context *p)
{
    switch (p->scause)
    {
    case EXCPT_U_ECALL:
        syscall_handler(p);
        p->sepc += 4; // 执行ecall下一条指令
        break;
    case EXCPT_MISALIGNED_INST:
        #ifdef _DEBUG
        printk("EXCPT_MISALIGNED_INST 0x%lx, pc: 0x%lx\n", p->stval, p->sepc);
        #endif
        break;
    case EXCPT_FAULT_EXE_INST:
        #ifdef _DEBUG
        printk("EXCPT_FAULT_EXE_INST 0x%lx, pc: 0x%lx\n", p->stval, p->sepc);
        #endif
        #ifdef _K210
        do_exec_fault(getcpu()->cur_proc, p->stval);
        #endif
        break;
    case EXCPT_ILLEGAL_INST:
        #ifdef _DEBUG
        printk("EXCPT_ILLEGAL_INST 0x%lx, pc: 0x%lx\n", p->stval, p->sepc);
        #endif
        break;
    case EXCPT_BREAKPOINT:
        #ifdef _DEBUG
        printk("EXCPT_BREAKPOINT 0x%lx, pc: 0x%lx\n", p->stval, p->sepc);
        #endif
        while(1) {}
        break;
    case EXCPT_MISALIGNED_LOAD:
        printk("EXCPT_MISALIGNED_LOAD 0x%lx, pc: 0x%lx\n", p->stval, p->sepc);
        break;
    case EXCPT_FAULT_LOAD:
        #ifdef _DEBUG
        printk("EXCPT_FAULT_LOAD 0x%lx, pc: 0x%lx\n", p->stval, p->sepc);
        #endif
        #ifdef _K210
        do_page_fault(getcpu()->cur_proc, p->stval);
        #endif
        break;
    case EXCPT_MISALIGNED_STORE:
        #ifdef _DEBUG
        printk("EXCPT_MISALIGNED_STORE 0x%lx, pc: 0x%lx\n", p->stval, p->sepc);
        #endif
        break;
    case EXCPT_FAULT_STORE:
        #ifdef _DEBUG
        printk("EXCPT_FAULT_STORE 0x%lx, pc: 0x%lx\n", p->stval, p->sepc);
        #endif
        #ifdef _K210
        do_page_fault(getcpu()->cur_proc, p->stval);
        #endif
        break;
    case EXCPT_S_ECALL:
        printk("EXCPT_S_ECALL\n");
        break;
    case EXCPT_M_ECALL:
        break;
    case EXCPT_INS_PAGE_FAULT:
        #ifdef _DEBUG
        printk("EXCPT_INS_PAGE_FAULT 0x%lx, pc: 0x%lx pid %lx\n", p->stval, p->sepc, getcpu()->cur_proc->pid);
        #endif
        #ifdef _QEMU
        do_exec_fault(getcpu()->cur_proc, p->stval);
        #endif
        break;
    case EXCPT_LOAD_PAGE_FAULT:
        #ifdef _DEBUG
        printk("EXCPT_LOAD_PAGE_FAULT 0x%lx, pc: 0x%lx\n", p->stval, p->sepc);
        #endif
        #ifdef _QEMU
        do_page_fault(getcpu()->cur_proc, p->stval);
        #endif
        break;
    case EXCPT_STORE_PAGE_FAULT:
        #ifdef _DEBUG
        printk("EXCPT_STORE_PAGE_FAULT 0x%lx, pc: 0x%lx\n", p->stval, p->sepc);
        #endif
        #ifdef _QEMU
        do_page_fault(getcpu()->cur_proc, p->stval);
        #endif
        break;
    default:
        #ifdef _DEBUG
        printk("unknown exception\n");
        #endif
        break;
    }
}

extern spinlock printk_lock;

void trap_handler(struct trap_context *p, uint64 isfromuser)
{
    struct Process *proc = getcpu()->cur_proc;
    uint do_signal = proc->do_signal;

    if(!isfromuser)
    {
        if(printk_lock.islocked)
        {
            unlock(&printk_lock);
        }
    } else
    {
        proc->stime_start = get_time();
        proc->utime += (get_time() - proc->utime_start);
        proc->utime_start = 0;
    }
    
    if(((int64)(p->scause)) < 0) // scause最高位为1为中断，0为异常
    {
        intr_handler(p);
    } else
    {
        exception_handler(p);
    }

    if(!do_signal && isfromuser)
    {
        lock(&proc->lock);
        list *l = proc->signal_list_head.next;
        while(l != &proc->signal_list_head)
        {
            struct sigpending *pending = sigpending_list_node2sigpending(l);
            l = l->next;
            del_list(&pending->sigpending_list_node);
            unlock(&proc->lock);
            if(signal_handler(proc, pending)) break;
            lock(&proc->lock);
        }
        unlock(&proc->lock);
    }

    if(isfromuser)
    {
        set_next_time();
        proc->stime += get_time() - proc->stime_start;
        proc->stime_start = 0;
        proc->utime_start = get_time();
        set_sscratch(&proc->tcontext);  // 进入内核态后sscratch被置为0，用于区别是用户态还是内核态，trap返回前需恢复sscratch即&proc->tcontext
    }
}
