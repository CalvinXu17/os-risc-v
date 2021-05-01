#ifndef DEVICE_H
#define DEVICE_H

#include "type.h"
#include "iobuf.h"

struct device
{
    uint32 blocks;
    uint32 block_size;    
    int (*open)(device *dev, uint64 flags);
    int (*close)(device *dev);
    int (*read)(device *dev, struct iobuf *buf);
};

#endif