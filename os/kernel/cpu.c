#include "cpu.h"

cpu cpus[CPU_N];


cpu* getcpu(void)
{
    return (cpu*)get_sscratch();
    // return &(cpus[get_tp()]);
}

uint64 gethartid(cpu *p)
{
    return p-cpus;
}

void cpu_init(uint64 hartid)
{
    set_tp(hartid);
    cpu *p = &cpus[hartid];
    p->k_sp = get_sp();
    p->locks_n = 0;
    p->old_intr = 0;
    set_sscratch((uint64)p);
}
