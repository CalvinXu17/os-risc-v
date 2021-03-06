#include "sdcard.h"
#include "spi.h"
#include "fpioa.h"
#include "gpiohs.h"
#include "printk.h"
#include "sysctl.h"

SD_CardInfo cardinfo;

uchar sdcard_init()
{
	fpioa_pin_init();
	return sd_init();
}

uint32 spi_set_clk_rate(spi_device_num_t spi_num, uint32 spi_clk)
{
    uint32 spi_baudr = sysctl_clock_get_freq(SYSCTL_CLOCK_SPI0 + spi_num) / spi_clk;
    if (spi_baudr < 2)
        spi_baudr = 2;
    else if (spi_baudr > 65534)
        spi_baudr = 65534;
    volatile spi_t *spi_controller = spi[spi_num];
    spi_controller->baudr = spi_baudr;
    return sysctl_clock_get_freq(SYSCTL_CLOCK_SPI0 + spi_num) / spi_clk;
}

void SD_CS_HIGH(void)
{
    gpiohs_set_pin(7, GPIO_PV_HIGH);
}

void SD_CS_LOW(void)
{
    gpiohs_set_pin(7, GPIO_PV_LOW);
}

void SD_HIGH_SPEED_ENABLE(void)
{
    spi_set_clk_rate(SPI_DEVICE_0, 10000000);
}

static void sd_lowlevel_init(uchar spi_index)
{
    gpiohs_set_drive_mode(7, GPIO_DM_OUTPUT);
}

// TODO: 保持与评测机一致，为SPI_CHIP_SELECT_0而不是SPI_CHIP_SELECT_3
static void sd_write_data(uchar *data_buff, uint32 length)
{
    spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    spi_send_data_standard(SPI_DEVICE_0, SPI_CHIP_SELECT_0, NULL, 0, data_buff, length);
}

static void sd_read_data(uchar *data_buff, uint32 length)
{
    spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    spi_receive_data_standard(SPI_DEVICE_0, SPI_CHIP_SELECT_0, NULL, 0, data_buff, length);
}

/*
 * @brief  Send 5 bytes command to the SD card.
 * @param  Cmd: The user expected command to send to SD card.
 * @param  Arg: The command argument.
 * @param  Crc: The CRC.
 * @retval None
 */
static void sd_send_cmd(uchar cmd, uint32 arg, uchar crc)
{
	uchar frame[6];
	/*!< Construct byte 1 */
	frame[0] = (cmd | 0x40);
	/*!< Construct byte 2 */
	frame[1] = (uchar)(arg >> 24);
	/*!< Construct byte 3 */
	frame[2] = (uchar)(arg >> 16);
	/*!< Construct byte 4 */
	frame[3] = (uchar)(arg >> 8);
	/*!< Construct byte 5 */
	frame[4] = (uchar)(arg);
	/*!< Construct CRC: byte 6 */
	frame[5] = (crc);
	/*!< SD chip select low */
	SD_CS_LOW();
	/*!< Send the Cmd bytes */
	sd_write_data(frame, 6);
}

/*
 * @brief  Send 5 bytes command to the SD card.
 * @param  Cmd: The user expected command to send to SD card.
 * @param  Arg: The command argument.
 * @param  Crc: The CRC.
 * @retval None
 */
static void sd_end_cmd(void)
{
	uchar frame[1] = {0xFF};
	/*!< SD chip select high */
	SD_CS_HIGH();
	/*!< Send the Cmd bytes */
	sd_write_data(frame, 1);
}

static uchar sd_get_response_R1()
{
    uchar result;
    uint16 timeout = 0xFFF;
    while (timeout--)
    {
        sd_read_data(&result, 1);
        if (result != SD_EMPTY_FILL)
            return result;
    }
    return 0xFF;
}

/* 
 * Read the rest of R3 response 
 * Be noticed: frame should be at least 4-byte long 
 */
static void sd_get_response_R3_rest(uchar *frame)
{
	sd_read_data(frame, 4);
}

/* 
 * Read the rest of R7 response 
 * Be noticed: frame should be at least 4-byte long 
 */
static void sd_get_response_R7_rest(uchar *frame)
{
	sd_read_data(frame, 4);
}

/*
 * @brief  Get SD card data response.
 * @param  None
 * @retval The SD status: Read data response xxx0<status>1
 *         - status 010: Data accecpted
 *         - status 101: Data rejected due to a crc error
 *         - status 110: Data rejected due to a Write error.
 *         - status 111: Data rejected due to other error.
 */

