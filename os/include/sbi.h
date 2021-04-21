#ifndef SBI_H
#define SBI_H

#include "type.h"

#define EID_SET_TIMER               0
#define EID_CONSOLE_PUTCHAR         1
#define EID_CONSOLE_GETCHAR         2
#define EID_CLEAR_IPI               3
#define EID_SEND_IPI                4
#define EID_REMOTE_FENCE_I          5
#define EID_REMOTE_SFENCE_VMA       6
#define EID_REMOTE_SFENCE_VMA_ASID  7  
#define EID_SHUTDOWN                8

#define SBI_SUCCESS                 0
#define SBI_ERR_FAILED              -1
#define SBI_ERR_NOT_SUPPORTED       -2
#define SBI_ERR_INVALID_PARAM       -3
#define SBI_ERR_DENIED              -4
#define SBI_ERR_INVALID_ADDRESS     -5
#define SBI_ERR_ALREADY_AVAILABLE   -6

typedef struct _sbiret 
{
    long error;
    long value;
}sbiret;

static inline sbiret sbi_ecall(uint64 EID, uint64 FID, uint64 arg0, uint64 arg1, uint64 arg2, uint64 arg3) __attribute__((always_inline));
static inline void sbi_set_timer(uint64 stime_value) __attribute__((always_inline));
static inline void sbi_console_putchar(int ch) __attribute__((always_inline));
static inline int sbi_console_getchar(void) __attribute__((always_inline));
static inline void sbi_clear_ipi(void) __attribute__((always_inline));
static inline void sbi_send_ipi(const unsigned long *hart_mask) __attribute__((always_inline));
static inline void sbi_remote_fence_i(const unsigned long *hart_mask) __attribute__((always_inline));
static inline void sbi_remote_sfence_vma(const unsigned long *hart_mask,
                           unsigned long start,
                           unsigned long size) __attribute__((always_inline));
static inline void sbi_remote_sfence_vma_asid(const unsigned long *hart_mask,
                                unsigned long start,
                                unsigned long size,
                                unsigned long asid) __attribute__((always_inline));
static inline void sbi_shutdown(void) __attribute__((always_inline));

static inline sbiret sbi_ecall(uint64 EID, uint64 FID, uint64 arg0, uint64 arg1, uint64 arg2, uint64 arg3)
{
    sbiret rt;
    asm volatile("mv a7, %2\n\t"
                 "mv a6, %3\n\t"
                 "mv a0, %4\n\t"
                 "mv a1, %5\n\t"
                 "mv a2, %6\n\t"
                 "mv a3, %7\n\t"
                 "ecall\n\t"
                 "mv %0, a0\n\t"
                 "mv %1, a1\n"
                 : "=r"(rt.error), "=r"(rt.value)
                 : "r"(EID), "r"(FID), "r"(arg0), "r"(arg1), "r"(arg2), "r"(arg3));
    return rt;
}

static inline void sbi_set_timer(uint64 stime_value)
{
    sbi_ecall(EID_SET_TIMER, 0, stime_value, 0, 0, 0);
}

static inline void sbi_console_putchar(int ch)
{
    sbi_ecall(EID_CONSOLE_PUTCHAR, 0, ch, 0, 0, 0);
}

static inline int sbi_console_getchar(void)
{
    return sbi_ecall(EID_CONSOLE_GETCHAR, 0, 0, 0, 0, 0).error;
}

static inline void sbi_clear_ipi(void)
{
    sbi_ecall(EID_CLEAR_IPI, 0, 0, 0, 0, 0);
}

static inline void sbi_send_ipi(const unsigned long *hart_mask)
{
    sbi_ecall(EID_SEND_IPI, 0, (uint64)hart_mask, 0, 0, 0);
}

static inline void sbi_remote_fence_i(const unsigned long *hart_mask)
{
    sbi_ecall(EID_REMOTE_FENCE_I, 0, (uint64)hart_mask, 0, 0, 0);
}

static inline void sbi_remote_sfence_vma(const unsigned long *hart_mask,
                           unsigned long start,
                           unsigned long size)
{
    sbi_ecall(EID_REMOTE_SFENCE_VMA, 0, (uint64)hart_mask, start, size, 0);
}

static inline void sbi_remote_sfence_vma_asid(const unsigned long *hart_mask,
                                unsigned long start,
                                unsigned long size,
                                unsigned long asid)
{
    sbi_ecall(EID_REMOTE_SFENCE_VMA_ASID, 0, (uint64)hart_mask, start, size, asid);
}

static inline void sbi_shutdown(void)
{
    sbi_ecall(EID_SHUTDOWN, 0, 0, 0, 0, 0);
}

#endif