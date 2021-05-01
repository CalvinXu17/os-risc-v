#ifndef CONSOLE_H
#define CONSOLE_H

#include "type.h"
#include "spinlock.h"

#define CONSOLE_BSIZE   512
#define BACKSPACE       0x100

struct ConsoleBuffer
{
    char buf[CONSOLE_BSIZE];
    uint32 r_pos;
    uint32 w_pos;
    uint32 e_pos;
    spinlock lock;
};

void console_init(void);
void console_putc(char c);
void console_intr(void);

#endif