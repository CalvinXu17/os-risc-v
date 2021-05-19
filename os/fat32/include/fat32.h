#ifndef FAT32_H
#define FAT32_H

#include "type.h"
#include "disk.h"

// 0 sector 0x01be
#define DPT_OFFSET      0x01be
#define DPT_ACTIVE      0x80
#define DPT_NOT_ACTIVE  0x00

typedef enum
{
    FAT_NOT_VALID = 0x00,
    FAT32 = 0x01,
    EXTEND = 0x05,
    WIN95_FAT32_0 = 0x0b,
    WIN95_FAT32_1 = 0x0c,
}fs_type;

typedef enum {
	FSTATUS_OK,
	FSTATUS_ERROR,
	FSTATUS_NO_VOLUME,
    FSTATUS_NOT_FILE,
    FSTATUS_NOT_DIR,
	FSTATUS_PATH_ERR,
	FSTATUS_EOF
} fstatus;

struct fat_dpt
{
    uchar bool_active;              // 是否为活动分区
    uchar start_header;             // 开始磁头
    unsigned start_sector : 6;      // 开始扇区
    unsigned start_cylinder: 10;    // 开始柱面
    uchar system_id;                // ***系统id，分区类型
    uchar end_header;               // 结束磁头
    unsigned end_sector : 6;        // 结束扇区
    unsigned end_cylinder : 10;     // 结束柱面 
    uint32 start_lba;               // ***相对扇区数，从磁盘开始到该分区的开始的位移量以扇区为单位
    uint32 total_sectors;           // ***该分区总扇区数
} __attribute__((__packed__));

struct fat_bpb
{
    uchar jmpcode[3];
    uchar OEMName[8];
    uint16 BytesPerSec; // ***每扇区字节数
    uchar SecPerClus;   // ***每簇扇区数
    uint16 RsvdSecCnt;  // ***保留扇区数
    uchar FATs_nr;      // ***FAT表数目
    uint16 RootCnt;     // 根目录表项数 FAT32固定为0
    uint16 TotalSec16;  // FAT32固定为0
    uchar media;        // 存储介质
    uint16 FATSize16;   // FAT表项大小 FAT32固定为0
    uint16 SecPerTrack; // 磁道扇区数
    uint16 Header_nr;   // 磁头数
    uint32 HideSec;     // dbr之前的隐藏扇区数=dpt.start_lba
    uint32 TotalSec32;  // ***总扇区数
} __attribute__((__packed__));

struct fat_hdr
{
    uint32 FATSize32;   // ***一个FAT表扇区数
    uint16 ExtFlag;     // 扩展标记
    uint16 Version;     // 版本号
    uint32 RootClus;    // ***根目录簇号
    uint16 FsInfo;      // fsinfo的扇区号，相对dbr为0扇区的扇区号
    uint16 BkBootSec;   // 备份Boot扇区号 通常为6
    uchar Reserved[12];
    uchar DrvID;        // 驱动器号
    uchar Reserved1;
    uchar BootSig;      // 扩展标记
    uint32 VolID;       // 卷序列号
    uchar VolLab[11];   // 卷标名
    uchar FSTypeName[8]; // 系统ID名
} __attribute__((__packed__));

struct fat_dbr
{
    struct fat_bpb bpb;
    struct fat_hdr hdr;
} __attribute__((__packed__));


/**
 * FSInfo结构 位于文件系统的1号扇区，即dbr所在扇区后面一个扇区
 */
struct fat_fsinfo
{
    uint32 FSI_LoadSig;                  // 固定标记：0x41615252
    uchar FSI_Reserved1[480];            // 未用 置0
    uint32 FSI_StrucSig;                 // 固定标记： 0x61417272
    uint32 FSI_Free_Count;               // 最新剩余簇数
    uint32 FSI_Next_Free;                // 从何处开始找剩余簇
    uchar FSI_Reserved2[12];             // 未用
    uint32 FSI_TrailSig;                 // 固定标记： 0xAA550000
} __attribute__((__packed__));

#define CLUSTER_INVALID                 0x0FFFFFFF          // 无效的簇号
#define CLUSTER_FREE                    0x00                // 空闲的cluster
#define CLUSTER_BAD                     0x0FFFFFF7          // 该簇存在坏扇区
#define FILE_DEFAULT_CLUSTER            0x00                // 文件的缺省簇号

