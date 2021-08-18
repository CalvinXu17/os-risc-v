#ifndef SIGNAL_H
#define SIGNAL_H

#include "type.h"
#include "intr.h"
#include "list.h"

#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGBUS		 7
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGURG		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGIO		29
#define SIGPOLL		SIGIO
/*
#define SIGLOST		29
*/
#define SIGPWR		30
#define SIGSYS		31
#define	SIGUNUSED	31

/* These should not be considered constants from userland.  */
#define SIGRTMIN	32
#define SIGRTMAX    64

#define SA_NOCLDSTOP	0x00000001
#define SA_NOCLDWAIT	0x00000002
#define SA_SIGINFO	    0x00000004
#define SA_ONSTACK	    0x08000000
#define SA_RESTART	    0x10000000
#define SA_NODEFER	    0x40000000
#define SA_RESETHAND	0x80000000

#define SA_NOMASK	SA_NODEFER
#define SA_ONESHOT	SA_RESETHAND

#define SIG_DFL	    0	    /* default signal handling */
#define SIG_IGN	    1	    /* ignore signal */
#define SIG_ERR	    -1	/* error return from signal */

#define SIG_CODE_START 0x4000000000 - 4096

typedef struct siginfo
{
	int si_signo;
	int si_code;
	int si_errno;
} __attribute__((aligned(8))) siginfo_t;

struct sigaction
{
    void     (*sa_handler)(int);
    uint64     sa_flags;
    uint64     sa_mask;
} __attribute__((aligned(8)));

struct sigpending
{
    int signo;
    int senderpid;
    list sigpending_list_node;
};

#define sigpending_list_node2sigpending(l)    GET_STRUCT_ENTRY(l, struct sigpending, sigpending_list_node)

void signal_init(void);


void init_proc_sigactions(struct Process *proc);
void copy_proc_sigactions(struct Process *to, struct Process *from);

void make_sigreturn_code(struct Process *proc);
void make_sigframe(struct Process *proc, int signo, void (*sa_handler)(int));
struct sigpending* new_sigpending(void);
void free_sigpending(struct sigpending *pending);
void free_process_sigpendings(struct Process *proc);

void send_signal(struct Process *proc, int signo);

int sys_rt_sigaction(int signo, struct sigaction *act, struct sigaction *oldact, uint64 size);
void sys_rt_sigreturn(void);

int signal_handler(struct Process *proc, struct sigpending *pending);

#endif