/**
 * 本源码配套的课程为 - 从0到1动手写FAT32文件系统。每个例程对应一个课时，尽可能注释。
 * 作者：李述铜
 * 课程网址：http://01ketang.cc
 * 版权声明：本源码非开源，二次开发，或其它商用前请联系作者。
 */
#ifndef Xbuf_H
#define Xbuf_H

#include "xtypes.h"
#include "xfat_obj.h"

#define XFAT_BUF_STATE_FREE        (0 << 0)			// 扇区空闲，未被写入任何数据
#define XFAT_BUF_STATE_CLEAN       (1 << 0)			// 扇区干净，未被写数据
#define XFAT_BUF_STATE_DIRTY       (2 << 0)			// 扇区脏，已经被写入数据，未回写到磁盘
#define XFAT_BUF_STATE_MSK         (3 << 0)         // 写状态掩码

/**
 * 磁盘缓存区
 */
struct _xfat_buf_t {
    u8_t * buf;                         // 数据缓冲区
    u32_t sector_no;                    // 扇区号
    u32_t flags;                        // 相关标记

    struct _xfat_buf_t * next;
    struct _xfat_buf_t * pre;
};
typedef struct _xfat_buf_t xfat_buf_t;

#define xfat_buf_state(buf)       (buf->flags & XFAT_BUF_STATE_MSK)

void xfat_buf_set_state(xfat_buf_t * buf, u32_t state);

/**
 * 磁盘缓存池
 */
struct _xfat_bpool_t {
    xfat_buf_t * first;
    xfat_buf_t * last;
    u32_t size;
};
typedef struct _xfat_bpool_t xfat_bpool_t;

// 磁盘缓存空间大小计算
#define XFAT_BUF_SIZE(sector_size, sector_nr)    ((sizeof(xfat_buf_t) + (sector_size)) * sector_nr)

xfat_err_t xfat_bpool_init(xfat_obj_t* obj, u32_t sector_size, u8_t * buffer, u32_t buf_size);
xfat_err_t xfat_bpool_read_sector(xfat_obj_t* obj, xfat_buf_t** buf, u32_t sector_no);
xfat_err_t xfat_bpool_write_sector(xfat_obj_t* obj, xfat_buf_t* buf, u8_t is_through);
xfat_err_t xfat_bpool_alloc(xfat_obj_t* obj, xfat_buf_t** buf, u32_t sector_no);
xfat_err_t xfat_bpool_flush(xfat_obj_t* obj);
xfat_err_t xfat_bpool_flush_sectors(xfat_obj_t* obj, u32_t start_sector, u32_t count);
xfat_err_t xfat_bpool_invalid_sectors(xfat_obj_t* obj, u32_t start_sector, u32_t count);


#endif //Xbuf_H
