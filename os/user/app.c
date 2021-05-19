#include "usyscall.h"


void AppMain(void)
{
    unsigned long long t; 
    while(1)
    {
        sys_test('a');
        t = 100000000;
        while(t--) {}
    }
}
