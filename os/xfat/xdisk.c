/**
 * 本源码配套的课程为 - 从0到1动手写FAT32文件系统。每个例程对应一个课时，尽可能注释。
 * 作者：李述铜
 * 课程网址：http://01ketang.cc
 * 版权声明：本源码非开源，二次开发，或其它商用前请联系作者。
 */
#include "xfat.h"
#include "xdisk.h"

/**
 * 初始化磁盘设备
 * @param disk 初始化的设备
 * @param name 设备的名称
 * @return
 */
xfat_err_t xdisk_open(xdisk_t* disk, const char* name, xdisk_driver_t* driver,
    void* init_data, u8_t* disk_buf, u32_t buf_size) {
    xfat_err_t err;

    xfat_obj_init(&disk->obj, XFAT_OBJ_DISK);

    disk->driver = driver;

    // 底层驱动初始化
    err = disk->driver->open(disk, init_data);
    if (err < 0) {
        return err;
    }

    err = xfat_bpool_init(&disk->obj, disk->sector_size, disk_buf, buf_size);
    if (err < 0) {
        return err;
    }

    disk->name = name;
    return FS_ERR_OK;
}

/**
 * 关闭存储设备
 * @param disk
 * @return
 */
xfat_err_t xdisk_close(xdisk_t * disk) {
    xfat_err_t err;

    err = xfat_bpool_flush(to_obj(disk));
    if (err < 0) {
        return err;
    }

    err = disk->driver->close(disk);
    if (err < 0) {
        return err;
    }

    return err;
}

/**
 * 从设备中读取指定扇区数量的数据
 * @param disk 读取的磁盘
 * @param buffer 读取数据存储的缓冲区
 * @param start_sector 读取的起始扇区
 * @param count 读取的扇区数量
 * @return
 */
xfat_err_t xdisk_read_sector(xdisk_t *disk, u8_t *buffer, u32_t start_sector, u32_t count) {
    xfat_err_t err;

    if (start_sector + count >= disk->total_sector) {
        return FS_ERR_PARAM;
    }

    err = disk->driver->read_sector(disk, buffer, start_sector, count);
    return err;
}

/**
 * 向设备中写指定的扇区数量的数据
 * @param disk 写入的存储设备
 * @param buffer 数据源缓冲区
 * @param start_sector 写入的起始扇区
 * @param count 写入的扇区数
 * @return
 */
xfat_err_t xdisk_write_sector(xdisk_t *disk, u8_t *buffer, u32_t start_sector, u32_t count) {
    xfat_err_t err;

    if (start_sector + count >= disk->total_sector) {
        return FS_ERR_PARAM;
    }

    err = disk->driver->write_sector(disk, buffer, start_sector, count);
    return err;
}

/**
 * 获取当前时间
 * @param timeinfo 时间存储的数据区
 * @return
 */
xfat_err_t xdisk_curr_time(xdisk_t *disk, struct _xfile_time_t *timeinfo) {
    xfat_err_t err;

    err = disk->driver->curr_time(disk, timeinfo);
    return err;
}

/**
 * 获取扩展分区下的子分区数量
 * @param disk 扩展分区所在的存储设备
 * @param start_sector 扩展分区所在的起始扇区
 * @param count 查询得到的子分区数量
 * @return
 */
static xfat_err_t disk_get_extend_part_count(xdisk_t * disk, u32_t start_sector, u32_t * count) {
    int r_count = 0;

    u32_t ext_start_sector = start_sector;
    do {
        mbr_part_t * part;
        xfat_buf_t * disk_buf;

        // 读取扩展分区的mbr
        xfat_err_t err = xfat_bpool_read_sector(to_obj(disk), &disk_buf, start_sector);
        if (err < 0) {
            return err;
        }

        // 当前分区无效，立即退出
        part = ((mbr_t *)disk_buf->buf)->part_info;
        if (part->system_id == FS_NOT_VALID) {
            break;
        }

        r_count++;

        // 没有后续分区, 立即退出
        if ((++part)->system_id != FS_EXTEND) {
            break;
        }

        // 寻找下一分区
        start_sector = ext_start_sector + part->relative_sectors;
    } while (1);

    *count = r_count;
    return FS_ERR_OK;
}

