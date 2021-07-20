#ifndef CFGFS_H
#define CFGFS_H

#include "type.h"
#include "vfs.h"

#define CFG_FILE_MAX_SIZE 128
typedef enum {
    ETC_LOCALTIME = 1,
    ETC_ADJTIME = 2,
    PROC_MOUNTS = 3,
    PROC_MEMINFO = 4,
    PROC_PROC = 5,
};

struct linux_dirent64;
int is_cfg_file(vfs_file_t *file);
int cfg_gentdents(vfs_file_t *file, struct linux_dirent64 *buf, int len);

extern vfs_chrdev_ops_t etc_dev;
extern vfs_fs_ops_t etc_fs;

extern vfs_chrdev_ops_t proc_dev;
extern vfs_fs_ops_t proc_fs;
#endif