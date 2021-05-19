#include "intr.h"
#include "printk.h"
#include "sbi.h"
#include "timer.h"
#include "mm.h"
#include "plic.h"
#include "console.h"
#include "syscall.h"

int64 cnt[CPU_N]={ 0 };

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
            #endif _DEBUG

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
    int i;
    for(i=0; i<CPU_N; i++)
    {
        cnt[i] = 0;
    }
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
        timer_handler(p);
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
        syscall_handler(p);
        p->sepc += 4; // 执行ecall下一条指令
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
        #ifdef _DEBUG
        printk("unknown exception\n");
        #endif
        break;
    }
}

void trap_handler(struct trap_context *p)
{
    if(((int64)(p->scause)) < 0) // scause最高位为1为中断，0为异常
    {
        intr_handler(p);
    } else
    {
        exception_handler(p);
    }
}