/**
 * 获取设备上总的分区数量
 * @param disk 查询的存储设备
 * @param count 分区数存储的位置
 * @return
 */
xfat_err_t xdisk_get_part_count(xdisk_t *disk, u32_t *count) {
	int r_count = 0, i = 0;
    mbr_part_t * part;
    u8_t extend_part_flag = 0;
    u32_t start_sector[4];
    xfat_buf_t* disk_buf = (xfat_buf_t*)0;

    // 读取mbr区
    xfat_err_t err = xfat_bpool_read_sector(to_obj(disk), &disk_buf, 0);
	if (err < 0) {
		return err;
	}

	// 解析统计主分区的数量，并标记出哪个分区是扩展分区
	part = ((mbr_t *)disk_buf->buf)->part_info;
	for (i = 0; i < MBR_PRIMARY_PART_NR; i++, part++) {
		if (part->system_id == FS_NOT_VALID) {
            continue;
        } else if (part->system_id == FS_EXTEND) {
            start_sector[i] = part->relative_sectors;
            extend_part_flag |= 1 << i;
        } else {
            r_count++;
        }
	}

	// 统计各个扩展分区下有多少个子分区
    if (extend_part_flag) {
        for (i = 0; i < MBR_PRIMARY_PART_NR; i++) {
            if (extend_part_flag & (1 << i)) {
                u32_t ext_count = 0;
                err = disk_get_extend_part_count(disk, start_sector[i], &ext_count);
                if (err < 0) {
                    return err;
                }

                r_count += ext_count;
            }
        }
    }

    *count = r_count;
	return FS_ERR_OK;
}

/**
 * 获取扩展下分区信息
 * @param disk 查询的存储设备
 * @param disk_part 分区信息存储的位置
 * @param start_sector 扩展分区起始的绝对物理扇区
 * @param part_no 查询的分区号
 * @param count 该扩展分区下一共有多少个子分区
 * @return
 */
static xfat_err_t disk_get_extend_part(xdisk_t * disk, xdisk_part_t * disk_part,
                    u32_t start_sector, int part_no, u32_t * count) {
    int curr_no = -1;
    xfat_err_t err = FS_ERR_OK;

    // 遍历整个扩展分区
    u32_t ext_start_sector = start_sector;
    do {
        mbr_part_t * part;
        xfat_buf_t* disk_buf;

        // 读取扩展分区的mbr
        err = xfat_bpool_read_sector(to_obj(disk), &disk_buf, start_sector);
        if (err < 0) {
            return err;
        }

        part = ((mbr_t *)disk_buf->buf)->part_info;
        if (part->system_id == FS_NOT_VALID) {  // 当前分区无效，设置未找到, 返回
            err = FS_ERR_EOF;
            break;
        }

        // 找到指定的分区号，计算出分区的绝对位置信息
        if (++curr_no == part_no) {
            disk_part->type = part->system_id;
            disk_part->start_sector = start_sector + part->relative_sectors;
            disk_part->total_sector = part->total_sectors;
            disk_part->relative_sector = part->relative_sectors;
            disk_part->disk = disk;
            break;
        }

        if ((++part)->system_id != FS_EXTEND) { // 无后续分区，设置未找到, 返回
            err = FS_ERR_EOF;
            break;
        }

        start_sector = ext_start_sector + part->relative_sectors;
    } while (1);

    *count = curr_no + 1;
    return err;
}

/**
 * 获取指定序号的分区信息
 * 注意，该操作依赖物理分区分配，如果设备的分区结构有变化，则序号也会改变，得到的结果不同
 * @param disk 存储设备
 * @param part 分区信息存储的位置
 * @param part_no 分区序号
 * @return
 */
