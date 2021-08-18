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
    struct Process *proc = getcpu()->cur_proc;
    while(len > 0)
    {
        P(&consbuf.sem);
        if(proc->receive_kill > 0)
            break;
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

static int vfs_console_open(vfs_file_t *file)
{
    return 0;
}

static int vfs_console_close(vfs_file_t *file)
{
    return 0;
}

static uint64 vfs_console_read(vfs_file_t *file, void *buf, uint64 count)
{
    return console_read(buf, count);
}

static uint64 vfs_console_write(vfs_file_t *file, const void *buf, uint64 count)
{
    return console_write(buf, count);
}

vfs_chrdev_ops_t stdin = {
    .open = vfs_console_open,
    .close = vfs_console_close,
    .read = vfs_console_read,
};

vfs_chrdev_ops_t stdout = {
    .open = vfs_console_open,
    .close = vfs_console_close,
    .write = vfs_console_write,
};

vfs_chrdev_ops_t stderr = {
    .open = vfs_console_open,
    .close = vfs_console_close,
    .write = vfs_console_write,
};