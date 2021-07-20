#ifndef PIPE_H
#define PIPE_H

#include "type.h"
#include "spinlock.h"
#include "sem.h"
#include "fs.h"

#define PIPE_BUFF_SIZE  1024

typedef struct pipe {
    spinlock mutex;
    struct semephore sem;
    uchar buf[PIPE_BUFF_SIZE];
    int r_pos;
    int w_pos;
    int r_ref;
    int w_ref;
} pipe_t;

pipe_t* pipe_alloc(void);
uint64 pipe_read(pipe_t *pip, char *buf, uint64 count);
uint64 pipe_write(pipe_t *pip, const char *buf, uint64 count);
int pipe_close(pipe_t *pip, ufile_type_t type);

#endif