xfat_err_t xdisk_get_part(xdisk_t *disk, xdisk_part_t *xdisk_part, int part_no) {
    int i;
    int curr_no = -1;
    mbr_part_t * mbr_part;
    xfat_buf_t* disk_buf;

	// 读取mbr
	xfat_err_t err = xfat_bpool_read_sector(to_obj(disk), &disk_buf, 0);
	if (err < 0) {
		return err;
	}

	// 遍历4个主分区描述
    mbr_part = ((mbr_t *)disk_buf->buf)->part_info;
    
	for (i = 0; i < MBR_PRIMARY_PART_NR; i++, mbr_part++) {
		if (mbr_part->system_id == FS_NOT_VALID) {
			continue;
        }

		// 如果是扩展分区，则进入查询子分区
		if (mbr_part->system_id == FS_EXTEND) {
            u32_t count = 0;
            err = disk_get_extend_part(disk, xdisk_part, mbr_part->relative_sectors, part_no - i, &count);
            if (err < 0) {      // 有错误
                return err;
            }

            if (err == FS_ERR_OK) {      // 找到分区
                return FS_ERR_OK;
            } else {                    // 未找到，增加计数
                curr_no += count;

                // todo: 扩展分区的查询破坏了当前读取缓冲，所以此处再次读取
                err = xfat_bpool_read_sector(to_obj(disk), &disk_buf, 0);
                if (err < 0) {
                    return err;
                }
            }
        } else {
		    // 在主分区中找到，复制信息
            if (++curr_no == part_no) {
                xdisk_part->type = mbr_part->system_id;
                xdisk_part->start_sector = mbr_part->relative_sectors;
                xdisk_part->total_sector = mbr_part->total_sectors;
                xdisk_part->relative_sector = mbr_part->relative_sectors;
                xdisk_part->disk = disk;
                return FS_ERR_OK;
            }
        }
	}
    // 无分区
    xdisk_part->type = 0x01;
    xdisk_part->start_sector = 0;
    xdisk_part->relative_sector = 0;
    xdisk_part->total_sector = 61067264;
    xdisk_part->disk = disk;
	return FS_ERR_OK;
}

static xfat_err_t set_ext_part_type(xdisk_part_t* part, u32_t ext_start_sector, xfs_type_t type) {
    xfat_err_t err = FS_ERR_OK;
    u32_t start_sector = ext_start_sector;
    xdisk_t* disk = part->disk;

    do {
        mbr_part_t* ext_part;
        xfat_buf_t* disk_buf;

        err = xfat_bpool_read_sector(to_obj(disk), &disk_buf, start_sector);
        if (err < 0) {
            return err;
        }

        ext_part = ((mbr_t*)disk_buf->buf)->part_info;
        if (ext_part->system_id == FS_NOT_VALID) {
            err = FS_ERR_EOF;
            break;
        }

        if (start_sector + ext_part->relative_sectors == part->start_sector) {
            ext_part->system_id = type;

            err = xfat_bpool_write_sector(to_obj(disk), disk_buf, 0);
            return err;
        }

        if ((++ext_part)->system_id != FS_EXTEND) { // 无后续分区，设置未找到, 返回
            err = FS_ERR_EOF;
            break;
        }

        start_sector = ext_start_sector + ext_part->relative_sectors;
    } while (1);

    return err;
}

/**
 * 设置指定分区格式
 * @param part
 * @param type
 * @return
 */
xfat_err_t xdisk_set_part_type(xdisk_part_t* part, xfs_type_t type) {
    xfat_err_t err = FS_ERR_OK;
    int i;
    mbr_part_t* mbr_part;
    xdisk_t* disk = part->disk;
    xfat_buf_t* disk_buf;

    err = xfat_bpool_read_sector(to_obj(disk), &disk_buf, 0);
    if (err < 0) {
        return err;
    }

    mbr_part = ((mbr_t*)disk_buf->buf)->part_info;
    for (i = 0; i < MBR_PRIMARY_PART_NR; i++, mbr_part++) {
        if (mbr_part->system_id == FS_NOT_VALID) {
            continue;
        }

        // 如果是扩展分区，则进入查询子分区
        if (mbr_part->system_id == FS_EXTEND) {
            u32_t count = 0;
            err = set_ext_part_type(part, mbr_part->relative_sectors, type);
            if (err < 0) {
                return err;
            }

            if (err != FS_ERR_EOF) {
                return FS_ERR_OK;
            }

            // todo: 扩展分区的查询破坏了当前读取缓冲，所以此处再次读取
            err = xfat_bpool_read_sector(to_obj(disk), &disk_buf, 0);
            if (err < 0) {
                return err;
            }
        } else if (mbr_part->relative_sectors == part->start_sector) {
            mbr_part->system_id = type;
                
            err = xfat_bpool_write_sector(to_obj(disk), disk_buf, 0);
            return err;
        }
    }

    return err;
}
