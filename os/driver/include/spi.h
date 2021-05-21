/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef DRIVER_SPI_H
#define DRIVER_SPI_H

#include "type.h"

#define SPI01_WORK_MODE_OFFSET              6
#define SPI012_TRANSFER_MODE_OFFSET         8
#define SPI01_DATA_BIT_LENGTH_OFFSET        16
#define SPI01_FRAME_FORMAT_OFFSET           21

#define SPI3_WORK_MODE_OFFSET               8
#define SPI3_TRANSFER_MODE_OFFSET           10
#define SPI3_DATA_BIT_LENGTH_OFFSET         0
#define SPI3_FRAME_FORMAT_OFFSET            22

#define SPI_DATA_BIT_LENGTH_BIT             5
#define SPI_MIN_DATA_BIT_LENGTH             4
#define SPI_MAX_DATA_BIT_LENGTH             (1 << SPI_DATA_BIT_LENGTH_BIT)

#define SPI_BAUDRATE_DEFAULT_VAL            (0x14)
#define SPI_INTERRUPT_DISABLE               (0x00)
#define SPI_DMACR_DEFAULT_VAL               (0x00)
#define SPI_DMATDLR_DEFAULT_VAL             (0x00)
#define SPI_DMARDLR_DEFAULT_VAL             (0x00)
#define SPI_SLAVE_DISABLE                   (0x00)
#define SPI_MASTER_DISABLE                  (0x00)
#define SPI_MASTER_ENABLE                   (0x01)
#define SPI_TMOD_DEFAULT_VAL                0

#define SPI_FIFO_CAPCITY_IN_BYTE            (32)

/* clang-format off */
typedef struct _spi
{
    /* SPI Control Register 0                                    (0x00)*/
    volatile uint32 ctrlr0;
    /* SPI Control Register 1                                    (0x04)*/
    volatile uint32 ctrlr1;
    /* SPI Enable Register                                       (0x08)*/
    volatile uint32 ssienr;
    /* SPI Microwire Control Register                            (0x0c)*/
    volatile uint32 mwcr;
    /* SPI Slave Enable Register                                 (0x10)*/
    volatile uint32 ser;
    /* SPI Baud Rate Select                                      (0x14)*/
    volatile uint32 baudr;
    /* SPI Transmit FIFO Threshold Level                         (0x18)*/
    volatile uint32 txftlr;
    /* SPI Receive FIFO Threshold Level                          (0x1c)*/
    volatile uint32 rxftlr;
    /* SPI Transmit FIFO Level Register                          (0x20)*/
    volatile uint32 txflr;
    /* SPI Receive FIFO Level Register                           (0x24)*/
    volatile uint32 rxflr;
    /* SPI Status Register                                       (0x28)*/
    volatile uint32 sr;
    /* SPI Interrupt Mask Register                               (0x2c)*/
    volatile uint32 imr;
    /* SPI Interrupt Status Register                             (0x30)*/
    volatile uint32 isr;
    /* SPI Raw Interrupt Status Register                         (0x34)*/
    volatile uint32 risr;
    /* SPI Transmit FIFO Overflow Interrupt Clear Register       (0x38)*/
    volatile uint32 txoicr;
    /* SPI Receive FIFO Overflow Interrupt Clear Register        (0x3c)*/
    volatile uint32 rxoicr;
    /* SPI Receive FIFO Underflow Interrupt Clear Register       (0x40)*/
    volatile uint32 rxuicr;
    /* SPI Multi-Master Interrupt Clear Register                 (0x44)*/
    volatile uint32 msticr;
    /* SPI Interrupt Clear Register                              (0x48)*/
    volatile uint32 icr;
    /* SPI DMA Control Register                                  (0x4c)*/
    volatile uint32 dmacr;
    /* SPI DMA Transmit Data Level                               (0x50)*/
    volatile uint32 dmatdlr;
    /* SPI DMA Receive Data Level                                (0x54)*/
    volatile uint32 dmardlr;
    /* SPI Identification Register                               (0x58)*/
    volatile uint32 idr;
    /* SPI DWC_ssi component version                             (0x5c)*/
    volatile uint32 ssic_version_id;
    /* SPI Data Register 0-36                                    (0x60 -- 0xec)*/
    volatile uint32 dr[36];
    /* SPI RX Sample Delay Register                              (0xf0)*/
    volatile uint32 rx_sample_delay;
    /* SPI SPI Control Register                                  (0xf4)*/
    volatile uint32 spi_ctrlr0;
    /* reserved                                                  (0xf8)*/
    volatile uint32 resv;
    /* SPI XIP Mode bits                                         (0xfc)*/
    volatile uint32 xip_mode_bits;
    /* SPI XIP INCR transfer opcode                              (0x100)*/
    volatile uint32 xip_incr_inst;
    /* SPI XIP WRAP transfer opcode                              (0x104)*/
    volatile uint32 xip_wrap_inst;
    /* SPI XIP Control Register                                  (0x108)*/
    volatile uint32 xip_ctrl;
    /* SPI XIP Slave Enable Register                             (0x10c)*/
    volatile uint32 xip_ser;
    /* SPI XIP Receive FIFO Overflow Interrupt Clear Register    (0x110)*/
    volatile uint32 xrxoicr;
    /* SPI XIP time out register for continuous transfers        (0x114)*/
    volatile uint32 xip_cnt_time_out;
    volatile uint32 endian;
} __attribute__((packed, aligned(4))) spi_t;
/* clang-format on */

