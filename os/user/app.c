#include "syscall.h"
#include "unistd.h"
#include "stdio.h"

int sys_test(const char *path)
{
    return __syscall(0, path);
}

char *paths[] = {"/root/clone",
                "/root/exit",
                "/root/fork",
                "/root/getpid",
                "/root/getppid",
                "/root/gettimeofday",
                "/root/uname",
                "/root/wait",
                "/root/sleep",
                "/root/waitpid",
                "/root/yield",
                "/root/times",
                "/root/getcwd",
                "/root/chdir",
                "/root/openat",
                "/root/open",
                "/root/read",
                "/root/write",
                "/root/close",
                "/root/mkdir_",
                "/root/pipe",
                "/root/dup",
                "/root/dup2",
                "/root/getdents",
                "/root/execve",
                "/root/brk",
                "/root/fstat",
                "/root/unlink",
                "/root/mount",
                "/root/umount",
                "/root/mmap",
                "/root/munmap"};

int main(void)
{
    int i;
    for(i = 0; i < sizeof(paths); i++)
    {
        int pid = sys_test(paths[i]);
        if(pid > 0)
        {
            waitpid(pid, 0, 0);
        }
    }
    
    return 0;
}