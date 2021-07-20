#ifndef SYSCALL_H
#define SYSCALL_H

#include "syscall_ids.h"
#include "type.h"

struct TimeVal
{
    uint64 sec;  // 自 Unix 纪元起的秒数
    uint64 usec; // 微秒数
};

struct tms              
{                     
	long tms_utime;  
	long tms_stime;  
	long tms_cutime; 
	long tms_cstime; 
};

struct utsname {
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
	char domainname[65];
};

#define WNOHANG		0x00000001
#define WUNTRACED	0x00000002
#define WEXITED		0x00000004
#define WCONTINUED	0x00000008
#define WNOWAIT		0x01000000	/* Don't reap, just poll status.  */

#define SIGCHLD   17

#define CLOCK_REALTIME			    0
#define CLOCK_MONOTONIC			    1
#define CLOCK_PROCESS_CPUTIME_ID	2
#define CLOCK_THREAD_CPUTIME_ID		3
#define CLOCK_MONOTONIC_RAW		    4
#define CLOCK_REALTIME_COARSE		5
#define CLOCK_MONOTONIC_COARSE		6
#define CLOCK_BOOTTIME			    7
#define CLOCK_REALTIME_ALARM		8
#define CLOCK_BOOTTIME_ALARM		9

#define MAKE_SYSCALL00(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    sys_##name(); \
}

#define MAKE_SYSCALL10(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    sys_##name(context->a0); \
}

#define MAKE_SYSCALL20(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    sys_##name(context->a0, context->a1); \
}

#define MAKE_SYSCALL30(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    sys_##name(context->a0, context->a1, context->a2); \
}

#define MAKE_SYSCALL40(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    sys_##name(context->a0, context->a1, context->a2, context->a3); \
}

#define MAKE_SYSCALL50(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    sys_##name(context->a0, context->a1, context->a2, context->a3, context->a4); \
}

#define MAKE_SYSCALL60(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    sys_##name(context->a0, context->a1, context->a2, context->a3, context->a4, context->a5); \
}


#define MAKE_SYSCALL01(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    context->a0 = sys_##name(); \
}

#define MAKE_SYSCALL11(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    context->a0 = sys_##name(context->a0); \
}

#define MAKE_SYSCALL21(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    context->a0 = sys_##name(context->a0, context->a1); \
}

#define MAKE_SYSCALL31(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    context->a0 = sys_##name(context->a0, context->a1, context->a2); \
}

#define MAKE_SYSCALL41(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    context->a0 = sys_##name(context->a0, context->a1, context->a2, context->a3); \
}

#define MAKE_SYSCALL51(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    context->a0 = sys_##name(context->a0, context->a1, context->a2, context->a3, context->a4); \
}

#define MAKE_SYSCALL61(name) \
void do_sys_##name(struct trap_context *context) \
{ \
    context->a0 = sys_##name(context->a0, context->a1, context->a2, context->a3, context->a4, context->a5); \
}

#define SET_SYSCALL(name) syscall_table[SYS_##name] = do_sys_##name;
#define SET_SYSCALL_NAME(name) syscall_name[SYS_##name] = "sys_"#name;

struct trap_context;

void syscall_handler(struct trap_context *context);
void syscall_init(void);

int sys_clone(uint flags, void *stack, int *ptid, uint64 tls, int *ctid);
int sys_wait4(int pid, int *code, int options);
void sys_exit(int code);
int sys_getppid(void);
int sys_getpid(void);
int sys_times(struct tms *tms);
int sys_uname(struct utsname *uts);
void sys_sched_yield(void);
int sys_gettimeofday(struct TimeVal *time, int tz);
int sys_nanosleep(struct TimeVal *timein, struct TimeVal *timeout);
uint64 sys_brk(void* addr);
uint64 sys_mmap(uint64 start, uint64 len, uint64 prot, uint64 flags, uint64 ufd, uint64 off);
int sys_munmap(void *start, int len);

int sys_mprotect(void *addr, uint64 len, uint prot);
int sys_set_tid_address(void *addr);
int sys_getuid(void);
int sys_geteuid(void);
int sys_getgid(void);
int sys_getegid(void);

int sys_clock_gettime(int clockid, struct TimeVal *time);
int sys_clock_nanosleep(int clockid, int flags, struct TimeVal *request, struct TimeVal *remain);

#endif