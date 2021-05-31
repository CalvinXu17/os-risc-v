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

#ifndef VFS_FILE_H
#define VFS_FILE_H

#include "type.h"

typedef struct vfs_inode_st vfs_inode_t;

// why do you open so many files in a IOT system?
#define VFS_FILE_OPEN_MAX               20

// why your file path so deep?
#define VFS_PATH_MAX                    32

// open flags(vfs_oflag_t): open method flags (3rd argument of tos_vfs_open)
#define VFS_OFLAG_READ              0x01
#define VFS_OFLAG_WRITE             0x02
#define VFS_OFLAG_EXISTING          0x00
#define VFS_OFLAG_CREATE_NEW        0x04
#define VFS_OFLAG_CREATE_ALWAYS     0x08
#define VFS_OFLAG_OPEN_ALWAYS       0x10
#define VFS_OFLAG_OPEN_APPEND       0x30

typedef enum vfs_whence_en {
    VFS_SEEK_SET,   /* the offset is set to offset bytes */
    VFS_SEEK_CUR,   /* the offset is set to its current location plus offset bytes */
    VFS_SEEK_END,   /* the offset is set to the size of the file plus offset bytes */
} vfs_whence_t;

/* type of a directory entry
   > ls
   d .
   d ..
   f .bashrc
   f .profile
   d Desktop
 */
typedef enum vfs_type_en {
    VFS_TYPE_FILE,
    VFS_TYPE_DIRECTORY,
    VFS_TYPE_OTHER,
} vfs_type_t;

typedef enum vfs_mode_en {
    VFS_MODE_READ_ONLY,
    VFS_MODE_WRITE_ONLY,
    VFS_MODE_READ_WRITE,
} vfs_mode_t;

typedef struct vfs_file_stat_st {
    vfs_type_t  type;       /*file type */
    vfs_mode_t  mode;       /* access mode */
    uint64      size;       /* size of file/directory, in bytes */
    uint32      blk_size;   /* block size used for filesystem I/O */
    uint32      blk_num;    /* number of blocks allocated */
    uint32      atime;      /* time of last access */
    uint32      mtime;      /* time of last modification */
    uint32      ctime;      /* time of last status change */
} vfs_fstat_t;

typedef struct vfs_fs_stat_st {
    uint32    type;       /* type of filesystem */
    uint64    name_len;   /* maximum length of filenames */
    uint32    blk_size;   /* optimal block size for transfers */
    uint32    blk_num;    /* total data blocks in the file system of this size */
    uint32    blk_free;   /* free blocks in the file system */
    uint32    blk_avail;  /* free blocks avail to non-superuser */
    uint32    file_num;   /* total file nodes in the file system */
    uint32    file_free;  /* free file nodes in the file system */
} vfs_fsstat_t;

typedef struct vfs_directory_entry_st {
    vfs_type_t          type;   /* entry type */
    uint64              size;   /* size */
    uint32              date;   /* modified date */
    uint32              time;   /* modified time */
    char                name[VFS_PATH_MAX];
} vfs_dirent_t;

typedef struct vfs_directory_st {
    vfs_dirent_t    dirent;
    vfs_off_t       pos; /*< read offset */
    vfs_inode_t    *inode;
    void           *private; /*< usually used to hook the real dirent struct of the certain filesystem */
    char            relative_path[VFS_PATH_MAX];
} vfs_dir_t;

typedef struct vfs_file_st {
    int             flags;
    vfs_off_t       pos; /*< read offset */
    vfs_inode_t    *inode;
    void           *private; /*< usually used to hook the real file struct of the certain filesystem */
    char            relative_path[VFS_PATH_MAX];
} vfs_file_t;

vfs_file_t *vfs_fd2file(int fd);

int vfs_file2fd(vfs_file_t *file);

vfs_file_t *vfs_file_alloc(void);

void vfs_file_free(vfs_file_t *file);

vfs_file_t *vfs_file_dup(vfs_file_t *file);

vfs_dir_t *vfs_dir_alloc(void);

void vfs_dir_free(vfs_dir_t *dir);

#endif

