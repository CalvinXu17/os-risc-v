#include "printk.h"
// #include "riscv.h"
#include "gpiohs.h"
// #include "buf.h"
#include "spinlock.h"
#include "dmac.h"
#include "spi.h"
#include "sdcard.h"

#define BSIZE 512

void SD_CS_HIGH(void) {
    gpiohs_set_pin(7, GPIO_PV_HIGH);
}

void SD_CS_LOW(void) {
    gpiohs_set_pin(7, GPIO_PV_LOW);
}

void SD_HIGH_SPEED_ENABLE(void) {
    // spi_set_clk_rate(SPI_DEVICE_0, 10000000);
}

static void sd_lowlevel_init(uchar spi_index) {
    gpiohs_set_drive_mode(7, GPIO_DM_OUTPUT);
    // spi_set_clk_rate(SPI_DEVICE_0, 200000);     /*set clk rate*/
}

static void sd_write_data(uchar const *data_buff, uint32 length) {
    spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    spi_send_data_standard(SPI_DEVICE_0, SPI_CHIP_SELECT_3, 0, 0, data_buff, length);
}

static void sd_read_data(uchar *data_buff, uint32 length) {
    spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    spi_receive_data_standard(SPI_DEVICE_0, SPI_CHIP_SELECT_3, 0, 0, data_buff, length);
}

static void sd_write_data_dma(uchar const *data_buff, uint32 length) {
    spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
	spi_send_data_standard_dma(DMAC_CHANNEL0, SPI_DEVICE_0, SPI_CHIP_SELECT_3, 0, 0, data_buff, length);
}

static void sd_read_data_dma(uchar *data_buff, uint32 length) {
    spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
	spi_receive_data_standard_dma(-1, DMAC_CHANNEL0, SPI_DEVICE_0, SPI_CHIP_SELECT_3, 0, 0, data_buff, length);
}

/*
 * @brief  Send 5 bytes command to the SD card.
 * @param  Cmd: The user expected command to send to SD card.
 * @param  Arg: The command argument.
 * @param  Crc: The CRC.
 * @retval None
 */
static void sd_send_cmd(uchar cmd, uint32 arg, uchar crc) {
	uchar frame[6];
	frame[0] = (cmd | 0x40);
	frame[1] = (uchar)(arg >> 24);
	frame[2] = (uchar)(arg >> 16);
	frame[3] = (uchar)(arg >> 8);
	frame[4] = (uchar)(arg);
	frame[5] = (crc);
	SD_CS_LOW();
	sd_write_data(frame, 6);
}

static void sd_end_cmd(void) {
	uchar frame[1] = {0xFF};
	/*!< SD chip select high */
	SD_CS_HIGH();
	/*!< Send the Cmd bytes */
	sd_write_data(frame, 1);
}

/*
 * Be noticed: all commands & responses below 
 * 		are in SPI mode format. May differ from 
 * 		what they are in SD mode. 
 */

#define SD_CMD0 	0 
#define SD_CMD8 	8
#define SD_CMD58 	58 		// READ_OCR
#define SD_CMD55 	55 		// APP_CMD
#define SD_ACMD41 	41 		// SD_SEND_OP_COND
#define SD_CMD16 	16 		// SET_BLOCK_SIZE 
#define SD_CMD17 	17 		// READ_SINGLE_BLOCK
#define SD_CMD24 	24 		// WRITE_SINGLE_BLOCK 
#define SD_CMD13 	13 		// SEND_STATUS

/*
 * Read sdcard response in R1 type. 
 */
static uchar sd_get_response_R1(void) {
	uchar result;
	uint16 timeout = 0xff;

	while (timeout--) {
		sd_read_data(&result, 1);
		if (result != 0xff)
			return result;
	}

	// timeout!
	return 0xff;
}

/* 
 * Read the rest of R3 response 
 * Be noticed: frame should be at least 4-byte long 
 */
static void sd_get_response_R3_rest(uchar *frame) {
	sd_read_data(frame, 4);
}

/* 
 * Read the rest of R7 response 
 * Be noticed: frame should be at least 4-byte long 
 */
static void sd_get_response_R7_rest(uchar *frame) {
	sd_read_data(frame, 4);
}

