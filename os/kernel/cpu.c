#include "cpu.h"

struct cpu cpus[CPU_N];

void cpu_init(uint64 hartid)
{
    set_tp(hartid);
    struct cpu *p = &cpus[hartid];
    p->cur_proc = 0;
}

uint64 get_time(void)
{
    register uint64 rt asm("a0");
    asm volatile("csrr %0, time" : "=r"(rt));
    return rt;
}