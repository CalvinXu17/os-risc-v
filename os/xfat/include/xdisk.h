/**
 * 本源码配套的课程为 - 从0到1动手写FAT32文件系统。每个例程对应一个课时，尽可能注释。
 * 作者：李述铜
 * 课程网址：http://01ketang.cc
 * 版权声明：本源码非开源，二次开发，或其它商用前请联系作者。
 */
#ifndef XDISK_H
#define	XDISK_H

#include "xtypes.h"
#include "xfat_buf.h"
#include "xfat_obj.h"

/**
 * 文件系统类型
 */
typedef enum {
	FS_NOT_VALID = 0x00,            // 无效类型
	FS_FAT32 = 0x01,                // FAT32
    FS_EXTEND = 0x05,               // 扩展分区
    FS_WIN95_FAT32_0 = 0xB,         // FAT32
    FS_WIN95_FAT32_1 = 0xC,         // FAT32
}xfs_type_t;

///////////////////////////////////////////////

/**
 * MBR的分区表项类型
 */
struct _mbr_part_t {
    u8_t boot_active;               // 分区是否活动
	u8_t start_header;              // 起始header
	u16_t start_sector : 6;         // 起始扇区
	u16_t start_cylinder : 10;	    // 起始磁道
	u8_t system_id;	                // 文件系统类型
	u8_t end_header;                // 结束header
	u16_t end_sector : 6;           // 结束扇区
	u16_t end_cylinder : 10;        // 结束磁道
	u32_t relative_sectors;	        // 相对于该驱动器开始的相对扇区数
	u32_t total_sectors;            // 总的扇区数
} __attribute__((__packed__));
typedef struct _mbr_part_t mbr_part_t;

#define MBR_PRIMARY_PART_NR	    4   // 4个分区表

/**
 * MBR区域描述结构
 */
struct _mbr_t {
	u8_t code[446];                 // 引导代码区
    mbr_part_t part_info[MBR_PRIMARY_PART_NR];
	u8_t boot_sig[2];               // 引导标志
} __attribute__((__packed__));
typedef struct _mbr_t mbr_t;

//////////////////////////////////////////////

// 相关前置声明
struct _xdisk_t;
struct _xfile_time_t;

/**
 * 磁盘驱动接口
 */
struct _xdisk_driver_t {
    xfat_err_t (*open) (struct _xdisk_t * disk, void * init_data);
    xfat_err_t (*close) (struct _xdisk_t * disk);
    xfat_err_t (*curr_time) (struct _xdisk_t * disk, struct _xfile_time_t *timeinfo);
    xfat_err_t (*read_sector) (struct _xdisk_t *disk, u8_t *buffer, u32_t start_sector, u32_t count);
    xfat_err_t (*write_sector) (struct _xdisk_t *disk, u8_t *buffer, u32_t start_sector, u32_t count);
};
typedef struct _xdisk_driver_t xdisk_driver_t;

/**
 * 存储设备类型
 */
struct _xdisk_t {
    xfat_obj_t obj;

    const char * name;              // 设备名称
    u32_t sector_size;              // 块大小
	u32_t total_sector;             // 总的块数量
    xdisk_driver_t * driver;        // 驱动接口
    void * data;                    // 设备自定义参数

    xfat_bpool_t bpool;		        // 磁盘缓存，用于分区访问
};
typedef struct _xdisk_t xdisk_t;

/**
 * 存储设备分区类型
 */
struct _xdisk_part_t {
	u32_t start_sector;             // 相对于整个物理存储区开始的块序号
	u32_t total_sector;             // 总的块数量
	u32_t relative_sector;          // 相对于逻辑驱动器的扇区号
	xfs_type_t type;                // 文件系统类型
	xdisk_t * disk;                 // 对应的存储设备
};
typedef struct _xdisk_part_t xdisk_part_t;

xfat_err_t xdisk_open(xdisk_t* disk, const char* name, xdisk_driver_t* driver,
    void* init_data, u8_t* disk_buf, u32_t buf_size);
xfat_err_t xdisk_close(xdisk_t * disk);
xfat_err_t xdisk_get_part_count(xdisk_t *disk, u32_t *count);
xfat_err_t xdisk_get_part(xdisk_t *disk, xdisk_part_t *xdisk_part, int part_no);
xfat_err_t xdisk_curr_time(xdisk_t *disk, struct _xfile_time_t *timeinfo);
xfat_err_t xdisk_read_sector(xdisk_t *disk, u8_t *buffer, u32_t start_sector, u32_t count);
xfat_err_t xdisk_write_sector(xdisk_t *disk, u8_t *buffer, u32_t start_sector, u32_t count);
xfat_err_t xdisk_set_part_type(xdisk_part_t * part, xfs_type_t type);

#endif

