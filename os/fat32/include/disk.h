#ifndef DISK_H
#define DISK_H

#include "type.h"

struct disk
{
    uint32 block_size;
    uint32 block_nr;
    int (*open)(struct disk *dsk);
    int (*close)(struct disk *dsk);
    int (*read)(struct disk *dsk, uchar *buf, int start_sector, int cnt);
    int (*write)(struct disk *dsk, uchar *buf, int start_sector, int cnt);
};

int disk_open(struct disk *dsk);
int disk_close(struct disk *dsk);
int disk_read(struct disk *dsk, uchar *buf, int start_sector, int cnt);
int disk_write(struct disk *dsk, uchar *buf, int start_sector, int cnt);
void disk_init(struct disk *dsk);

#endif