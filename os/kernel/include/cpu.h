#ifndef CPU_H
#define CPU_H

#include "type.h"

// static inline uint64 get_mhardid(void)
// {
//     uint64 rt;
//     asm volatile("csrr %0, mhardid" : "=r"(rt));
//     return rt;
// }

// // machine mode
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

// static inline uint64 get_mstatus(void)
// {
//     uint64 rt;
//     asm volatile("csrr %0, mstatus" : "=r"(rt));
//     return rt;
// }

// static inline void set_mstatus(uint64 x)
// {
//     asm volatile("csrw mstatus, %0"
//                  : 
//                  : "r"(x));
// }

// static inline uint64 get_mtvec(void)
// {
//     uint64 rt;
//     asm volatile("csrr %0, mtvec" : "=r"(rt));
//     return rt;
// }

// static inline void set_mtvec(uint64 x)
// {
//     asm volatile("csrw mtvec, %0"
//                  : 
//                  : "r"(x));
// }

// static inline uint64 get_mepc(void)
// {
//     uint64 rt;
//     asm volatile("csrr %0, mepc" : "=r"(rt));
//     return rt;
// }

// static inline void set_mepc(uint64 x)
// {
//     asm volatile("csrw mepc, %0"
//                  : 
//                  : "r"(x));
// }

// static inline uint64 get_mie(void)
// {
//     uint64 rt;
//     asm volatile("csrr %0, mie" : "=r"(rt));
//     return rt;
// }

// static inline void set_mie(uint64 x)
// {
//     asm volatile("csrw mie, %0"
//                  : 
//                  : "r"(x));
// }

// static inline uint64 get_mscratch(void)
// {
//     uint64 rt;
//     asm volatile("csrr %0, mscratch" : "=r"(rt));
//     return rt;
// }

// static inline void set_mscratch(uint64 x)
// {
//     asm volatile("csrw mscratch, %0"
//                  : 
//                  : "r"(x));
// }

// static inline uint64 get_mideleg(void)
// {
//     uint64 rt;
//     asm volatile("csrr %0, mideleg" : "=r"(rt));
//     return rt;
// }

// static inline void set_mideleg(uint64 x)
// {
//     asm volatile("csrw mideleg, %0"
//                  : 
//                  : "r"(x));
// }

// static inline uint64 get_mcounteren(void)
// {
//     uint64 rt;
//     asm volatile("csrr %0, mcounteren" : "=r"(rt));
//     return rt;
// }

// static inline void set_mcounteren(uint64 x)
// {
//     asm volatile("csrw mcounteren, %0" \
//                  : \
//                  : "r"(x));
// }

uint64 get_time(void);


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

static inline uint64 get_sip(void)
{
    uint64 rt;
    asm volatile("csrr %0, sip" : "=r"(rt));
    return rt;
}

static inline void set_sip(uint64 x)
{
    asm volatile("csrw sip, %0"
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

static inline void sfence_vma(void)
{
    #ifdef _K210
    asm volatile("fence");
    #endif
    asm volatile("sfence.vma");
    #ifdef _K210
    asm volatile("fence.i");
    #endif
}

static inline void wfi(void)
{
    asm volatile("wfi");
}

struct Process;
struct cpu
{
    struct Process *cur_proc;
};

#define CPU_N   2
extern struct cpu cpus[CPU_N];

static inline struct cpu* getcpu(void) __attribute__((always_inline));;
static inline uint64 gethartid(void) __attribute__((always_inline));;
void cpu_init(uint64 hartid);

static inline struct cpu* getcpu(void)
{
    return &(cpus[get_tp()]);
}

static inline uint64 gethartid(void)
{
    return get_tp();
}

#define M_MODE  0
#define S_MODE  1
#define U_MODE  3

#define SIE     (1L << 1)
#define SPIE    (1L << 5)
#define SPP     (1L << 8)
#define SUM     (1L << 18)

#define SSIE    (1L << 1)
#define STIE    (1L << 5)
#define SEIE    (1L << 9)

#define SFS     (0x00006000)

#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)
#define PTE_G (1L << 5)

#define PTE_F (1L << 8) // fork
#define PTE_S (1L << 9) // share

#define PA2PTE(pa) (((uint64)(pa)>>12)<<10)
#define PTE2PA(pa) (((uint64)(pa)>>10)<<12)


#endif