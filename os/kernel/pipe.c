#include "pipe.h"
#include "kmalloc.h"

pipe_t* pipe_alloc(void)
{
    pipe_t *p = kmalloc(sizeof(pipe_t));
    if(!p) return NULL;
    init_spinlock(&p->mutex, "pipe_lock");
    init_semephonre(&p->sem, "pipe_sem", 0);
    p->r_pos = p->w_pos = 0;
    p->r_ref = p->w_ref = 1;
    return p;
}

uint64 pipe_read(pipe_t *pip, char *buf, uint64 count)
{
    int cnt = 0;
    while(count > 0)
    {
        lock(&pip->mutex);
        if(pip->w_ref == 0 && pip->r_pos == pip->w_pos)
        {
            unlock(&pip->mutex);
            break;
        }
        unlock(&pip->mutex);
        P(&pip->sem);
        lock(&pip->mutex);
        *buf = pip->buf[pip->r_pos++];
        pip->r_pos %= PIPE_BUFF_SIZE;
        unlock(&pip->mutex);
        buf++;
        count--;
        cnt++;
    }
    return cnt;
}

uint64 pipe_write(pipe_t *pip, const char *buf, uint64 count)
{
    int cnt = 0;
    while(count > 0)
    {
        lock(&pip->mutex);
        pip->buf[pip->w_pos++] = *buf;
        pip->w_pos %= PIPE_BUFF_SIZE;
        V(&pip->sem);
        buf++;
        cnt++;
        count--;
        if(pip->r_ref == 0)
        {
            unlock(&pip->mutex);
            break;
        }
        unlock(&pip->mutex);
    }
    return cnt;
}

int pipe_close(pipe_t *pip, ufile_type_t type)
{
    int rt = 0;
    lock(&pip->mutex);
    if(type == UTYPE_PIPEIN)
    {
        if(pip->r_ref)
            pip->r_ref--;
        else rt = -1;
    } else if(type == UTYPE_PIPEOUT)
    {
        if(pip->w_ref)
            pip->w_ref--;
        else rt = -1;
    }

    if(!pip->r_ref && !pip->w_ref)
    {
        unlock(&pip->mutex);
        kfree(pip);
    } else unlock(&pip->mutex);
    return rt;
}