typedef enum _spi_device_num
{
    SPI_DEVICE_0,
    SPI_DEVICE_1,
    SPI_DEVICE_2,
    SPI_DEVICE_3,
    SPI_DEVICE_MAX,
} spi_device_num_t;

typedef enum _spi_work_mode
{
    SPI_WORK_MODE_0,
    SPI_WORK_MODE_1,
    SPI_WORK_MODE_2,
    SPI_WORK_MODE_3,
} spi_work_mode_t;

typedef enum _spi_frame_format
{
    SPI_FF_STANDARD,
    SPI_FF_DUAL,
    SPI_FF_QUAD,
    SPI_FF_OCTAL
} spi_frame_format_t;

typedef enum _spi_instruction_address_trans_mode
{
    SPI_AITM_STANDARD,
    SPI_AITM_ADDR_STANDARD,
    SPI_AITM_AS_FRAME_FORMAT
} spi_instruction_address_trans_mode_t;

typedef enum _spi_transfer_mode
{
    SPI_TMOD_TRANS_RECV,
    SPI_TMOD_TRANS,
    SPI_TMOD_RECV,
    SPI_TMOD_EEROM
} spi_transfer_mode_t;

typedef enum _spi_transfer_width
{
    SPI_TRANS_CHAR = 0x1,
    SPI_TRANS_SHORT = 0x2,
    SPI_TRANS_INT = 0x4,
} spi_transfer_width_t;

typedef enum _spi_chip_select
{
    SPI_CHIP_SELECT_0,
    SPI_CHIP_SELECT_1,
    SPI_CHIP_SELECT_2,
    SPI_CHIP_SELECT_3,
    SPI_CHIP_SELECT_MAX,
} spi_chip_select_t;

typedef enum
{
    WRITE_CONFIG,
    READ_CONFIG,
    WRITE_DATA_BYTE,
    READ_DATA_BYTE,
    WRITE_DATA_BLOCK,
    READ_DATA_BLOCK,
} spi_slave_command_e;

typedef struct
{
    uchar cmd;
    uchar err;
    uint32 addr;
    uint32 len;
} spi_slave_command_t;

typedef enum
{
    IDLE,
    COMMAND,
    TRANSFER,
} spi_slave_status_e;


extern volatile spi_t *const spi[4];

/**
 * @brief       Set spi configuration
 *
 * @param[in]   spi_num             Spi bus number
 * @param[in]   mode                Spi mode
 * @param[in]   frame_format        Spi frame format
 * @param[in]   data_bit_length     Spi data bit length
 * @param[in]   endian              0:little-endian 1:big-endian
 *
 * @return      Void
 */
void spi_init(spi_device_num_t spi_num, spi_work_mode_t work_mode, spi_frame_format_t frame_format,
              uint64 data_bit_length, uint32 endian);

/**
 * @brief       Spi send data
 *
 * @param[in]   spi_num         Spi bus number
 * @param[in]   chip_select     Spi chip select
 * @param[in]   cmd_buff        Spi command buffer point
 * @param[in]   cmd_len         Spi command length
 * @param[in]   tx_buff         Spi transmit buffer point
 * @param[in]   tx_len          Spi transmit buffer length
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void spi_send_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uchar *cmd_buff,
                            uint64 cmd_len, const uchar *tx_buff, uint64 tx_len);

/**
 * @brief       Spi receive data
 *
 * @param[in]   spi_num             Spi bus number
 * @param[in]   chip_select         Spi chip select
 * @param[in]   cmd_buff            Spi command buffer point
 * @param[in]   cmd_len             Spi command length
 * @param[in]   rx_buff             Spi receive buffer point
 * @param[in]   rx_len              Spi receive buffer length
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void spi_receive_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uchar *cmd_buff,
                               uint64 cmd_len, uchar *rx_buff, uint64 rx_len);


#endif /* _DRIVER_SPI_H */
