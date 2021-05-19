#ifndef USYSCALL_H
#define USYSCALL_H

#include "type.h"
#include "syscall.h"

static inline uint64 _syscall(long n, uint64 _a0, uint64 _a1, uint64 _a2, uint64
		_a3, uint64 _a4, uint64 _a5) {
	register uint64 a0 asm("a0") = _a0;
	register uint64 a1 asm("a1") = _a1;
	register uint64 a2 asm("a2") = _a2;
	register uint64 a3 asm("a3") = _a3;
	register uint64 a4 asm("a4") = _a4;
	register uint64 a5 asm("a5") = _a5;
	register long syscall_id asm("a7") = n;
	asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"
			(a5), "r"(syscall_id));
	return a0;
}

static inline int sys_test(int c)
{
    return _syscall(0, c, 0, 0, 0, 0, 0);
}

#endif