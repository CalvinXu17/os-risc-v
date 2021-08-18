#ifndef TIMER_H
#define TIMER_H

#include "type.h"
#include "cpu.h"

// qemu 10MHz, k210 6.5MHz(8MHz in rustsbi)
#ifdef _QEMU
#define TIMER_FRQ 10000000
#endif
#ifdef _K210
#define TIMER_FRQ 8000000
#endif

struct trap_context;
void timer_init(uint64 stime_value);
int set_next_time(void);
void timer_handler(struct trap_context *);

#endif