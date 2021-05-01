#ifndef IOBUF_H
#define IOBUF_H

#include "type.h"

struct iobuf
{
    void *addr;
    uint64 offset;
    uint64 len;
    
};

#endif