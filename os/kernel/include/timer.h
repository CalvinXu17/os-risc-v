#ifndef TIMER_H
#define TIMER_H

#include "type.h"
#include "cpu.h"

// qemu 10MHz, k210 6.5MHz
#ifdef _QEMU
#define TIMER_FRQ 10000000
#endif
#ifdef _K210
#define TIMER_FRQ 6500000
#endif

void timer_init(uint64 stime_value);
void set_next_time(void);
void timer_handler(cpu *);

#endif