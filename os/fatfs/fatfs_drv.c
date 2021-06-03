#include "vfs.h"
#include "hal.h"
#include "ff.h"
#include "diskio.h"

DWORD get_fattime(void)
{
    return 1386283008;
}

static hal_sd_t sd;
static hal_sd_info_t sd_info;

static int sd_open(vfs_inode_t *dev)
{
    if (hal_sd_init(&sd) != 0) {
        return -1;
    }
    return hal_sd_info_get(&sd, &sd_info);
}

static int sd_close(vfs_inode_t *dev)
{
    return hal_sd_deinit(&sd);
}

static uchar sdio_aligned_buffer[512] __attribute__((aligned(4)));

static uint64 sd_read(vfs_inode_t *dev, void *buf, uint64 start_sector, unsigned int nsectors)
{
    int rc = 0;
    uint32 i;
    uchar *buff = (uchar *)buf;
    #ifdef _K210
    if ((uint64)buff % 4 != 0) {
        for (i = 0; i < nsectors; ++i) {
            rc = hal_sd_read(&sd, sdio_aligned_buffer, start_sector + i, 1);
            memcpy(buff, sdio_aligned_buffer, 512);
            buff += 512;
        }
    } else {
        for (i = 0; i < nsectors; ++i) {
            rc = hal_sd_read(&sd, buff, start_sector + i, 1);
            buff += 512;
        }
    }
    #else
    for (i = 0; i < nsectors; ++i)
    {
            rc = hal_sd_read(&sd, sdio_aligned_buffer, start_sector + i, 1);
            memcpy(buff, sdio_aligned_buffer, 512);
            buff += 512;
    }
    #endif
    return rc;
}

static uint64 sd_write(vfs_inode_t *dev, const unsigned char *buf, uint64 start_sector, unsigned int nsectors)
{
    int rc = 0;
    uint32 i;
    uchar *buff = (uchar *)buf;
    #ifdef _K210
    if ((uint64)buff % 4 != 0) {
        for (i = 0; i < nsectors; ++i) {
            memcpy(sdio_aligned_buffer, buff, 512);
            rc = hal_sd_write(&sd, sdio_aligned_buffer, start_sector + i, 1);
            buff += 512;
        }
    } else {
        for (i = 0; i < nsectors; ++i) {
            rc = hal_sd_write(&sd, buff, start_sector + i, 1);
            buff += 512;
        }
    }
    #else
    for (i = 0; i < nsectors; ++i)
    {
        memcpy(sdio_aligned_buffer, buff, 512);
        rc = hal_sd_write(&sd, sdio_aligned_buffer, start_sector + i, 1);
        buff += 512;
    }
    #endif
    return rc;
}

static int sd_ioctl(vfs_inode_t *dev, int cmd, unsigned long arg)
{
    int rc = 0;
    void *buff = (void *)arg;

    if (cmd != CTRL_SYNC && !buff) {
        return -1;
    }

    switch (cmd) {
        case CTRL_SYNC:
            break;

        case GET_SECTOR_SIZE:
            *(DWORD *)buff = sd_info.blk_size;
            break;

        case GET_BLOCK_SIZE:
            *(WORD *)buff = sd_info.blk_size;
            break;

        case GET_SECTOR_COUNT:
            *(DWORD *)buff = sd_info.blk_num;
            break;

        default:
            rc = -1;
            break;
    }

    return rc;
}

static int sd_geometry(vfs_inode_t *dev, vfs_blkdev_geo_t *geo)
{
    hal_sd_info_t sd_info;
    hal_sd_state_t sd_state;

    if (!dev || !geo) {
        return -1;
    }

    if (hal_sd_info_get(&sd, &sd_info) != 0) {
        return -1;
    }

    if (hal_sd_state_get(&sd, &sd_state) != 0) {
        return -1;
    }

    geo->is_available   = (sd_state != HAL_SD_STAT_ERROR);
    geo->sector_size    = sd_info.blk_size;
    geo->nsectors       = sd_info.blk_num;
    return 0;
}


vfs_blkdev_ops_t sd_dev = {
    .open       = sd_open,
    .close      = sd_close,
    .read       = sd_read,
    .write      = sd_write,
    .ioctl      = sd_ioctl,
    .geometry   = sd_geometry,
};

