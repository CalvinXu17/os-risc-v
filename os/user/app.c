#include "unistd.h"

char *argvs1[] = {"./busybox", "sh", "./lua_testcode.sh", NULL};
char *argvs2[] = {"./busybox", "sh", "./busybox_testcode.sh", NULL};
char *envps[] = {"SHELL=shell", "PWD=/root", "HOME=/root", "USER=root", "SHLVL=1", "PATH=/root", "OLDPWD=/root", "_=busybox", NULL};

int main(int argc, char *argv)
{
    int pid = fork();
    if(pid)
    {
        waitpid(pid, 0, 0);
        execve("./busybox", argvs2, envps);
    } else {
        execve("./busybox", argvs1, envps);
    }
    
    return 0;
}