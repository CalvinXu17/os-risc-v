#include "hal_sd.h"
#include "sdcard.h"

extern SD_CardInfo cardinfo;

int hal_sd_init(hal_sd_t *sd)
{
    if(!sdcard_init())
    {
        return 0;
    } else return 1;
}

int hal_sd_read(hal_sd_t *sd, uchar *buf, uint32 start_sector, uint32 nsectors)
{
    if(!sd_read_sector(buf, start_sector, nsectors))
    {
        return 0;
    } else return 1;
}

int hal_sd_write(hal_sd_t *sd, const uchar *buf, uint32 start_sector, uint32 nsectors)
{
    if(!sd_write_sector(buf, start_sector, nsectors))
    {
        return 0;
    } else return 1;
}

int hal_sd_read_dma(hal_sd_t *sd, uchar *buf, uint32 blk_addr, uint32 blk_num)
{
    return 0;
}

int hal_sd_write_dma(hal_sd_t *sd, const uchar *buf, uint32 blk_addr, uint32 blk_num)
{
    return 0;
}

int hal_sd_erase(hal_sd_t *sd, uint32 blk_add_start, uint32 blk_addr_end)
{
    return 0;
}

int hal_sd_info_get(hal_sd_t *sd, hal_sd_info_t *info)
{
    info->blk_size = cardinfo.CardBlockSize;
    info->blk_num = cardinfo.CardCapacity / cardinfo.CardBlockSize;
    info->logical_blk_size = info->blk_size;
    info->logical_blk_num = info->blk_num;
    info->relative_card_addr = 0;
    return 0;
}

int hal_sd_state_get(hal_sd_t *sd, hal_sd_state_t *state)
{
    *state = HAL_SD_STAT_READY;
    return 0;
}

int hal_sd_deinit(hal_sd_t *sd)
{
    return 0;
}