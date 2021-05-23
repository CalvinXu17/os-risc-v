#ifndef INTR_H
#define INTR_H

#include "type.h"
#include "cpu.h"

struct trap_context
{
    uint64 ra;
    uint64 sp;
    uint64 gp;
    uint64 tp;
    uint64 t0;
    uint64 t1;
    uint64 t2;
    uint64 s0; // fp
    uint64 s1;
    uint64 a0;
    uint64 a1;
    uint64 a2;
    uint64 a3;
    uint64 a4;
    uint64 a5;
    uint64 a6;
    uint64 a7;
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
    uint64 t3;
    uint64 t4;
    uint64 t5;
    uint64 t6;
    
    uint64 sstatus;
    uint64 sepc;
    uint64 sbadvaddr;
    uint64 scause;
    uint64 stval;
    uint64 k_sp;
    uint64 hartid;
};

#define INT_S_SOFT       1
#define INT_M_SOFT       3
#define INT_S_TIMER      5
#define INT_M_TIMER      7
#define INT_S_EXTERNAL   9
#define INT_M_EXTERNAL   11

#define EXCPT_MISALIGNED_INST      0
#define EXCPT_FAULT_EXE_INST       1
#define EXCPT_ILLEGAL_INST         2
#define EXCPT_BREAKPOINT           3
#define EXCPT_MISALIGNED_LOAD      4
#define EXCPT_FAULT_LOAD           5
#define EXCPT_MISALIGNED_STORE     6
#define EXCPT_FAULT_STORE          7
#define EXCPT_U_ECALL              8
#define EXCPT_S_ECALL              9
#define EXCPT_M_ECALL              11
#define EXCPT_INS_PAGE_FAULT       12
#define EXCPT_LOAD_PAGE_FAULT      13
#define EXCPT_STORE_PAGE_FAULT     15

static inline void intr_open(void)
{
    uint64 sstatus = get_sstatus();
    sstatus |= 0x2; // 0010
    set_sstatus(sstatus);
}

static inline void intr_close(void)
{
    uint64 sstatus = get_sstatus();
    sstatus &= 0xfffffffffffffffd; // 1101
    set_sstatus(sstatus);
}

static inline uint64 is_intr_open(void)
{
    uint64 sstatus = get_sstatus();
    sstatus >>= 1;
    sstatus &= 0x1;
    return sstatus;
}

void intr_init(void);
extern void trap_entry(void);
extern void trap_ret(void);

#endif