#include "syscall.h"
#include "stdio.h"
#include "unistd.h"

int sys_test(int c)
{
    return syscall(SYS_test, c);
}

int main(void)
{
    while(1)
    {
        printf("return value is %d\n", sleep(3));
    }
    return 0;
}
