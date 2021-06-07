#ifndef _VFS_H_
#define  _VFS_H_

#include "type.h"
#include "vfs_err.h"
#include "vfs_types.h"
#include "vfs_file.h"
#include "vfs_device.h"
#include "vfs_fs.h"
#include "vfs_inode.h"

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

int vfs_link(const char *oldpath, const char *newpath);

int vfs_unlink(const char *pathname);

int vfs_mkdir(const char *pathname);

int vfs_rmdir(const char *pathname);

int vfs_rename(const char *oldpath, const char *newpath);

int vfs_stat(const char *pathname, vfs_fstat_t *buf);

#endif /* _VFS_H_ */