static void detect_busy(void)
{
	uchar response;
	sd_read_data(&response, 1);
	while (response != 0xff)
		sd_read_data(&response, 1);
	/*!< Return response */
}

static uchar sd_get_data_write_response(void)
{
	uchar response;
	int timeout = 0xfff;
	/*!< Read resonse */
	while (--timeout) {
		sd_read_data(&response, 1);
		response &= 0x1F;
		if (response == 0x05)
			break;
	}

	if (0 == timeout) {
		#ifdef _DEBUG
		printk("WAITREAD END failed\n");
		#endif
		return 0xff;
	}
	/*!< Wait null data */
	detect_busy();
	return 0;
}

static int switch_to_SPI_mode(void)
{
	int timeout = 0xff;

	while (--timeout) {
		sd_send_cmd(SD_CMD0, 0, 0x95);
		uint64 result = sd_get_response_R1();
		sd_end_cmd();

		if (0x01 == result) break;
	}
	if (0 == timeout) {
		#ifdef _DEBUG
		printk("SD_CMD0 failed\n");
		#endif
		return 0xff;
	}
	return 0;
}

// verify supply voltage range 
static int verify_operation_condition(void) {
	uint64 result;

	// Stores the response reversely. 
	// That means 
	// frame[2] - VCA 
	// frame[3] - Check Pattern 
	uchar frame[4];

	sd_send_cmd(SD_CMD8, 0x01aa, 0x87);
	result = sd_get_response_R1();
	sd_get_response_R7_rest(frame);
	sd_end_cmd();

	if (0x09 == result) {
		#ifdef _DEBUG
		printk("invalid CRC for CMD8\n");
		#endif
		return 0xff;
	}
	else if (0x01 == result && 0x01 == (frame[2] & 0x0f) && 0xaa == frame[3]) {
		return 0x00;
	}
	#ifdef _DEBUG
	printk("verify_operation_condition() fail!\n");
	#endif
	return 0xff;
}

// read OCR register to check if the voltage range is valid 
// this step is not mandotary, but I advise to use it 
static int read_OCR(void)
{
	uint64 result;
	uchar ocr[4];

	int timeout;

	timeout = 0xff;
	while (--timeout) {
		sd_send_cmd(SD_CMD58, 0, 0);
		result = sd_get_response_R1();
		sd_get_response_R3_rest(ocr);
		sd_end_cmd();

		if (
			0x01 == result && // R1 response in idle status 
			(ocr[1] & 0x1f) && (ocr[2] & 0x80) 	// voltage range valid 
		) {
			return 0;
		}
	}

	// timeout!
	#ifdef _DEBUG
	printk("read_OCR() timeout!\n");
	printk("result = %d\n", result);
	#endif
	return 0xff;
}

// send ACMD41 to tell sdcard to finish initializing 
static int set_SDXC_capacity(void)
{
	uchar result = 0xff;

	int timeout = 0xfff;
	while (--timeout) {
		sd_send_cmd(SD_CMD55, 0, 0);
		result = sd_get_response_R1();
		sd_end_cmd();
		if (0x01 != result) {
			#ifdef _DEBUG
			printk("SD_CMD55 fail! result = %d\n", result);
			#endif
			return 0xff;
		}

		sd_send_cmd(SD_ACMD41, 0x40000000, 0);
		result = sd_get_response_R1();
		sd_end_cmd();
		if (0 == result) {
			return 0;
		}
	}

	// timeout! 
	#ifdef _DEBUG
	printk("set_SDXC_capacity() timeout!\n");
	printk("result = %d\n", result);
	#endif
	return 0xff;
}

// Used to differ whether sdcard is SDSC type. 
static int is_standard_sd = 0;
// check OCR register to see the type of sdcard, 
// thus determine whether block size is suitable to buffer size
static int check_block_size(void)
{
	uchar result = 0xff;
	uchar ocr[4];

	int timeout = 0xff;
	while (timeout --) {
		sd_send_cmd(SD_CMD58, 0, 0);
		result = sd_get_response_R1();
		sd_get_response_R3_rest(ocr);
		sd_end_cmd();

		if (0 == result) {
			if (ocr[0] & 0x40) {
				#ifdef _DEBUG
				printk("SDHC/SDXC detected\n");
				#endif
				is_standard_sd = 0;
			}
			else {
				#ifdef _DEBUG
				printk("SDSC detected, setting block size\n");
				#endif
				// setting SD card block size to BSIZE 
				int timeout = 0xff;
				int result = 0xff;
				while (--timeout) {
					sd_send_cmd(SD_CMD16, 512, 0);
					result = sd_get_response_R1();
					sd_end_cmd();

					if (0 == result) break;
				}
				if (0 == timeout) {
					#ifdef _DEBUG
					printk("check_OCR(): fail to set block size");
					#endif
					return 0xff;
				}
				is_standard_sd = 1;
			}
			return 0;
		}
	}

	// timeout! 
	#ifdef _DEBUG
	printk("check_OCR() timeout!\n");
	printk("result = %d\n", result);
	#endif
	return 0xff;
}

