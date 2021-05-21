#include "console.h"
#include "sbi.h"
#include "printk.h"
#include "sched.h"

struct ConsoleBuffer consbuf;

void console_init(void)
{
    init_spinlock(&(consbuf.mutex), "console_lock");
    init_semephonre(&consbuf.sem, "console_sem", 0);
    consbuf.r_pos = consbuf.w_pos = consbuf.e_pos = 0;
    // init device

}

void console_putc(char c)
{
    if(c == '\b')
    {
        sbi_console_putchar('\b');
        sbi_console_putchar(' ');
        sbi_console_putchar('\b');
    } else
    {
        sbi_console_putchar(c);
    }
}

int serial_handle(void)
{
    int c = sbi_console_getchar();
    if(c < 0) return -1;
    if(c == 127) c = '\b';
    return c;
}

void console_intr(void)
{
    int c = serial_handle();
    
    if(c >= 0)
    {
        lock(&consbuf.mutex);
        consbuf.buf[consbuf.w_pos++] = (char)c;
        consbuf.w_pos %= CONSOLE_BSIZE;
        unlock(&consbuf.mutex);
        V(&consbuf.sem);
    }
}

int console_read(char *s, int len)
{
    int cnt = len;
    while(len > 0)
    {
        P(&consbuf.sem);
        lock(&consbuf.mutex);
        *s = consbuf.buf[consbuf.r_pos++];
        consbuf.r_pos %= CONSOLE_BSIZE;
        unlock(&consbuf.mutex);
        s++;
        len--;
    }
    return cnt;
}

int console_write(const char *s, int len)
{
    lock(&consbuf.mutex);
    char *p;
    for(p=s; p < s + len; p++)
        console_putc(*p);
    unlock(&consbuf.mutex);
    return len;
}