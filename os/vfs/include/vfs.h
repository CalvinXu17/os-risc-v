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

#ifndef _TOS_VFS_H_
#define  _TOS_VFS_H_

#include "type.h"
#include "vfs_err.h"
#include "vfs_types.h"
#include "vfs_file.h"
#include "vfs_device.h"
#include "vfs_fs.h"
#include "vfs_inode.h"

#define TOS_CFG_VFS_PATH_MAX        26

int vfs_open(const char *pathname, vfs_oflag_t flags);

int vfs_close(int fd);

uint64 vfs_read(int fd, void *buf, uint64 count);

uint64 vfs_write(int fd, const void *buf, uint64 count);

vfs_off_t vfs_lseek(int fd, vfs_off_t offset, vfs_whence_t whence);

int vfs_ioctl(int fd, unsigned long request, ...);

int vfs_sync(int fd);

int vfs_dup(int oldfd);

int vfs_fstat(int fd, vfs_fstat_t *buf);

int vfs_ftruncate(int fd, vfs_off_t length);

VFS_DIR *vfs_opendir(const char *name);

int vfs_closedir(VFS_DIR *dirp);

vfs_dirent_t *vfs_readdir(VFS_DIR *dirp);

int vfs_rewinddir(VFS_DIR *dirp);

int vfs_statfs(const char *path, vfs_fsstat_t *buf);

int vfs_unlink(const char *pathname);

int vfs_mkdir(const char *pathname);

int vfs_rmdir(const char *pathname);

int vfs_rename(const char *oldpath, const char *newpath);

int vfs_stat(const char *pathname, vfs_fstat_t *buf);

#endif /* _TOS_VFS_H_ */