/*
 * @brief  Read the CSD card register
 *         Reading the contents of the CSD register in SPI mode is a simple
 *         read-block transaction.
 * @param  SD_csd: pointer on an SCD register structure
 * @retval The SD Response:
 *         - 0xFF: Sequence failed
 *         - 0: Sequence succeed
 */
static uchar sd_get_csdregister(SD_CSD *SD_csd)
{
	uchar csd_tab[18];
	/*!< Send CMD9 (CSD register) or CMD10(CSD register) */
	sd_send_cmd(SD_CMD9, 0, 0);
	/*!< Wait for response in the R1 format (0x00 is no errors) */
	if (sd_get_response_R1() != 0x00) {
		sd_end_cmd();
		return 0xFF;
	}
	if (sd_get_response_R1() != SD_START_DATA_SINGLE_BLOCK_READ) {
		sd_end_cmd();
		return 0xFF;
	}
	/*!< Store CSD register value on csd_tab */
	/*!< Get CRC bytes (not really needed by us, but required by SD) */
	sd_read_data(csd_tab, 18);
	sd_end_cmd();
	/*!< Byte 0 */
	SD_csd->CSDStruct = (csd_tab[0] & 0xC0) >> 6;
	SD_csd->SysSpecVersion = (csd_tab[0] & 0x3C) >> 2;
	SD_csd->Reserved1 = csd_tab[0] & 0x03;
	/*!< Byte 1 */
	SD_csd->TAAC = csd_tab[1];
	/*!< Byte 2 */
	SD_csd->NSAC = csd_tab[2];
	/*!< Byte 3 */
	SD_csd->MaxBusClkFrec = csd_tab[3];
	/*!< Byte 4 */
	SD_csd->CardComdClasses = csd_tab[4] << 4;
	/*!< Byte 5 */
	SD_csd->CardComdClasses |= (csd_tab[5] & 0xF0) >> 4;
	SD_csd->RdBlockLen = csd_tab[5] & 0x0F;
	/*!< Byte 6 */
	SD_csd->PartBlockRead = (csd_tab[6] & 0x80) >> 7;
	SD_csd->WrBlockMisalign = (csd_tab[6] & 0x40) >> 6;
	SD_csd->RdBlockMisalign = (csd_tab[6] & 0x20) >> 5;
	SD_csd->DSRImpl = (csd_tab[6] & 0x10) >> 4;
	SD_csd->Reserved2 = 0; /*!< Reserved */
	SD_csd->DeviceSize = (csd_tab[6] & 0x03) << 10;
	/*!< Byte 7 */
	SD_csd->DeviceSize = (csd_tab[7] & 0x3F) << 16;
	/*!< Byte 8 */
	SD_csd->DeviceSize |= csd_tab[8] << 8;
	/*!< Byte 9 */
	SD_csd->DeviceSize |= csd_tab[9];
	/*!< Byte 10 */
	SD_csd->EraseGrSize = (csd_tab[10] & 0x40) >> 6;
	SD_csd->EraseGrMul = (csd_tab[10] & 0x3F) << 1;
	/*!< Byte 11 */
	SD_csd->EraseGrMul |= (csd_tab[11] & 0x80) >> 7;
	SD_csd->WrProtectGrSize = (csd_tab[11] & 0x7F);
	/*!< Byte 12 */
	SD_csd->WrProtectGrEnable = (csd_tab[12] & 0x80) >> 7;
	SD_csd->ManDeflECC = (csd_tab[12] & 0x60) >> 5;
	SD_csd->WrSpeedFact = (csd_tab[12] & 0x1C) >> 2;
	SD_csd->MaxWrBlockLen = (csd_tab[12] & 0x03) << 2;
	/*!< Byte 13 */
	SD_csd->MaxWrBlockLen |= (csd_tab[13] & 0xC0) >> 6;
	SD_csd->WriteBlockPaPartial = (csd_tab[13] & 0x20) >> 5;
	SD_csd->Reserved3 = 0;
	SD_csd->ContentProtectAppli = (csd_tab[13] & 0x01);
	/*!< Byte 14 */
	SD_csd->FileFormatGrouop = (csd_tab[14] & 0x80) >> 7;
	SD_csd->CopyFlag = (csd_tab[14] & 0x40) >> 6;
	SD_csd->PermWrProtect = (csd_tab[14] & 0x20) >> 5;
	SD_csd->TempWrProtect = (csd_tab[14] & 0x10) >> 4;
	SD_csd->FileFormat = (csd_tab[14] & 0x0C) >> 2;
	SD_csd->ECC = (csd_tab[14] & 0x03);
	/*!< Byte 15 */
	SD_csd->CSD_CRC = (csd_tab[15] & 0xFE) >> 1;
	SD_csd->Reserved4 = 1;
	/*!< Return the reponse */
	return 0;
}

