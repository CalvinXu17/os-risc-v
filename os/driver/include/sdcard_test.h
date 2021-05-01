#ifndef SDCARD_TEST_H
#define SDCARD_TEST_H

#include "type.h"

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
uchar sd_read_sector_dma(uchar *data_buff, uint32 sector, uint32 count);
uchar sd_write_sector_dma(uchar *data_buff, uint32 sector, uint32 count);


#endif