#define DIRITEM_NAME_FREE               0xE5                // 目录项空闲名标记
#define DIRITEM_NAME_END                0x00                // 目录项结束名标记

#define DIRITEM_NTRES_BODY_LOWER        0x08                // 文件名小写
#define DIRITEM_NTRES_EXT_LOWER         0x10                // 扩展名小写
#define DIRITEM_NTRES_ALL_UPPER         0x00                // 文件名全部大写
#define DIRITEM_NTRES_CASE_MASK         0x18                // 大小写掩码

#define DIRITEM_ATTR_READ_ONLY          0x01                // 目录项属性：只读
#define DIRITEM_ATTR_HIDDEN             0x02                // 目录项属性：隐藏
#define DIRITEM_ATTR_SYSTEM             0x04                // 目录项属性：系统类型
#define DIRITEM_ATTR_VOLUME_ID          0x08                // 目录项属性：卷id
#define DIRITEM_ATTR_DIRECTORY          0x10                // 目录项属性：目录
#define DIRITEM_ATTR_ARCHIVE            0x20                // 目录项属性：归档
#define DIRITEM_ATTR_LONG_NAME          0x0F                // 目录项属性：长文件名

#define DIRITEM_GET_FREE        (1 << 0)
#define DIRITEM_GET_USED        (1 << 2)
#define DIRITEM_GET_END         (1 << 3)
#define DIRITEM_GET_ALL         0xff

/**
 * FAT目录项的日期类型
 */
struct fat_diritem_date
{
    unsigned day : 5;                  // 日
    unsigned month : 4;                // 月
    unsigned year_from_1980 : 7;       // 年
} __attribute__((__packed__));

/**
 * FAT目录项的时间类型
 */
struct fat_diritem_time
{
    unsigned second_2 : 5;             // 2秒，真正的秒数要乘2
    unsigned minute : 6;               // 分
    unsigned hour : 5;                 // 时
} __attribute__((__packed__));

/**
 * FAT文件目录项
 */
struct fat_diritem
{
    uchar DIR_Name[8];                          // 文件名，不足补空格0x20
    uchar DIR_ExtName[3];                       // 扩展名，不足补空格0x20
    uchar DIR_Attr;                             // 属性，只用到低6位
    uchar DIR_NameCase;                         // 大小写
    uchar DIR_CrtTimeTeenth;                    // 忽略，创建时间的毫秒
    struct fat_diritem_time DIR_CrtTime;        // 创建时间
    struct fat_diritem_date DIR_CrtDate;        // 创建日期
    struct fat_diritem_date DIR_LastAccDate;    // 最后访问日期
    uint16 DIR_ClusterH;                        // 簇号高16位
    struct fat_diritem_time DIR_WrtTime;        // 修改时间
    struct fat_diritem_date DIR_WrtDate;        // 修改日期
    uint16 DIR_ClusterL;                        // 簇号低16位
    uint32 DIR_FileSize;                        // 文件字节大小
} __attribute__((__packed__));

/**
 * 簇类型
 */
typedef union _fat_cluster32
{
    struct {
        unsigned next : 28;                // 下一簇
        unsigned reserved : 4;             // 保留，为0
    } s;
    uint32 v;
} fat_cluster32;




/*****************************/


#define FAT_NAME_LEN       16

/**
 * xfat结构
 */
struct fat
{
    char name[FAT_NAME_LEN];             // FAT名称
    uint32 bpb_sector;                   // bpb所在扇区号
    uint32 total_sectors;                // 总扇区数
    uint32 bytes_per_sector;             // 每扇区字节数
    uint32 sectors_per_cluster;          // 每簇扇区数
    uint64 total_size;                   // 总容量（字节）

    
    uint32 fat_nr;                       // FAT表数量
    uint32 fat_sectors;                  // 每个FAT表的扇区数
    uint32 fat_start_sector;             // FAT表起始扇区

    uint32 root_cluster;                 // 根目录的簇号
    uint32 root_sector;                  // 根目录扇区号

    uint32 fsi_sector;                   // fsi表的扇区号
    uint32 backup_sector;                // 备份扇区
    uint32 cluster_next_free;            // 下一可用的簇(建议值)
    uint32 cluster_total_free;           // 总的空闲簇数量(建议值)
    uchar  buf[512];                     // 文件缓存
    struct disk *dsk;                    // 磁盘设备
};

/**
 * 时间描述结构
 */
