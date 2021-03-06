#ifndef VFS_DEVICE_H
#define  VFS_DEVICE_H

#include "type.h"

typedef struct vfs_inode_st vfs_inode_t;

typedef struct vfs_char_device_operations_st {
    int     (*open)     (vfs_file_t *file);
    int     (*close)    (vfs_file_t *file);
    uint64  (*read)     (vfs_file_t *file, void *buf, uint64 count);
    uint64  (*write)    (vfs_file_t *file, const void *buf, uint64 count);
    vfs_off_t   (*lseek)    (vfs_file_t *file, vfs_off_t offset, vfs_whence_t whence);
    int     (*ioctl)    (vfs_file_t *file, int cmd, unsigned long arg);

    int     (*suspend)  (void);
    int     (*resume)   (void);
} vfs_chrdev_ops_t;

typedef struct vfs_block_device_geometry_st {
    int         is_available;
    uint32    sector_size;
    uint32    nsectors;
} vfs_blkdev_geo_t;

typedef struct vfs_block_device_operations_st {
    int     (*open)     (vfs_inode_t *dev);
    int     (*close)    (vfs_inode_t *dev);
    uint64  (*read)     (vfs_inode_t *dev, void *buf, uint64 start_sector, unsigned int nsectors);
    uint64  (*write)    (vfs_inode_t *dev, const unsigned char *buf, uint64 start_sector, unsigned int nsectors);
    int     (*geometry) (vfs_inode_t *dev, vfs_blkdev_geo_t *geo);
    int     (*erase)    (vfs_inode_t *dev, uint64 start_sector, uint64 nsectors);
    int     (*ioctl)    (vfs_inode_t *dev, int cmd, unsigned long arg);

    int     (*suspend)  (void);
    int     (*resume)   (void);
} vfs_blkdev_ops_t;

vfs_err_t vfs_block_device_register(const char *device_name, vfs_blkdev_ops_t *ops);

vfs_err_t vfs_block_device_unregister(const char *device_name);

vfs_err_t vfs_char_device_register(const char *device_name, vfs_chrdev_ops_t *ops);

vfs_err_t vfs_char_device_unregister(const char *device_name);

#endif

