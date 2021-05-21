#ifndef SDCARD_TEST_H
#define SDCARD_TEST_H

#include "type.h"

/*
 * SD Card Commands
 * @brief  Commands: CMDxx = CMD-number | 0x40
 */
#define SD_CMD_SIGN (0x40)
#define SD_CMD0     0   // chip reset
#define SD_CMD8     8   // voltage negotiation
#define SD_CMD9     9   // read CSD register
#define SD_CMD10    10  // read CID register
#define SD_CMD12    12  // end multiple continuous sector read
#define SD_CMD16    16  /*!< CMD16 = 0x50 */
#define SD_CMD17    17  // start single sector read
#define SD_CMD18    18  // start multiple continuous sector read and send start sector
#define SD_ACMD23   23  // start multiple continuous sector write and send sector count
#define SD_CMD24    24  // start single sector write
#define SD_CMD25    25  // start multiple continuous sector write and send start sector
#define SD_ACMD41   41  // capacity mode set
#define SD_CMD55    55  // ACMD prefix
#define SD_CMD58    58  // read CCS(card capacity status)
#define SD_CMD59    59  /*!< CMD59 = 0x59 */

#define SD_INIT_MODE_RESULT_OK                      (0x01)
#define SD_TRANS_MODE_RESULT_OK                     (0x00)
#define SD_START_DATA_READ_RESPONSE                 (0xFE)
#define SD_START_DATA_SINGLE_BLOCK_READ    			(0xFE)  /*!< Data token start byte, Start Single Block Read */
#define SD_START_DATA_MULTIPLE_BLOCK_READ  			(0xFE)  /*!< Data token start byte, Start Multiple Block Read */
#define SD_START_DATA_SINGLE_BLOCK_WRITE   			(0xFE)  /*!< Data token start byte, Start Single Block Write */
#define SD_START_DATA_SINGLE_BLOCK_WRITE   			(0xFE)  /*!< Data token start byte, Start Single Block Write */
#define SD_START_DATA_MULTIPLE_BLOCK_WRITE 			(0xFC)  /*!< Data token start byte, Start Multiple Block Write */

/**
 * CMD frame format
 * |   CRC  | arg[31:24] | arg[23:16] | arg[15:8] | arg[7:0] |   CMD  |
 * | byte 5 |    bit 4   |    bit 3   |   bit 2   |   bit 1  |  bit 0 |
 */
#define SD_CMD_CMD_BIT              0
#define SD_CMD_ARG_MSB0             1
#define SD_CMD_ARG_MSB1             2
#define SD_CMD_ARG_MSB2             3
#define SD_CMD_ARG_MSB3             4
#define SD_CMD_CRC_BIT              5
#define SD_CMD_FRAME_SIZE           6

#define SD_EMPTY_FILL               (0xFF)

#define SD_R3_RESPONSE_REST_LENGTH  4
#define SD_R7_RESPONSE_REST_LENGTH  4

/** 
  * @brief  Card Specific Data: CSD Register   
  */ 
typedef struct {
	uchar  CSDStruct;            /*!< CSD structure */
	uchar  SysSpecVersion;       /*!< System specification version */
	uchar  Reserved1;            /*!< Reserved */
	uchar  TAAC;                 /*!< Data read access-time 1 */
	uchar  NSAC;                 /*!< Data read access-time 2 in CLK cycles */
	uchar  MaxBusClkFrec;        /*!< Max. bus clock frequency */
	uint16 CardComdClasses;      /*!< Card command classes */
	uchar  RdBlockLen;           /*!< Max. read data block length */
	uchar  PartBlockRead;        /*!< Partial blocks for read allowed */
	uchar  WrBlockMisalign;      /*!< Write block misalignment */
	uchar  RdBlockMisalign;      /*!< Read block misalignment */
	uchar  DSRImpl;              /*!< DSR implemented */
	uchar  Reserved2;            /*!< Reserved */
	uint32 DeviceSize;           /*!< Device Size */
	uchar  MaxRdCurrentVDDMin;   /*!< Max. read current @ VDD min */
	uchar  MaxRdCurrentVDDMax;   /*!< Max. read current @ VDD max */
	uchar  MaxWrCurrentVDDMin;   /*!< Max. write current @ VDD min */
	uchar  MaxWrCurrentVDDMax;   /*!< Max. write current @ VDD max */
	uchar  DeviceSizeMul;        /*!< Device size multiplier */
	uchar  EraseGrSize;          /*!< Erase group size */
	uchar  EraseGrMul;           /*!< Erase group size multiplier */
	uchar  WrProtectGrSize;      /*!< Write protect group size */
	uchar  WrProtectGrEnable;    /*!< Write protect group enable */
	uchar  ManDeflECC;           /*!< Manufacturer default ECC */
	uchar  WrSpeedFact;          /*!< Write speed factor */
	uchar  MaxWrBlockLen;        /*!< Max. write data block length */
	uchar  WriteBlockPaPartial;  /*!< Partial blocks for write allowed */
	uchar  Reserved3;            /*!< Reserded */
	uchar  ContentProtectAppli;  /*!< Content protection application */
	uchar  FileFormatGrouop;     /*!< File format group */
	uchar  CopyFlag;             /*!< Copy flag (OTP) */
	uchar  PermWrProtect;        /*!< Permanent write protection */
	uchar  TempWrProtect;        /*!< Temporary write protection */
	uchar  FileFormat;           /*!< File Format */
	uchar  ECC;                  /*!< ECC code */
	uchar  CSD_CRC;              /*!< CSD CRC */
	uchar  Reserved4;            /*!< always 1*/
} SD_CSD;

/** 
  * @brief  Card Identification Data: CID Register   
  */
typedef struct {
	uchar  ManufacturerID;       /*!< ManufacturerID */
	uint16 OEM_AppliID;          /*!< OEM/Application ID */
	uint32 ProdName1;            /*!< Product Name part1 */
	uchar  ProdName2;            /*!< Product Name part2*/
	uchar  ProdRev;              /*!< Product Revision */
	uint32 ProdSN;               /*!< Product Serial Number */
	uchar  Reserved1;            /*!< Reserved1 */
	uint16 ManufactDate;         /*!< Manufacturing Date */
	uchar  CID_CRC;              /*!< CID CRC */
	uchar  Reserved2;            /*!< always 1 */
} SD_CID;

/** 
  * @brief SD Card information 
  */
typedef struct {
	SD_CSD SD_csd;
	SD_CID SD_cid;
	uint64 CardCapacity;  /*!< Card Capacity */
	uint32 CardBlockSize; /*!< Card Block Size */
} SD_CardInfo;

extern SD_CardInfo cardinfo;

uchar sd_init(void);
uchar sdcard_init(void);
uchar sd_read_sector(uchar *data_buff, uint32 sector, uint32 count);
uchar sd_write_sector(uchar *data_buff, uint32 sector, uint32 count);

#endif