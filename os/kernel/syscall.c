#include "syscall.h"
#include "printk.h"
#include "intr.h"
#include "process.h"
#include "cpu.h"

void* syscall_table[SYS_NR];

#ifdef _STRACE
char* syscall_name[SYS_NR];
#endif

void syscall_handler(struct trap_context *context)
{
    uint64 a0;
    a0 = context->a0;
    if(context->a7 >=0 && context->a7 < SYS_NR)
    {
        int (*syscall)(struct trap_context *);
        syscall = syscall_table[context->a7];
        syscall(context);
    } else
    {
        #ifdef _DEBUG
        printk("out of index\n");
        #endif
        context->a0 = -1;
    }
    #ifdef _STRACE
    if((int)context->a0 < 0)
    {
        printk("pid %d pc: %lx %s(%lx, %lx, %lx, %lx, %lx, %lx) = %lx\n", getcpu()->cur_proc->pid, context->sepc, syscall_name[context->a7], a0, context->a1, context->a2, context->a3, context->a4, context->a5, context->a0);
    }
    #endif
}

void unknown_syscall(struct trap_context *context)
{
    #ifdef _DEBUG
    // printk("unknown syscall %d!\n", context->a7);
    #endif
    context->a0 = -1;
}

#ifdef _STRACE
void set_syscall_name()
{
    int i;
    for(i=0;i<SYS_NR;i++)
    {
        syscall_name[i] = "unknown";
    }
    SET_SYSCALL_NAME(io_setup);
    SET_SYSCALL_NAME(io_destroy);
    SET_SYSCALL_NAME(io_submit);
    SET_SYSCALL_NAME(io_cancel);
    SET_SYSCALL_NAME(io_getevents);
    SET_SYSCALL_NAME(setxattr);
    SET_SYSCALL_NAME(lsetxattr);
    SET_SYSCALL_NAME(fsetxattr);
    SET_SYSCALL_NAME(getxattr);
    SET_SYSCALL_NAME(lgetxattr);
    SET_SYSCALL_NAME(fgetxattr);
    SET_SYSCALL_NAME(listxattr);
    SET_SYSCALL_NAME(llistxattr);
    SET_SYSCALL_NAME(flistxattr);
    SET_SYSCALL_NAME(removexattr);
    SET_SYSCALL_NAME(lremovexattr);
    SET_SYSCALL_NAME(fremovexattr);
    SET_SYSCALL_NAME(getcwd);
    SET_SYSCALL_NAME(lookup_dcookie);
    SET_SYSCALL_NAME(eventfd2);
    SET_SYSCALL_NAME(epoll_create1);
    SET_SYSCALL_NAME(epoll_ctl);
    SET_SYSCALL_NAME(epoll_pwait);
    SET_SYSCALL_NAME(dup);
    SET_SYSCALL_NAME(dup3);
    SET_SYSCALL_NAME(fcntl);
    SET_SYSCALL_NAME(inotify_init1);
    SET_SYSCALL_NAME(inotify_add_watch);
    SET_SYSCALL_NAME(inotify_rm_watch);
    SET_SYSCALL_NAME(ioctl);
    SET_SYSCALL_NAME(ioprio_set);
    SET_SYSCALL_NAME(ioprio_get);
    SET_SYSCALL_NAME(flock);
    SET_SYSCALL_NAME(mknodat);
    SET_SYSCALL_NAME(mkdirat);
    SET_SYSCALL_NAME(unlinkat);
    SET_SYSCALL_NAME(symlinkat);
    SET_SYSCALL_NAME(linkat);
    SET_SYSCALL_NAME(umount2);
    SET_SYSCALL_NAME(mount);
    SET_SYSCALL_NAME(pivot_root);
    SET_SYSCALL_NAME(nfsservctl);
    SET_SYSCALL_NAME(statfs);
    SET_SYSCALL_NAME(fstatfs);
    SET_SYSCALL_NAME(truncate);
    SET_SYSCALL_NAME(ftruncate);
    SET_SYSCALL_NAME(fallocate);
    SET_SYSCALL_NAME(faccessat);
    SET_SYSCALL_NAME(chdir);
    SET_SYSCALL_NAME(fchdir);
    SET_SYSCALL_NAME(chroot);
    SET_SYSCALL_NAME(fchmod);
    SET_SYSCALL_NAME(fchmodat);
    SET_SYSCALL_NAME(fchownat);
    SET_SYSCALL_NAME(fchown);
    SET_SYSCALL_NAME(openat);
    SET_SYSCALL_NAME(close);
    SET_SYSCALL_NAME(vhangup);
    SET_SYSCALL_NAME(pipe2);
    SET_SYSCALL_NAME(quotactl);
    SET_SYSCALL_NAME(getdents64);
    SET_SYSCALL_NAME(lseek);
    SET_SYSCALL_NAME(read);
    SET_SYSCALL_NAME(write);
    SET_SYSCALL_NAME(readv);
    SET_SYSCALL_NAME(writev);
    SET_SYSCALL_NAME(pread64);
    SET_SYSCALL_NAME(pwrite64);
    SET_SYSCALL_NAME(preadv);
    SET_SYSCALL_NAME(pwritev);
    SET_SYSCALL_NAME(sendfile);
    SET_SYSCALL_NAME(pselect6);
    SET_SYSCALL_NAME(ppoll);
    SET_SYSCALL_NAME(signalfd4);
    SET_SYSCALL_NAME(vmsplice);
    SET_SYSCALL_NAME(splice);
    SET_SYSCALL_NAME(tee);
    SET_SYSCALL_NAME(readlinkat);
    SET_SYSCALL_NAME(fstatat);
    SET_SYSCALL_NAME(fstat);
    SET_SYSCALL_NAME(sync);
    SET_SYSCALL_NAME(fsync);
    SET_SYSCALL_NAME(fdatasync);
    SET_SYSCALL_NAME(sync_file_range);
    SET_SYSCALL_NAME(timerfd_create);
    SET_SYSCALL_NAME(timerfd_settime);
    SET_SYSCALL_NAME(timerfd_gettime);
    SET_SYSCALL_NAME(utimensat);
    SET_SYSCALL_NAME(acct);
    SET_SYSCALL_NAME(capget);
    SET_SYSCALL_NAME(capset);
    SET_SYSCALL_NAME(personality);
    SET_SYSCALL_NAME(exit);
    SET_SYSCALL_NAME(exit_group);
    SET_SYSCALL_NAME(waitid);
    SET_SYSCALL_NAME(set_tid_address);
    SET_SYSCALL_NAME(unshare);
    SET_SYSCALL_NAME(futex);
    SET_SYSCALL_NAME(set_robust_list);
    SET_SYSCALL_NAME(get_robust_list);
    SET_SYSCALL_NAME(nanosleep);
    SET_SYSCALL_NAME(getitimer);
    SET_SYSCALL_NAME(setitimer);
    SET_SYSCALL_NAME(kexec_load);
    SET_SYSCALL_NAME(init_module);
    SET_SYSCALL_NAME(delete_module);
    SET_SYSCALL_NAME(timer_create);
    SET_SYSCALL_NAME(timer_gettime);
    SET_SYSCALL_NAME(timer_getoverrun);
    SET_SYSCALL_NAME(timer_settime);
    SET_SYSCALL_NAME(timer_delete);
    SET_SYSCALL_NAME(clock_settime);
    SET_SYSCALL_NAME(clock_gettime);
    SET_SYSCALL_NAME(clock_getres);
    SET_SYSCALL_NAME(clock_nanosleep);
    SET_SYSCALL_NAME(syslog);
    SET_SYSCALL_NAME(ptrace);
    SET_SYSCALL_NAME(sched_setparam);
    SET_SYSCALL_NAME(sched_setscheduler);
    SET_SYSCALL_NAME(sched_getscheduler);
    SET_SYSCALL_NAME(sched_getparam);
    SET_SYSCALL_NAME(sched_setaffinity);
    SET_SYSCALL_NAME(sched_getaffinity);
    SET_SYSCALL_NAME(sched_yield);
    SET_SYSCALL_NAME(sched_get_priority_max);
    SET_SYSCALL_NAME(sched_get_priority_min);
    SET_SYSCALL_NAME(sched_rr_get_interval);
    SET_SYSCALL_NAME(restart_syscall);
    SET_SYSCALL_NAME(kill);
    SET_SYSCALL_NAME(tkill);
    SET_SYSCALL_NAME(tgkill);
    SET_SYSCALL_NAME(sigaltstack);
    SET_SYSCALL_NAME(rt_sigsuspend);
    SET_SYSCALL_NAME(rt_sigaction);
    SET_SYSCALL_NAME(rt_sigprocmask);
    SET_SYSCALL_NAME(rt_sigpending);
    SET_SYSCALL_NAME(rt_sigtimedwait);
    SET_SYSCALL_NAME(rt_sigqueueinfo);
    SET_SYSCALL_NAME(rt_sigreturn);
    SET_SYSCALL_NAME(setpriority);
    SET_SYSCALL_NAME(getpriority);
    SET_SYSCALL_NAME(reboot);
    SET_SYSCALL_NAME(setregid);
    SET_SYSCALL_NAME(setgid);
    SET_SYSCALL_NAME(setreuid);
    SET_SYSCALL_NAME(setuid);
    SET_SYSCALL_NAME(setresuid);
    SET_SYSCALL_NAME(getresuid);
    SET_SYSCALL_NAME(setresgid);
    SET_SYSCALL_NAME(getresgid);
    SET_SYSCALL_NAME(setfsuid);
    SET_SYSCALL_NAME(setfsgid);
    SET_SYSCALL_NAME(times);
    SET_SYSCALL_NAME(time);
    SET_SYSCALL_NAME(setpgid);
    SET_SYSCALL_NAME(getpgid);
    SET_SYSCALL_NAME(getsid);
    SET_SYSCALL_NAME(setsid);
    SET_SYSCALL_NAME(getgroups);
    SET_SYSCALL_NAME(setgroups);
    SET_SYSCALL_NAME(uname);
    SET_SYSCALL_NAME(sethostname);
    SET_SYSCALL_NAME(setdomainname);
    SET_SYSCALL_NAME(getrlimit);
    SET_SYSCALL_NAME(setrlimit);
    SET_SYSCALL_NAME(getrusage);
    SET_SYSCALL_NAME(umask);
    SET_SYSCALL_NAME(prctl);
    SET_SYSCALL_NAME(getcpu);
    SET_SYSCALL_NAME(gettimeofday);
    SET_SYSCALL_NAME(settimeofday);
    SET_SYSCALL_NAME(adjtimex);
    SET_SYSCALL_NAME(getpid);
    SET_SYSCALL_NAME(getppid);
    SET_SYSCALL_NAME(getuid);
    SET_SYSCALL_NAME(geteuid);
    SET_SYSCALL_NAME(getgid);
    SET_SYSCALL_NAME(getegid);
    SET_SYSCALL_NAME(gettid);
    SET_SYSCALL_NAME(sysinfo);
    SET_SYSCALL_NAME(mq_open);
    SET_SYSCALL_NAME(mq_unlink);
    SET_SYSCALL_NAME(mq_timedsend);
    SET_SYSCALL_NAME(mq_timedreceive);
    SET_SYSCALL_NAME(mq_notify);
    SET_SYSCALL_NAME(mq_getsetattr);
    SET_SYSCALL_NAME(msgget);
    SET_SYSCALL_NAME(msgctl);
    SET_SYSCALL_NAME(msgrcv);
    SET_SYSCALL_NAME(msgsnd);
    SET_SYSCALL_NAME(semget);
    SET_SYSCALL_NAME(semctl);
    SET_SYSCALL_NAME(semtimedop);
    SET_SYSCALL_NAME(semop);
    SET_SYSCALL_NAME(shmget);
    SET_SYSCALL_NAME(shmctl);
    SET_SYSCALL_NAME(shmat);
    SET_SYSCALL_NAME(shmdt);
    SET_SYSCALL_NAME(socket);
    SET_SYSCALL_NAME(socketpair);
    SET_SYSCALL_NAME(bind);
    SET_SYSCALL_NAME(listen);
    SET_SYSCALL_NAME(accept);
    SET_SYSCALL_NAME(connect);
    SET_SYSCALL_NAME(getsockname);
    SET_SYSCALL_NAME(getpeername);
    SET_SYSCALL_NAME(sendto);
    SET_SYSCALL_NAME(recvfrom);
    SET_SYSCALL_NAME(setsockopt);
    SET_SYSCALL_NAME(getsockopt);
    SET_SYSCALL_NAME(shutdown);
    SET_SYSCALL_NAME(sendmsg);
    SET_SYSCALL_NAME(recvmsg);
    SET_SYSCALL_NAME(readahead);
    SET_SYSCALL_NAME(brk);
    SET_SYSCALL_NAME(munmap);
    SET_SYSCALL_NAME(mremap);
    SET_SYSCALL_NAME(add_key);
    SET_SYSCALL_NAME(request_key);
    SET_SYSCALL_NAME(keyctl);
    SET_SYSCALL_NAME(clone);
    SET_SYSCALL_NAME(execve);
    SET_SYSCALL_NAME(mmap);
    SET_SYSCALL_NAME(fadvise64);
    SET_SYSCALL_NAME(swapon);
    SET_SYSCALL_NAME(swapoff);
    SET_SYSCALL_NAME(mprotect);
    SET_SYSCALL_NAME(msync);
    SET_SYSCALL_NAME(mlock);
    SET_SYSCALL_NAME(munlock);
    SET_SYSCALL_NAME(mlockall);
    SET_SYSCALL_NAME(munlockall);
    SET_SYSCALL_NAME(mincore);
    SET_SYSCALL_NAME(madvise);
    SET_SYSCALL_NAME(remap_file_pages);
    SET_SYSCALL_NAME(mbind);
    SET_SYSCALL_NAME(get_mempolicy);
    SET_SYSCALL_NAME(set_mempolicy);
    SET_SYSCALL_NAME(migrate_pages);
    SET_SYSCALL_NAME(move_pages);
    SET_SYSCALL_NAME(rt_tgsigqueueinfo);
    SET_SYSCALL_NAME(perf_event_open);
    SET_SYSCALL_NAME(accept4);
    SET_SYSCALL_NAME(recvmmsg);
    SET_SYSCALL_NAME(arch_specific_syscall);
    SET_SYSCALL_NAME(wait4);
}
#endif

