#include "timer.h"
#include "sbi.h"
#include "cpu.h"

static uint64 tbase=100000;
void timer_init(uint64 stime_value)
{
    tbase = stime_value;
    sbi_set_timer(get_time() + tbase);
    // enable clock
    uint64 sie = get_sie();
    sie |= 0x20;
    set_sie(sie);
}

void timer_handler(cpu *p)
{
    sbi_set_timer(get_time() + tbase); // 设置下一次中断事件

    // TODO

}