static int switch_to_SPI_mode(void) {
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
static int read_OCR(void) {
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
static int set_SDXC_capacity(void) {
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
static int check_block_size(void) {
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
				if (512 != BSIZE) {
					#ifdef _DEBUG
					printk("BSIZE != 512\n");
					#endif
					return 0xff;
				}

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
					sd_send_cmd(SD_CMD16, BSIZE, 0);
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
 * @brief  Initializes the SD/SD communication.
 * @param  None
 * @retval The SD Response:
 *         - 0xFF: Sequence failed
 *         - 0: Sequence succeed
 */
static int sd_init(void) {
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

	return 0;
}

static struct sleeplock sdcard_lock;

void sdcard_init(void) {
	int result = sd_init();
	initsleeplock(&sdcard_lock, "sdcard");

	if (0 != result) {
		panic("sdcard_init failed");
	}
	#ifdef DEBUG
	printk("sdcard_init\n");
	#endif
}

void sdcard_read_sector(uchar *buf, int sectorno) {
	uchar result;
	uint32 address;
	uchar dummy_crc[2];

	#ifdef DEBUG
	printk("sdcard_read_sector()\n");
	#endif

	if (is_standard_sd) {
		address = sectorno << 9;
	}
	else {
		address = sectorno;
	}

	// enter critical section!
	acquiresleep(&sdcard_lock);

	sd_send_cmd(SD_CMD17, address, 0);
	result = sd_get_response_R1();

	if (0 != result) {
		releasesleep(&sdcard_lock);
		panic("sdcard: fail to read");
	}

	int timeout = 0xffffff;
	while (--timeout) {
		sd_read_data(&result, 1);
		if (0xfe == result) break;
	}
	if (0 == timeout) {
		panic("sdcard: timeout waiting for reading");
	}
	sd_read_data_dma(buf, BSIZE);
	sd_read_data(dummy_crc, 2);

	sd_end_cmd();

	releasesleep(&sdcard_lock);
	// leave critical section!
}

void sdcard_write_sector(uchar *buf, int sectorno) {
	uint32 address;
	static uchar const START_BLOCK_TOKEN = 0xfe;
	uchar dummy_crc[2] = {0xff, 0xff};

	#ifdef DEBUG
	printk("sdcard_write_sector()\n");
	#endif

	if (is_standard_sd) {
		address = sectorno << 9;
	}
	else {
		address = sectorno;
	}

	// enter critical section!
	acquiresleep(&sdcard_lock);

	sd_send_cmd(SD_CMD24, address, 0);
	if (0 != sd_get_response_R1()) {
		releasesleep(&sdcard_lock);
		panic("sdcard: fail to write");
	}

	// sending data to be written 
	sd_write_data(&START_BLOCK_TOKEN, 1);
	sd_write_data_dma(buf, BSIZE);
	sd_write_data(dummy_crc, 2);

	// waiting for sdcard to finish programming 
	uchar result;
	int timeout = 0xfff;
	while (--timeout) {
		sd_read_data(&result, 1);
		if (0x05 == (result & 0x1f)) {
			break;
		}
	}
	if (0 == timeout) {
		releasesleep(&sdcard_lock);
		panic("sdcard: invalid response token");
	}
	
	timeout = 0xffffff;
	while (--timeout) {
		sd_read_data(&result, 1);
		if (0 != result) break;
	}
	if (0 == timeout) {
		releasesleep(&sdcard_lock);
		panic("sdcard: timeout waiting for response");
	}
	sd_end_cmd();

	// send SD_CMD13 to check if writing is correctly done 
	uchar error_code = 0xff;
	sd_send_cmd(SD_CMD13, 0, 0);
	result = sd_get_response_R1();
	sd_read_data(&error_code, 1);
	sd_end_cmd();
	if (0 != result || 0 != error_code) {
		releasesleep(&sdcard_lock);
		#ifdef _DEBUG
		printk("result: %x\n", result);
		printk("error_code: %x\n", error_code);
		panic("sdcard: an error occurs when writing");
		#endif
	}

	releasesleep(&sdcard_lock);
	// leave critical section!
}

// A simple test for sdcard read/write test 
void test_sdcard(void) {
	uchar buf[BSIZE];

	for (int sec = 0; sec < 5; sec ++) {
		for (int i = 0; i < BSIZE; i ++) {
			buf[i] = 0xaa;		// data to be written 
		}

		sdcard_write_sector(buf, sec);

		for (int i = 0; i < BSIZE; i ++) {
			buf[i] = 0xff;		// fill in junk
		}

		sdcard_read_sector(buf, sec);
		for (int i = 0; i < BSIZE; i ++) {
			if (0 == i % 16) {
				#ifdef _DEBUG
				printk("\n");
				#endif
			}
			#ifdef _DEBUG
			printk("%x ", buf[i]);
			#endif
		}
		#ifdef _DEBUG
		printk("\n");
		#endif
	}

	while (1) ;
}
