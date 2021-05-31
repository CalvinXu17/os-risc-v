/*----------------------------------------------------------------------------
 * Tencent is pleased to support the open source community by making TencentOS
 * available.
 *
 * Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
 * If you have downloaded a copy of the TencentOS binary from Tencent, please
 * note that the TencentOS binary is licensed under the BSD 3-Clause License.
 *
 * If you have downloaded a copy of the TencentOS source code from Tencent,
 * please note that TencentOS source code is licensed under the BSD 3-Clause
 * License, except for the third-party components listed below which are
 * subject to different license terms. Your integration of TencentOS into your
 * own projects may require compliance with the BSD 3-Clause License, as well
 * as the other licenses applicable to the third-party components included
 * within TencentOS.
 *---------------------------------------------------------------------------*/

#ifndef HAL_SD_H
#define HAL_SD_H

#include "type.h"

typedef enum hal_sd_state_en {
    HAL_SD_STAT_RESET,         /*< not yet initialized or disabled */
    HAL_SD_STAT_READY,         /*< initialized and ready for use   */
    HAL_SD_STAT_TIMEOUT,       /*< timeout state                   */
    HAL_SD_STAT_BUSY,          /*< process ongoing                 */
    HAL_SD_STAT_PROGRAMMING,   /*< programming state               */
    HAL_SD_STAT_RECEIVING,     /*< receinving state                */
    HAL_SD_STAT_TRANSFER,      /*< transfert state                 */
    HAL_SD_STAT_ERROR,         /*< error state                     */
} hal_sd_state_t;

typedef struct hal_sd_info_st {
    uint32 card_type;             /*< card type                       */
    uint32 card_version;          /*< card version                    */
    uint32 class;                 /*< card class                      */
    uint32 relative_card_addr;    /*< relative card address           */
    uint32 blk_num;               /*< card capacity in blocks         */
    uint32 blk_size;              /*< one block size in bytes         */
    uint32 logical_blk_num;       /*< card logical capacity in blocks */
    uint32 logical_blk_size;      /*< logical block size in bytes     */
} hal_sd_info_t;

typedef struct hal_sd_st {
    void           *private_sd;
} hal_sd_t;

int hal_sd_init(hal_sd_t *sd);

int hal_sd_read(hal_sd_t *sd, uchar *buf, uint32 start_sector, uint32 nsectors);

int hal_sd_write(hal_sd_t *sd, const uchar *buf, uint32 start_sector, uint32 nsectors);

int hal_sd_read_dma(hal_sd_t *sd, uchar *buf, uint32 start_sector, uint32 nsectors);

int hal_sd_write_dma(hal_sd_t *sd, const uchar *buf, uint32 start_sector, uint32 nsectors);

int hal_sd_erase(hal_sd_t *sd, uint32 blk_add_start, uint32 blk_addr_end);

int hal_sd_info_get(hal_sd_t *sd, hal_sd_info_t *info);

int hal_sd_state_get(hal_sd_t *sd, hal_sd_state_t *state);

int hal_sd_deinit(hal_sd_t *sd);

#endif

