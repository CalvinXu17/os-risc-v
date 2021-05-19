#include "cpu.h"

struct cpu cpus[CPU_N];


struct cpu* getcpu(void)
{
    return &(cpus[get_tp()]);
}

uint64 gethartid(void)
{
    return get_tp();
}

void cpu_init(uint64 hartid)
{
    set_tp(hartid);
    struct cpu *p = &cpus[hartid];
    p->locks_n = 0;
    p->old_intr = 0;
    p->cur_proc = 0;
}