/*
 * @brief  Read the CID card register.
 *         Reading the contents of the CID register in SPI mode is a simple
 *         read-block transaction.
 * @param  SD_cid: pointer on an CID register structure
 * @retval The SD Response:
 *         - 0xFF: Sequence failed
 *         - 0: Sequence succeed
 */
static uchar sd_get_cidregister(SD_CID *SD_cid)
{
	uchar cid_tab[18];
	/*!< Send CMD10 (CID register) */
	sd_send_cmd(SD_CMD10, 0, 0);
	/*!< Wait for response in the R1 format (0x00 is no errors) */
	if (sd_get_response_R1() != 0x00) {
		sd_end_cmd();
		return 0xFF;
	}
	if (sd_get_response_R1() != SD_START_DATA_SINGLE_BLOCK_READ) {
		sd_end_cmd();
		return 0xFF;
	}
	/*!< Store CID register value on cid_tab */
	/*!< Get CRC bytes (not really needed by us, but required by SD) */
	sd_read_data(cid_tab, 18);
	sd_end_cmd();
	/*!< Byte 0 */
	SD_cid->ManufacturerID = cid_tab[0];
	/*!< Byte 1 */
	SD_cid->OEM_AppliID = cid_tab[1] << 8;
	/*!< Byte 2 */
	SD_cid->OEM_AppliID |= cid_tab[2];
	/*!< Byte 3 */
	SD_cid->ProdName1 = cid_tab[3] << 24;
	/*!< Byte 4 */
	SD_cid->ProdName1 |= cid_tab[4] << 16;
	/*!< Byte 5 */
	SD_cid->ProdName1 |= cid_tab[5] << 8;
	/*!< Byte 6 */
	SD_cid->ProdName1 |= cid_tab[6];
	/*!< Byte 7 */
	SD_cid->ProdName2 = cid_tab[7];
	/*!< Byte 8 */
	SD_cid->ProdRev = cid_tab[8];
	/*!< Byte 9 */
	SD_cid->ProdSN = cid_tab[9] << 24;
	/*!< Byte 10 */
	SD_cid->ProdSN |= cid_tab[10] << 16;
	/*!< Byte 11 */
	SD_cid->ProdSN |= cid_tab[11] << 8;
	/*!< Byte 12 */
	SD_cid->ProdSN |= cid_tab[12];
	/*!< Byte 13 */
	SD_cid->Reserved1 |= (cid_tab[13] & 0xF0) >> 4;
	SD_cid->ManufactDate = (cid_tab[13] & 0x0F) << 8;
	/*!< Byte 14 */
	SD_cid->ManufactDate |= cid_tab[14];
	/*!< Byte 15 */
	SD_cid->CID_CRC = (cid_tab[15] & 0xFE) >> 1;
	SD_cid->Reserved2 = 1;
	/*!< Return the reponse */
	return 0;
}

/*
 * @brief  Returns information about specific card.
 * @param  cardinfo: pointer to a SD_CardInfo structure that contains all SD
 *         card information.
 * @retval The SD Response:
 *         - 0xFF: Sequence failed
 *         - 0: Sequence succeed
 */
static uchar sd_get_cardinfo(SD_CardInfo *cardinfo)
{
	if (sd_get_csdregister(&(cardinfo->SD_csd)))
		return 0xFF;
	if (sd_get_cidregister(&(cardinfo->SD_cid)))
		return 0xFF;
	cardinfo->CardCapacity = (cardinfo->SD_csd.DeviceSize + 1) * 1024;
	cardinfo->CardBlockSize = 1 << (cardinfo->SD_csd.RdBlockLen);
	cardinfo->CardCapacity *= cardinfo->CardBlockSize;
	/*!< Returns the reponse */
	return 0;
}

