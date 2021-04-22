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

void cpu_init(cpu *p)
{
    uint64 hartid = p - cpus;
    set_tp(hartid);
    p->k_sp = get_sp();
    p->intrdepth = 0;
    p->intrenable = 0;
    set_sscratch((uint64)p);
}