struct fat_file_time
{
    uint16 year;
    uchar month;
    uchar day;
    uchar hour;
    uchar minute;
    uchar second;
};

/**
 * 文件类型
 */
typedef enum _fat_file_type
{
    FAT_DIR,
    FAT_FILE,
    FAT_VOL,
} fat_file_type;

#define FILE_ATTR_READONLY         (1 << 0)        // 文件只读

#define SFN_LEN                     11              // sfn文件名长

#define FILE_LOCATE_NORMAL         (1 << 0)        // 查找普通文件
#define FILE_LOCATE_DOT            (1 << 1)        // 查找.和..文件
#define FILE_LOCATE_VOL            (1 << 2)        // 查找卷标
#define FILE_LOCALE_SYSTEM         (1 << 3)        // 查找系统文件
#define FILE_LOCATE_HIDDEN         (1 << 4)        // 查找隐藏文件
#define FILE_LOCATE_ALL            0xFF            // 查找所有
#define FILEINFO_NAME_SIZE         32
/**
 * 文件信息结构
 */
struct fat_fileinfo
{
    char file_name[FILEINFO_NAME_SIZE];          // 文件名
    uint32 size;                                 // 文件字节大小
    uint16 attr;                                 // 文件属性
    fat_file_type type;                          // 文件类型
    struct fat_file_time create_time;            // 创建时间
    struct fat_file_time last_acctime;           // 最后访问时间
    struct fat_file_time modify_time;            // 最后修改时间
};

/**
 * 文件类型
 */
struct fat_file
{
    struct fat *fat32;                  // 对应的fat结构

    struct fat_fileinfo finfo;          // 文件属性信息
    // xfat_err_t err;                  // 上一次的操作错误码

    uint32 start_cluster;               // 文件数据的开始簇
    uint32 cur_cluster;                 // 当前簇号
    uint32 cur_sector_offset;           // 当前簇中的扇区偏移
    uint32 cur_byte_offset;             // 当前扇区的字节偏移
    uint32 cur_from_file_start;         // 当前数据到文件开始的偏移，即已经读取的数据大小
    uint32 dir_cluster;                 // 所在的根目录的描述项起始簇号
    uint32 dir_cluster_offset;          // 所在的根目录的描述项的簇偏移
};

/**
 * 文件seek的定位类型
 */
typedef enum _file_orgin {
    FAT_SEEK_SET,                    // 文件开头
    FAT_SEEK_CUR,                    // 当前位置
    FAT_SEEK_END,                    // 文件结尾
}file_orgin;

/**
 * 文件修改的时间类型
 */
typedef enum _stime_type {
    FAT_TIME_ATIME,                  // 修改访问时间
    FAT_TIME_CTIME,                  // 修改创建时间
    FAT_TIME_MTIME,                  // 修改修改时间
}stime_type;

/**
 * 簇大小枚举类型
 */
typedef enum _cluster_size {
    XFAT_CLUSTER_512B = 512,
    XFAT_CLUSTER_1K = (1 * 1024),
    XFAT_CLUSTER_2K = (2 * 1024),
    XFAT_CLUSTER_4K = (4 * 1024),
    XFAT_CLUSTER_8K = (8 * 1024),
    XFAT_CLUSTER_16K = (16 * 1024),
    XFAT_CLUSTER_32K = (32 * 1024),
    XFAT_CLUSTER_AUTO,              // 内部自动选择
}cluster_size;

struct fat_fmt_ctrl
{
    fs_type type;                // 文件系统类型
    cluster_size cluster_size;
    const char * vol_name;          // 磁盘卷标
};

/**
 * 格式化信息
 */
struct _xfat_fmt_info_t
{
    uchar fat_count;
    uchar media;
    uint32 fat_sectors;
    uint32 rsvd_sectors;
    uint32 root_cluster;
    uint16 sec_per_cluster;

    uint32 backup_sector;
    uint32 fsinfo_sector;
};

#define FILE_APPEND     0x1
#define FILE_OVERRIDE   0x2

int fat32_init(struct disk *dsk);
int get_fat(struct fat *fat32, int index);
int set_fat(struct fat *fat32, int index, int value);
fstatus update_file_diritem(struct fat_file *file, int isdelete);
fstatus search_file_in_dir(struct fat_file *dir, struct fat_file *file, const char *name, int len);
fstatus fat_read_file(struct fat_file *file, uchar *buf, int cnt, int *ret);

#endif