#include "disk.h"
#include "sdcard_test.h"

int disk_open(struct disk *dsk)
{
    return 1;
}
int disk_close(struct disk *dsk)
{
    return 1;
}

int disk_read(struct disk *dsk, uchar *buf, int start_sector, int cnt)
{
    if(!sd_read_sector(buf, start_sector, cnt))
        return 1;
    else return 0;
}
int disk_write(struct disk *dsk, uchar *buf, int start_sector, int cnt)
{
    if(!sd_write_sector(buf, start_sector, cnt))
        return 1;
    else return 0;
}

void disk_init(struct disk *dsk)
{
    dsk->block_size = 512;
    dsk->open = disk_open;
    dsk->close = disk_close;
    dsk->read = disk_read;
    dsk->write = disk_write;
}
