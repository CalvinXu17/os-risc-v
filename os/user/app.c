#include "syscall.h"
#include "unistd.h"
#include "stdio.h"

int sys_test(const char *path)
{
    return __syscall(0, path);
}

char *paths[] = {"/clone",
                "/exit",
                "/fork",
                "/getpid",
                "/getppid",
                "/gettimeofday",
                "/uname",
                "/wait",
                "/sleep",
                "/waitpid",
                "/yield",
                "/times",
                "/getcwd",
                "/chdir",
                "/openat",
                "/open",
                "/read",
                "/write",
                "/close",
                "/mkdir_",
                "/pipe",
                "/dup",
                "/dup2",
                "/getdents",
                "/execve",
                "/brk",
                "/fstat",
                "/unlink",
                "/mount",
                "/umount",
                "/mmap",
                "/munmap"};

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