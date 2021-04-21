#ifndef CPU_H
#define CPU_H

#include "type.h"

#define M_MODE  0
#define S_MODE  1
#define U_MODE  3

static inline uint64 get_mhardid(void)
{
    uint64 rt;
    asm volatile("csrr %0, mhardid" : "=r"(rt));
    return rt;
}

// machine mode
static inline uint64 get_mtval(void)
{
    uint64 rt;
    asm volatile("csrr %0, mtval" : "=r"(rt));
    return rt;
}

static inline uint64 get_mcause(void)
{
    uint64 rt;
    asm volatile("csrr %0, mcause" : "=r"(rt));
    return rt;
}

static inline uint64 get_mstatus(void)
{
    uint64 rt;
    asm volatile("csrr %0, mstatus" : "=r"(rt));
    return rt;
}

static inline void set_mstatus(uint64 x)
{
    asm volatile("csrw mstatus, %0"
                 : 
                 : "r"(x));
}

static inline uint64 get_mtvec(void)
{
    uint64 rt;
    asm volatile("csrr %0, mtvec" : "=r"(rt));
    return rt;
}

static inline void set_mtvec(uint64 x)
{
    asm volatile("csrw mtvec, %0"
                 : 
                 : "r"(x));
}

static inline uint64 get_mepc(void)
{
    uint64 rt;
    asm volatile("csrr %0, mepc" : "=r"(rt));
    return rt;
}

static inline void set_mepc(uint64 x)
{
    asm volatile("csrw mepc, %0"
                 : 
                 : "r"(x));
}

static inline uint64 get_mie(void)
{
    uint64 rt;
    asm volatile("csrr %0, mie" : "=r"(rt));
    return rt;
}

static inline void set_mie(uint64 x)
{
    asm volatile("csrw mie, %0"
                 : 
                 : "r"(x));
}

static inline uint64 get_mscratch(void)
{
    uint64 rt;
    asm volatile("csrr %0, mscratch" : "=r"(rt));
    return rt;
}

static inline void set_mscratch(uint64 x)
{
    asm volatile("csrw mscratch, %0"
                 : 
                 : "r"(x));
}

static inline uint64 get_mideleg(void)
{
    uint64 rt;
    asm volatile("csrr %0, mideleg" : "=r"(rt));
    return rt;
}

static inline void set_mideleg(uint64 x)
{
    asm volatile("csrw mideleg, %0"
                 : 
                 : "r"(x));
}

static inline uint64 get_mcounteren(void)
{
    uint64 rt;
    asm volatile("csrr %0, mcounteren" : "=r"(rt));
    return rt;
}

static inline void set_mcounteren(uint64 x)
{
    asm volatile("csrw mcounteren, %0" \
                 : \
                 : "r"(x));
}

static inline uint64 get_time(void)
{
    uint64 rt;
    asm volatile("csrr %0, time" : "=r"(rt));
    return rt;
}


// supervisor mode
static inline uint64 get_stval(void)
{
    uint64 rt;
    asm volatile("csrr %0, stval" : "=r"(rt));
    return rt;
}

static inline uint64 get_scause(void)
{
    uint64 rt;
    asm volatile("csrr %0, scause" : "=r"(rt));
    return rt;
}

static inline uint64 get_sstatus(void)
{
    uint64 rt;
    asm volatile("csrr %0, sstatus" : "=r"(rt));
    return rt;
}

static inline void set_sstatus(uint64 x)
{
    asm volatile("csrw sstatus, %0"
                 : 
                 : "r"(x));
}

static inline uint64 get_stvec(void)
{
    uint64 rt;
    asm volatile("csrr %0, stvec" : "=r"(rt));
    return rt;
}

static inline void set_stvec(uint64 x)
{
    asm volatile("csrw stvec, %0"
                 : 
                 : "r"(x));
}

static inline uint64 get_sepc(void)
{
    uint64 rt;
    asm volatile("csrr %0, sepc" : "=r"(rt));
    return rt;
}

static inline void set_sepc(uint64 x)
{
    asm volatile("csrw sepc, %0"
                 : 
                 : "r"(x));
}

static inline uint64 get_sie(void)
{
    uint64 rt;
    asm volatile("csrr %0, sie" : "=r"(rt));
    return rt;
}

static inline void set_sie(uint64 x)
{
    asm volatile("csrw sie, %0"
                 : 
                 : "r"(x));
}

static inline uint64 get_sscratch(void)
{
    uint64 rt;
    asm volatile("csrr %0, sscratch" : "=r"(rt));
    return rt;
}

static inline void set_sscratch(uint64 x)
{
    asm volatile("csrw sscratch, %0"
                 : 
                 : "r"(x));
}

static inline uint64 get_sideleg(void)
{
    uint64 rt;
    asm volatile("csrr %0, sideleg" : "=r"(rt));
    return rt;
}

static inline void set_sideleg(uint64 x)
{
    asm volatile("csrw sideleg, %0"
                 : 
                 : "r"(x));
}

static inline uint64 get_satp(void)
{
    uint64 rt;
    asm volatile("csrr %0, satp" : "=r"(rt));
    return rt;
}

static inline void set_satp(uint64 x)
{
    asm volatile("csrw satp, %0"
                 : 
                 : "r"(x));
}

// common register
static inline uint64 get_ra(void)
{
    uint64 rt;
    asm volatile("mv %0, ra" : "=r"(rt));
    return rt;
}

static inline uint64 get_sp(void)
{
    uint64 rt;
    asm volatile("mv %0, sp" : "=r"(rt));
    return rt;
}

static inline uint64 get_gp(void)
{
    uint64 rt;
    asm volatile("mv %0, gp" : "=r"(rt));
    return rt;
}

static inline uint64 get_tp(void)
{
    uint64 rt;
    asm volatile("mv %0, tp" : "=r"(rt));
    return rt;
}

static inline void set_tp(uint64 x)
{
    asm volatile("mv tp, %0" 
                 : 
                 : "r"(x));
}

#endif