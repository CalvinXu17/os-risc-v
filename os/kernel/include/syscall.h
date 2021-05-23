#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_test            0
#define SYS_getcwd          17
#define SYS_dup             23
#define SYS_dup3            24
#define SYS_mkdirat         34
#define SYS_unlinkat        35
#define SYS_linkat          37
#define SYS_umount2         39
#define SYS_mount           40
#define SYS_chdir           49
#define SYS_openat          56
#define SYS_close           57
#define SYS_pipe2           59
#define SYS_getdents64      61
#define SYS_read            63
#define SYS_write           64
#define SYS_fstat           80
#define SYS_exit            93
#define SYS_nanosleep       101
#define SYS_sched_yield     124
#define SYS_times           153
#define SYS_uname           160
#define SYS_gettimeofday    169
#define SYS_getpid          172
#define SYS_getppid         173
#define SYS_brk             214
#define SYS_munmap          215
#define SYS_clone           220
#define SYS_execve          221
#define SYS_mmap            222
#define SYS_wait4           260

#define SYS_NR              261

#include "type.h"

struct TimeVal
{
    uint64 sec;  // 自 Unix 纪元起的秒数
    uint64 usec; // 微秒数
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


#include "intr.h"

void syscall_handler(struct trap_context *context);
void syscall_init(void);

#endif