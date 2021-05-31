/*----------------------------------------------------------------------------
 * Tencent is pleased to support the open source community by making TencentOS
 * available.
 *
 * Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
 * If you have downloaded a copy of the TencentOS binary from Tencent, please
 * note that the TencentOS binary is licensed under the BSD 3-Clause License.
 *
 * If you have downloaded a copy of the TencentOS source code from Tencent,
 * please note that TencentOS source code is licensed under the BSD 3-Clause
 * License, except for the third-party components listed below which are
 * subject to different license terms. Your integration of TencentOS into your
 * own projects may require compliance with the BSD 3-Clause License, as well
 * as the other licenses applicable to the third-party components included
 * within TencentOS.
 *---------------------------------------------------------------------------*/

#ifndef  VFS_FS_H
#define  VFS_FS_H

#include "type.h"
#include "list.h"

typedef struct vfs_inode_st vfs_inode_t;

typedef struct vfs_filesystem_operations_st {
    int     (*open)         (vfs_file_t *file, const char *pathname, vfs_oflag_t flags);

    int     (*close)        (vfs_file_t *file);
    uint64 (*read)         (vfs_file_t *file, void *buf, uint64 count);
    uint64 (*write)        (vfs_file_t *file, const void *buf, uint64 count);
    vfs_off_t   (*lseek)        (vfs_file_t *file, vfs_off_t offset, vfs_whence_t whence);
    int     (*ioctl)        (vfs_file_t *file, int cmd, unsigned long arg);

    int     (*sync)         (vfs_file_t *file);
    int     (*dup)          (const vfs_file_t *old_file, vfs_file_t *new_file);
    int     (*fstat)        (const vfs_file_t *file, vfs_fstat_t *buf);
    int     (*truncate)     (vfs_file_t *file, vfs_off_t length);

    int     (*opendir)      (vfs_dir_t *dir, const char *pathname);
    int     (*closedir)     (vfs_dir_t *dir);
    int     (*readdir)      (vfs_dir_t *dir, vfs_dirent_t *dirent);
    int     (*rewinddir)    (vfs_dir_t *dir);

    int     (*bind)         (vfs_inode_t *fs, vfs_inode_t *dev);
    int     (*unbind)       (vfs_inode_t *fs);

    int     (*statfs)       (vfs_inode_t *fs, vfs_fsstat_t *buf);

    int     (*unlink)       (vfs_inode_t *fs, const char *pathname);
    int     (*mkdir)        (vfs_inode_t *fs, const char *pathname);
    int     (*rmdir)        (vfs_inode_t *fs, const char *pathname);
    int     (*rename)       (vfs_inode_t *fs, const char *oldpath, const char *newpath);
    int     (*stat)         (vfs_inode_t *fs, const char *pathname, vfs_fstat_t *buf);

    int     (*mkfs)         (vfs_inode_t *dev, int opt, unsigned long arg);
} vfs_fs_ops_t;

typedef struct vfs_filesystem_map_st {
    const char      *name;
    vfs_fs_ops_t    *ops;
    list            list;
} vfs_fsmap_t;

vfs_err_t vfs_fs_register(const char *fs_name, vfs_fs_ops_t *ops);

vfs_err_t vfs_fs_unregister(const char *fs_name);

vfs_err_t vfs_fs_mount(const char *device_path, const char *dir, const char *fs_name);

vfs_err_t vfs_fs_umount(const char *dir);

vfs_err_t vfs_fs_mkfs(const char *device_path, const char *fs_name, int opt, unsigned long arg);

#endif