extern int sys_test(const char *path);

MAKE_SYSCALL11(test);
MAKE_SYSCALL31(read);
MAKE_SYSCALL31(write);
MAKE_SYSCALL51(clone);
MAKE_SYSCALL31(wait4);
MAKE_SYSCALL10(exit);
MAKE_SYSCALL01(getppid);
MAKE_SYSCALL01(getpid);
MAKE_SYSCALL11(times);
MAKE_SYSCALL11(uname);
MAKE_SYSCALL00(sched_yield);
MAKE_SYSCALL21(gettimeofday);
MAKE_SYSCALL21(nanosleep);
MAKE_SYSCALL11(brk);
MAKE_SYSCALL61(mmap);
MAKE_SYSCALL21(munmap);
MAKE_SYSCALL21(getcwd);
MAKE_SYSCALL11(chdir);
MAKE_SYSCALL41(openat);
MAKE_SYSCALL11(close);
MAKE_SYSCALL31(mkdirat);
MAKE_SYSCALL11(pipe2);
MAKE_SYSCALL11(dup);
MAKE_SYSCALL21(dup2);
MAKE_SYSCALL31(getdents64);
MAKE_SYSCALL31(execve);
MAKE_SYSCALL21(fstat);
MAKE_SYSCALL31(unlinkat);
MAKE_SYSCALL51(mount);
MAKE_SYSCALL21(umount2);