/*
 * @brief  Initializes the SD/SD communication.
 * @param  None
 * @retval The SD Response:
 *         - 0xFF: Sequence failed
 *         - 0: Sequence succeed
 */
uchar sd_init(void)
{
	uchar frame[10];
	sd_lowlevel_init(0);
	//SD_CS_HIGH();
	SD_CS_LOW();

	// send dummy bytes for 80 clock cycles 
	for (int i = 0; i < 10; i ++) 
		frame[i] = 0xff;
	sd_write_data(frame, 10);

	if (0 != switch_to_SPI_mode()) 
		return 0xff;
	if (0 != verify_operation_condition()) 
		return 0xff;
	if (0 != read_OCR()) 
		return 0xff;
	if (0 != set_SDXC_capacity()) 
		return 0xff;
	if (0 != check_block_size()) 
		return 0xff;
	SD_HIGH_SPEED_ENABLE();
	return sd_get_cardinfo(&cardinfo);
}

/*
 * @brief  Reads a block of data from the SD.
 * @param  data_buff: pointer to the buffer that receives the data read from the
 *                  SD.
 * @param  sector: SD's internal address to read from.
 * @retval The SD Response:
 *         - 0xFF: Sequence failed
 *         - 0: Sequence succeed
 */
uchar sd_read_sector(uchar *data_buff, uint32 sector, uint32 count)
{
	uchar frame[2], flag;
	/*!< Send CMD17 (SD_CMD17) to read one block */
	if (count == 1) {
		flag = 0;
		sd_send_cmd(SD_CMD17, sector, 0);
	} else {
		flag = 1;
		sd_send_cmd(SD_CMD18, sector, 0);
	}
	/*!< Check if the SD acknowledged the read block command: R1 response (0x00: no errors) */
	if (sd_get_response_R1() != 0x00) {
		sd_end_cmd();
		return 0xFF;
	}
	while (count) {
		if (sd_get_response_R1() != SD_START_DATA_SINGLE_BLOCK_READ)
			break;
		/*!< Read the SD block data : read NumByteToRead data */
		sd_read_data(data_buff, 512);
		/*!< Get CRC bytes (not really needed by us, but required by SD) */
		sd_read_data(frame, 2);
		data_buff += 512;
		count--;
	}
	sd_end_cmd();
	if (flag) {
		sd_send_cmd(SD_CMD12, 0, 0);
		sd_get_response_R1();
		sd_end_cmd();
		sd_end_cmd();
	}
	/*!< Returns the reponse */
	return count > 0 ? 0xFF : 0;
}

/*
 * @brief  Writes a block on the SD
 * @param  data_buff: pointer to the buffer containing the data to be written on
 *                  the SD.
 * @param  sector: address to write on.
 * @retval The SD Response:
 *         - 0xFF: Sequence failed
 *         - 0: Sequence succeed
 */
uchar sd_write_sector(uchar *data_buff, uint32 sector, uint32 count)
{
	uchar token[2] = {0xFF, 0x00}, dummpy_crc[2] = {0xFF, 0xFF};
	uchar flag = 0;
    if (count == 1){
		flag = 0;
        token[1] = SD_START_DATA_SINGLE_BLOCK_WRITE;
        sd_send_cmd(SD_CMD24, sector, 0);
    } else{
		flag = 1;
        token[1] = SD_START_DATA_MULTIPLE_BLOCK_WRITE;
        sd_send_cmd(SD_CMD25, sector, 0);
    }

    if (sd_get_response_R1() != SD_TRANS_MODE_RESULT_OK) {
		#ifdef _DEBUG
        printk("Write sector(s) CMD error!\n");
		#endif
		sd_end_cmd();
        return 0xFF;
    }

        while (count)
    {
		sd_write_data(token, 1);
        sd_write_data(token+1, 1);
        sd_write_data(data_buff, 512);
        sd_write_data(dummpy_crc, 2);

        if (sd_get_data_write_response() != SD_TRANS_MODE_RESULT_OK) {
			#ifdef _DEBUG
            printk("Data write response error!\n");
			#endif
			sd_end_cmd();
            return 0xFF;
        }

        data_buff += 512;
        count--;
    }

	if (flag) {
		char end = 0xfd;
		sd_write_data(&end, 1);
		detect_busy();
		sd_end_cmd();
		sd_send_cmd(SD_CMD12, 0, 0);
		sd_get_response_R1();
		sd_end_cmd();
	}

	sd_end_cmd();
    if (count > 0) {
		#ifdef _DEBUG
        printk("Not all sectors are written!\n");
		#endif
        return 0xFF;
    }

    return 0;
}
