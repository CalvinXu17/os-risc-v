// SPI Protocol Implementation
#include "utils.h"
#include "spi.h"
#include "panic.h"
#include "mm.h"
#include "string.h"

volatile spi_t *const spi[4] =
    {
        (volatile spi_t *)SPI0_V,
        (volatile spi_t *)SPI1_V,
        (volatile spi_t *)SPI_SLAVE_V,
        (volatile spi_t *)SPI2_V};

void spi_init(spi_device_num_t spi_num, spi_work_mode_t work_mode, spi_frame_format_t frame_format,
              uint64 data_bit_length, uint32 endian)
{
    assert(data_bit_length >= 4 && data_bit_length <= 32);
    assert(spi_num < SPI_DEVICE_MAX && spi_num != 2);

    // uchar dfs_offset, frf_offset, work_mode_offset;
    uchar dfs_offset = 0;
    uchar frf_offset = 0;
    uchar work_mode_offset = 0;
    switch(spi_num)
    {
        case 0:
        case 1:
            dfs_offset = 16;
            frf_offset = 21;
            work_mode_offset = 6;
            break;
        case 2:
            // configASSERT(!"Spi Bus 2 Not Support!");
            break;
        case 3:
        default:
            dfs_offset = 0;
            frf_offset = 22;
            work_mode_offset = 8;
            break;
    }

    switch(frame_format)
    {
        case SPI_FF_DUAL:
            // configASSERT(data_bit_length % 2 == 0);
            break;
        case SPI_FF_QUAD:
            // configASSERT(data_bit_length % 4 == 0);
            break;
        case SPI_FF_OCTAL:
            // configASSERT(data_bit_length % 8 == 0);
            break;
        default:
            break;
    }
    volatile spi_t *spi_adapter = spi[spi_num];
    if(spi_adapter->baudr == 0)
        spi_adapter->baudr = SPI_BAUDRATE_DEFAULT_VAL;
    spi_adapter->imr = SPI_INTERRUPT_DISABLE;
    spi_adapter->dmacr = SPI_DMACR_DEFAULT_VAL;
    spi_adapter->dmatdlr = SPI_DMATDLR_DEFAULT_VAL;
    spi_adapter->dmardlr = SPI_DMARDLR_DEFAULT_VAL;
    spi_adapter->ser = SPI_SLAVE_DISABLE;
    spi_adapter->ssienr = SPI_MASTER_DISABLE;
    spi_adapter->ctrlr0 = (work_mode << work_mode_offset) | (frame_format << frf_offset) | ((data_bit_length - 1) << dfs_offset);
    spi_adapter->spi_ctrlr0 = SPI_TMOD_DEFAULT_VAL;
    spi_adapter->endian = endian;
}

static void spi_set_transfer_mode(uchar spi_num, spi_transfer_mode_t tmode)
{
    assert(spi_num < SPI_DEVICE_MAX);

    uchar tmode_offset;
    switch (spi_num)
    {
        case 0:
        case 1:
        case 2:
            tmode_offset = SPI012_TRANSFER_MODE_OFFSET;
            break;
        case 3:
            tmode_offset = SPI3_TRANSFER_MODE_OFFSET;
            break;
        default:
            break;
    }

    set_bits_value_offset(&spi[spi_num]->ctrlr0, 3, tmode, tmode_offset);
}

void spi_send_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uchar *cmd_buff,
                            uint64 cmd_len, const uchar *tx_buff, uint64 tx_len)
{
    assert(spi_num < SPI_DEVICE_MAX && spi_num != 2);
    assert(tx_len != 0);

    // set transfer mode
    spi_set_transfer_mode(spi_num, SPI_TMOD_TRANS);

    // set register status, begin to transfer
    volatile spi_t *spi_controller = spi[spi_num];
    spi_controller->ser = 1U << chip_select;
    spi_controller->ssienr = SPI_MASTER_ENABLE;

    // data transmission
    uint64 fifo_len, i;
    uint32 cur = 0;
    while (tx_len)
    {
        fifo_len = SPI_FIFO_CAPCITY_IN_BYTE - spi_controller->txflr;
        if (tx_len < fifo_len)
            fifo_len = tx_len;

        for (i = 0; i < fifo_len; i++)
            spi_controller->dr[0] = tx_buff[cur++];
        tx_len -= fifo_len;
    }
    // finish sign
    while ((spi_controller->sr & 0x05) != 0x04) ;

    spi_controller->ser = SPI_SLAVE_DISABLE;
    spi_controller->ser = SPI_MASTER_DISABLE;
}

void spi_receive_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uchar *cmd_buff,
                               uint64 cmd_len, uchar *rx_buff, uint64 rx_len)
{
    assert(spi_num < SPI_DEVICE_MAX && spi_num != 2);
    assert(rx_len != 0);

    // set transfer mode
    spi_set_transfer_mode(spi_num, SPI_TMOD_RECV);

    // set register status, begin to transfer
    volatile spi_t *spi_controller = spi[spi_num];
    spi_controller->ctrlr1 = (uint32)(rx_len - 1);
    spi_controller->ssienr = SPI_MASTER_ENABLE;
    spi_controller->dr[0] = 0xffffffff;
    spi_controller->ser = 1U << chip_select;

    // data transmission
    uint64 fifo_len;
    uint32 cur = 0, i;
    while (rx_len)
    {
        fifo_len = spi_controller->rxflr;
        if (rx_len < fifo_len)
            fifo_len = rx_len;

        for (i = 0; i < fifo_len; i++)
            rx_buff[cur++] = spi_controller->dr[0];
        rx_len -= fifo_len;
    }
    spi_controller->ser = SPI_SLAVE_DISABLE;
    spi_controller->ssienr = SPI_MASTER_DISABLE;
}
