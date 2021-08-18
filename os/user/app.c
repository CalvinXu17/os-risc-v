#include "unistd.h"

char *argvs[] = {"./busybox", "sh", "./busybox_testcode.sh", NULL};
char *envps[] = {"SHELL=shell", "PWD=/root", "HOME=/root", "USER=root", "SHLVL=1", "PATH=/root", "OLDPWD=/root", "_=busybox", NULL};

int main(int argc, char *argv)
{
    execve("./busybox", argvs, envps);
    return 0;
}