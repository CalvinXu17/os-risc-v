#include "console.h"
#include "sbi.h"
#include "printk.h"
struct ConsoleBuffer consbuf;

void console_init(void)
{
    init_spinlock(&(consbuf.lock), "console_lock");
    consbuf.r_pos = consbuf.w_pos = consbuf.e_pos;
    // init device

}

void console_putc(char c)
{
    if(c == BACKSPACE)
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
    lock(&consbuf.lock);
    int c = serial_handle();
    
    if(c >= 0)
    {
        console_putc(c);
        consbuf.buf[consbuf.w_pos++] = (char)c;
        if(consbuf.w_pos==CONSOLE_BSIZE) consbuf.w_pos = 0;
    }
    unlock(&consbuf.lock);
}

int console_read(void)
{

}

int console_write(void)
{

}