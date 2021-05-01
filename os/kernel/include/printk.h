#ifndef PRINTK_H
#define PRINTK_H

#include "sbi.h"
#include "console.h"

typedef __builtin_va_list va_list;
#define va_start(ap, last)      (__builtin_va_start(ap, last))
#define va_arg(ap, type)        (__builtin_va_arg(ap, type))
#define va_end(ap)

static inline void putc(char ch)
{
    console_putc(ch);
}

static inline int puts(const char *s)
{
    int cnt=0;
    while(*s != '\0')
    {
        putc(*s);
        cnt++;
        s++;
    }
    return cnt;
}
int printk(const char *fmt, ...);

#endif