MAKE_SYSCALL31(mprotect);
MAKE_SYSCALL11(set_tid_address);
MAKE_SYSCALL01(getuid);
MAKE_SYSCALL01(geteuid);
MAKE_SYSCALL01(getgid);
MAKE_SYSCALL01(getegid);
MAKE_SYSCALL31(readv);
MAKE_SYSCALL31(writev);
MAKE_SYSCALL21(clock_gettime);
MAKE_SYSCALL41(readlinkat);
MAKE_SYSCALL11(fcntl);
MAKE_SYSCALL01(rt_sigaction);
MAKE_SYSCALL01(exit_group);
MAKE_SYSCALL41(fstatat);
MAKE_SYSCALL31(ppoll);
MAKE_SYSCALL41(clock_nanosleep);
MAKE_SYSCALL21(statfs);
MAKE_SYSCALL31(syslog);
MAKE_SYSCALL41(faccessat);
MAKE_SYSCALL21(ioctl);

void syscall_init(void)
{
    int i;
    for(i = 0; i < SYS_NR; i++)
    {
        syscall_table[i] = unknown_syscall;
    }

    #ifdef _STRACE
    set_syscall_name();
    #endif

    syscall_table[SYS_test] = do_sys_test;
    syscall_table[SYS_read] = do_sys_read;
    syscall_table[SYS_write] = do_sys_write;
    syscall_table[SYS_clone] = do_sys_clone;
    syscall_table[SYS_wait4] = do_sys_wait4;
    syscall_table[SYS_exit] = do_sys_exit;
    syscall_table[SYS_getppid] = do_sys_getppid;
    syscall_table[SYS_getpid] = do_sys_getpid;
    syscall_table[SYS_times] = do_sys_times;
    syscall_table[SYS_uname] = do_sys_uname;
    syscall_table[SYS_sched_yield] = do_sys_sched_yield;
    syscall_table[SYS_gettimeofday] = do_sys_gettimeofday;
    syscall_table[SYS_nanosleep] = do_sys_nanosleep;
    syscall_table[SYS_brk] = do_sys_brk;
    syscall_table[SYS_mmap] = do_sys_mmap;
    syscall_table[SYS_munmap] = do_sys_munmap;

    syscall_table[SYS_getcwd] = do_sys_getcwd;
    syscall_table[SYS_chdir] = do_sys_chdir;
    syscall_table[SYS_openat] = do_sys_openat;
    syscall_table[SYS_close] = do_sys_close;
    syscall_table[SYS_mkdirat] = do_sys_mkdirat;
    syscall_table[SYS_pipe2] = do_sys_pipe2;
    syscall_table[SYS_dup] = do_sys_dup;
    syscall_table[SYS_dup3] = do_sys_dup2;
    syscall_table[SYS_getdents64] = do_sys_getdents64;
    syscall_table[SYS_execve] = do_sys_execve;
    syscall_table[SYS_fstat] = do_sys_fstat;
    syscall_table[SYS_unlinkat] = do_sys_unlinkat;
    syscall_table[SYS_mount] = do_sys_mount;
    syscall_table[SYS_umount2] = do_sys_umount2;


    syscall_table[SYS_mprotect] = do_sys_mprotect;
    syscall_table[SYS_set_tid_address] = do_sys_set_tid_address;
    syscall_table[SYS_getuid] = do_sys_getuid;
    syscall_table[SYS_geteuid] = do_sys_geteuid;
    syscall_table[SYS_getgid] = do_sys_getgid;
    syscall_table[SYS_getegid] = do_sys_getegid;
    syscall_table[SYS_readv] = do_sys_readv;
    syscall_table[SYS_writev] = do_sys_writev;
    syscall_table[SYS_clock_gettime] = do_sys_clock_gettime;
    syscall_table[SYS_readlinkat] = do_sys_readlinkat;
    syscall_table[SYS_fcntl] = do_sys_fcntl;
    syscall_table[SYS_fstatat] = do_sys_fstatat;
    syscall_table[SYS_ppoll] = do_sys_ppoll;

    syscall_table[SYS_rt_sigaction] = do_sys_rt_sigaction;
    syscall_table[SYS_exit_group] = do_sys_exit_group;
    syscall_table[SYS_clock_nanosleep] = do_sys_clock_nanosleep;
    syscall_table[SYS_statfs] = do_sys_statfs;
    syscall_table[SYS_syslog] = do_sys_syslog;
    syscall_table[SYS_faccessat] = do_sys_faccessat;
    syscall_table[SYS_ioctl] = do_sys_ioctl;
}