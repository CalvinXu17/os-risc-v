#include "cpu.h"
#include "intr.h"
#include "printk.h"
#include "sbi.h"
#include "timer.h"

extern void trap_entry(void);

void intr_init(void)
{
    set_stvec((uint64)trap_entry);
    timer_init(TIMER_FRQ);
    intr_open();
}

int64 cnt=1;
void intr_handler(cpu *p)
{
    uint64 cause = (p->context.scause << 1) >> 1; // 最高位置0
    switch (cause)
    {
    case INT_S_SOFT:
        
        break;
    case INT_M_SOFT:
        
        break;
    case INT_S_TIMER:
        timer_handler(p);
        printk("INT_S_TIMER %ld\n", cnt++);
        break;
    case INT_M_TIMER:
        printk("INT_M_TIMER\n");
        break;
    case INT_S_EXTERNAL:
        
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
    p->intrdepth++; // 记录中断层数
    printk("trap depth: %ld\n", p->intrdepth);
    if(((int64)(p->context.scause)) < 0) // scause最高位为1为中断，0为异常
    {
        intr_handler(p);
    } else
    {
        exception_handler(p);    
    }
    p->intrdepth--;
}
