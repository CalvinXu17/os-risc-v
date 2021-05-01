#include "cpu.h"
#include "intr.h"
#include "printk.h"
#include "sbi.h"
#include "timer.h"
#include "mm.h"
#include "plic.h"
#include "console.h"

extern void trap_entry(void);

void ext_intr_init(void)
{
    #ifdef _K210
    uint64 sie = get_sie();
    sie = sie | 0x200 | 0x2; // open extern intr and soft intr
    set_sie(sie);
    #else
    uint64 sie = get_sie();
    sie = sie | 0x200; // open extern intr
    set_sie(sie);
    #endif
}

void ext_intr_handler(cpu *p)
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
            printk("SDCARD INTR\n");
        } else if(irq) printk("unknown extern intr: %d\n", irq);
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
    printk("timer frequence: %ld\n", TIMER_FRQ);
    timer_init(TIMER_FRQ);
    intr_open();
}

int64 cnt[CPU_N]={ 0 };
void intr_handler(cpu *p)
{
    uint64 cause = (p->context.scause << 1) >> 1; // 最高位置0
    uint64 stval;
    int irq;
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
        timer_handler(p);
        printk("INT_S_TIMER %ld\n", ++cnt[gethartid(p)]);
        break;
    case INT_M_TIMER:
        printk("INT_M_TIMER\n");
        break;
    case INT_S_EXTERNAL:
        printk("INT_S_EXTERNAL\n");
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

void exception_handler(cpu *p)
{
    switch (p->context.scause)
    {
    case EXCPT_MISALIGNED_INST:
        break;
    case EXCPT_FAULT_EXE_INST:
        break;
    case EXCPT_ILLEGAL_INST:
        break;
    case EXCPT_BREAKPOINT:
        break;
    case EXCPT_MISALIGNED_LOAD:
        break;
    case EXCPT_FAULT_LOAD:
        break;
    case EXCPT_MISALIGNED_STORE:
        break;
    case EXCPT_FAULT_STORE:
        break;
    case EXCPT_U_ECALL:
        break;
    case EXCPT_S_ECALL:
        break;
    case EXCPT_M_ECALL:
        break;
    case EXCPT_INS_PAGE_FAULT:
        break;
    case EXCPT_LOAD_PAGE_FAULT:
        break;
    case EXCPT_STORE_PAGE_FAULT:
        break;
    default:
        break;
    }
}

void trap_handler(cpu *p)
{
    // printk("hartid: %ld ", gethartid(p));
    if(((int64)(p->context.scause)) < 0) // scause最高位为1为中断，0为异常
    {
        intr_handler(p);
    } else
    {
        exception_handler(p);    
    }
}
