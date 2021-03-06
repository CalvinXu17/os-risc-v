#include "vfs.h"
#include "string.h"

int vfs_open(const char *pathname, vfs_oflag_t flags)
{
    int path_len = 0, ret = -1;
    char *relative_path = NULL;
    vfs_file_t *file = NULL;
    vfs_inode_t *inode = NULL;

    path_len = strlen(pathname);
    if (path_len > VFS_PATH_MAX) {
        return -VFS_ERR_PATH_TOO_LONG;
    }

    if (pathname[path_len - 1] == '/') {
        return -VFS_ERR_PARA_INVALID;
    }

    inode = vfs_inode_find(pathname, (const char **)&relative_path);
    if (!inode) {
        return -VFS_ERR_INODE_NOT_FOUND;
    }

    file = vfs_file_alloc();
    if (!file) {
        ret = -VFS_ERR_FILE_NO_AVAILABLE;
        goto errout0;
    }

    file->flags     = flags;
    file->pos       = 0;
    file->inode     = inode;
    file->private   = NULL;

    if (VFS_INODE_IS_FILESYSTEM(inode)) {
        if (!relative_path) {
            ret = -VFS_ERR_PARA_INVALID;
            goto errout1;
        }

        if (inode->ops.fs_ops->open) {
            ret = inode->ops.fs_ops->open(file, relative_path, flags);
            strcpy(file->relative_path, pathname);
            file->relative_path[strlen(pathname)] = '\0';
        }
    } else if (VFS_INODE_IS_CHAR_DEVICE(inode)) {
        if (inode->ops.cd_ops->open) {
            ret = inode->ops.cd_ops->open(file);
            strcpy(file->relative_path, pathname);
            file->relative_path[strlen(pathname)] = '\0';
        }
    } else {
        goto errout1;
    }

    if (ret < 0) {
        goto errout1;
    }

    vfs_inode_refinc(inode);

    return vfs_file2fd(file);

errout1:
    vfs_file_free(file);

errout0:
    return ret;
}

int vfs_close(int fd)
{
    int ret = -1;
    vfs_file_t *file = NULL;
    vfs_inode_t *inode = NULL;

    file = vfs_fd2file(fd);
    if (!file) {
        return -VFS_ERR_FILE_NOT_OPEN;
    }

    if(!file->inode)
        return -VFS_ERR_INODE_NOT_FOUND;

    inode = file->inode;

    if (VFS_INODE_IS_FILESYSTEM(inode)) {
        if (inode->ops.fs_ops->close) {
            ret = inode->ops.fs_ops->close(file);
        }
    } else if (VFS_INODE_IS_CHAR_DEVICE(inode)) {
        if (inode->ops.cd_ops->close) {
            ret = inode->ops.cd_ops->close(file);
        }
    } else {
        return -VFS_ERR_INODE_INVALID;
    }

    if (ret == 0) {
        vfs_inode_release(inode);
        vfs_file_free(file);
    }

    return ret;
}

uint64 vfs_read(int fd, void *buf, uint64 count)
{
    int ret = -1;
    vfs_file_t *file = NULL;
    vfs_inode_t *inode = NULL;

    if (!buf) {
        return -VFS_ERR_BUFFER_NULL;
    }

    file = vfs_fd2file(fd);
    if (!file) {
        return -VFS_ERR_FILE_NOT_OPEN;
    }

    inode = file->inode;

    if (VFS_INODE_IS_FILESYSTEM(inode)) {
        if (inode->ops.fs_ops->read) {
            ret = inode->ops.fs_ops->read(file, buf, count);
        }
    } else if (VFS_INODE_IS_CHAR_DEVICE(inode)) {
        if (inode->ops.cd_ops->read) {
            ret = inode->ops.cd_ops->read(file, buf, count);
        }
    } else {
        return -VFS_ERR_INODE_INVALID;
    }

    return ret;
}

uint64 vfs_write(int fd, const void *buf, uint64 count)
{
    int ret = -1;
    vfs_file_t *file = NULL;
    vfs_inode_t *inode = NULL;

    if (!buf) {
        return -VFS_ERR_BUFFER_NULL;
    }

    file = vfs_fd2file(fd);
    if (!file) {
        return -VFS_ERR_FILE_NOT_OPEN;
    }

    inode = file->inode;

    if (VFS_INODE_IS_FILESYSTEM(inode)) {
        if (inode->ops.fs_ops->write) {
            ret = inode->ops.fs_ops->write(file, buf, count);
        }
    } else if (VFS_INODE_IS_CHAR_DEVICE(inode)) {
        if (inode->ops.cd_ops->write) {
            ret = inode->ops.cd_ops->write(file, buf, count);
        }
    } else {
        return -VFS_ERR_INODE_INVALID;
    }

    return ret;
}

vfs_off_t vfs_lseek(int fd, vfs_off_t offset, vfs_whence_t whence)
{
    int ret = -1;
    vfs_file_t *file = NULL;
    vfs_inode_t *inode = NULL;

    file = vfs_fd2file(fd);
    if (!file) {
        return -VFS_ERR_FILE_NOT_OPEN;
    }

    inode = file->inode;

    if (VFS_INODE_IS_FILESYSTEM(inode)) {
        if (inode->ops.fs_ops->lseek) {
            ret = inode->ops.fs_ops->lseek(file, offset, whence);
        }
    } else if (VFS_INODE_IS_CHAR_DEVICE(inode)) {
        if (inode->ops.cd_ops->lseek) {
            ret = inode->ops.cd_ops->lseek(file, offset, whence);
        }
    } else {
        return -VFS_ERR_INODE_INVALID;
    }

    return ret;
}

int vfs_ioctl(int fd, unsigned long request, ...)
{
    int ret = -1;
    va_list ap;
    unsigned long arg;
    vfs_file_t *file = NULL;
    vfs_inode_t *inode = NULL;

    file = vfs_fd2file(fd);
    if (!file) {
        return -VFS_ERR_FILE_NOT_OPEN;
    }

    va_start(ap, request);
    arg = va_arg(ap, unsigned long);
    va_end(ap);

    inode = file->inode;

    if (VFS_INODE_IS_FILESYSTEM(inode)) {
        if (inode->ops.fs_ops->ioctl) {
            ret = inode->ops.fs_ops->ioctl(file, request, arg);
        }
    } else if (VFS_INODE_IS_CHAR_DEVICE(inode)) {
        if (inode->ops.cd_ops->ioctl) {
            ret = inode->ops.cd_ops->ioctl(file, request, arg);
        }
    } else {
        return -VFS_ERR_INODE_INVALID;
    }

    return ret;
}

int vfs_sync(int fd)
{
    int ret = -1;
    vfs_file_t *file = NULL;
    vfs_inode_t *inode = NULL;

    file = vfs_fd2file(fd);
    if (!file) {
        return -VFS_ERR_FILE_NOT_OPEN;
    }

    inode = file->inode;
    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return -VFS_ERR_INODE_INVALID;
    }

    if (inode->ops.fs_ops->sync) {
        ret = inode->ops.fs_ops->sync(file);
    }

    return ret;
}

int vfs_dup(int oldfd)
{
    int ret = -1;
    vfs_file_t *old_file = NULL, *new_file = NULL;
    vfs_inode_t *inode = NULL;

    old_file = vfs_fd2file(oldfd);
    if (!old_file) {
        return -VFS_ERR_FILE_NOT_OPEN;
    }

    inode = old_file->inode;
    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return -VFS_ERR_INODE_INVALID;
    }

    new_file = vfs_file_dup(old_file);
    if (!new_file) {
        return -VFS_ERR_FILE_NO_AVAILABLE;
    }

    if (inode->ops.fs_ops->dup) {
        ret = inode->ops.fs_ops->dup(old_file, new_file);
        if (ret < 0) {
            vfs_file_free(new_file);
        } else
        {
            ret = vfs_file2fd(new_file);
        }
    } else
    {
        ret = vfs_file2fd(new_file);
    }
    
    if (ret < 0) {
        vfs_file_free(new_file);
    }
    
    return ret;
}

int vfs_fstat(int fd, vfs_fstat_t *buf)
{
    int ret = -1;
    vfs_file_t *file = NULL;
    vfs_inode_t *inode = NULL;

    if (!buf) {
        return -VFS_ERR_BUFFER_NULL;
    }

    file = vfs_fd2file(fd);
    if (!file) {
        return -VFS_ERR_FILE_NOT_OPEN;
    }

    inode = file->inode;
    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return -VFS_ERR_INODE_INVALID;
    }

    if (inode->ops.fs_ops->fstat) {
        ret = inode->ops.fs_ops->fstat(file, buf);
    }

    return ret;
}

int vfs_ftruncate(int fd, vfs_off_t length)
{
    int ret = -1;
    vfs_file_t *file = NULL;
    vfs_inode_t *inode = NULL;

    file = vfs_fd2file(fd);
    if (!file) {
        return -VFS_ERR_FILE_NOT_OPEN;
    }

    inode = file->inode;
    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return -VFS_ERR_INODE_INVALID;
    }

    if (inode->ops.fs_ops->truncate) {
        ret = inode->ops.fs_ops->truncate(file, length);
    }

    return ret;
}

VFS_DIR *vfs_opendir(const char *name)
{
    int path_len = 0, ret = -1;
    char *relative_path = NULL;
    vfs_dir_t *dir = NULL;
    vfs_inode_t *inode = NULL;

    if (!name) {
        return NULL;
    }

    path_len = strlen(name);
    if (path_len > VFS_PATH_MAX) {
        return NULL;
    }

    inode = vfs_inode_find(name, (const char **)&relative_path);
    if (!inode) {
        return NULL;
    }

    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return NULL;
    }

    dir = vfs_dir_alloc();
    if (!dir) {
        return NULL;
    }

    dir->pos        = 0;
    dir->inode      = inode;
    dir->private    = NULL;

    if (inode->ops.fs_ops->opendir) {
        ret = inode->ops.fs_ops->opendir(dir, relative_path);
        int rpathlen = strlen(relative_path); 
        if(rpathlen > 0)
        {
            if(relative_path[rpathlen-1] == '/')
            {
                strncpy(dir->relative_path, relative_path, rpathlen-1);
            } else strncpy(dir->relative_path, relative_path, rpathlen);
        } else dir->relative_path[0] = '\0';
    }

    if (ret < 0) {
        vfs_dir_free(dir);
        return NULL;
    }

    vfs_inode_refinc(inode);
    return (VFS_DIR *)dir;
}

int vfs_closedir(VFS_DIR *dirp)
{
    int ret = -1;
    vfs_dir_t *dir = NULL;
    vfs_inode_t *inode = NULL;

    dir     = (vfs_dir_t *)dirp;
    inode   = dir->inode;

    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return -VFS_ERR_INODE_INVALID;
    }

    if (inode->ops.fs_ops->closedir) {
        ret = inode->ops.fs_ops->closedir(dir);
    }

    if (ret == 0) {
        vfs_inode_release(inode);
        vfs_dir_free(dir);
    }

    return ret;
}

vfs_dirent_t *vfs_readdir(VFS_DIR *dirp)
{
    int ret = -1;
    vfs_dir_t *dir = NULL;
    vfs_inode_t *inode = NULL;

    if (!dirp) {
        return NULL;
    }

    dir     = (vfs_dir_t *)dirp;
    inode   = dir->inode;

    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return NULL;
    }

    if (inode->ops.fs_ops->readdir) {
        ret = inode->ops.fs_ops->readdir(dir, &dir->dirent);
    }

    if (ret != 0) {
        return NULL;
    }

    return &dir->dirent;
}

int vfs_rewinddir(VFS_DIR *dirp)
{
    int ret = -1;
    vfs_dir_t *dir = NULL;
    vfs_inode_t *inode = NULL;


    dir     = (vfs_dir_t *)dirp;
    inode   = dir->inode;

    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return -VFS_ERR_INODE_INVALID;
    }

    if (inode->ops.fs_ops->rewinddir) {
        ret = inode->ops.fs_ops->rewinddir(dir);
    }

    return ret;
}

int vfs_statfs(const char *path, vfs_fsstat_t *buf)
{
    int ret = -1;
    vfs_inode_t *inode = NULL;

    if (!buf) {
        return -VFS_ERR_BUFFER_NULL;
    }

    inode = vfs_inode_find(path, NULL);
    if (!inode) {
        return -VFS_ERR_INODE_NOT_FOUND;
    }

    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return -VFS_ERR_INODE_INVALID;
    }

    if (inode->ops.fs_ops->statfs) {
        ret = inode->ops.fs_ops->statfs(inode, buf);
    }

    return ret;
}

int vfs_link(const char *oldpath, const char *newpath)
{
    if(!oldpath || !newpath)
    {
        return -VFS_ERR_BUFFER_NULL;
    }
    int path_len_old, path_len_new, ret = -1;
    char *relative_path_old = NULL;
    char *relative_path_new = NULL;
    vfs_inode_t *inode_old = NULL;
    vfs_inode_t *inode_new = NULL;

    path_len_old = strlen(oldpath);
    path_len_new = strlen(newpath);
    if (path_len_old > VFS_PATH_MAX || path_len_new > VFS_PATH_MAX) {
        return -VFS_ERR_PATH_TOO_LONG;
    }

    if (oldpath[path_len_old - 1] == '/' || newpath[path_len_new - 1] == '/') {
        return -VFS_ERR_PARA_INVALID;
    }

    inode_old = vfs_inode_find(oldpath, (const char **)&relative_path_old);
    inode_new = vfs_inode_find(newpath, (const char **)&relative_path_new);
    if (!inode_old || !inode_new) {
        return -VFS_ERR_INODE_NOT_FOUND;
    }

    if (!VFS_INODE_IS_FILESYSTEM(inode_old) || !VFS_INODE_IS_FILESYSTEM(inode_new) || inode_old != inode_new) {
        return -VFS_ERR_INODE_INVALID;
    }

    if (!relative_path_old || !relative_path_new) {
        return -VFS_ERR_PARA_INVALID;
    }

    if (inode_old->ops.fs_ops->unlink) {
        ret = inode_old->ops.fs_ops->link(inode_old, relative_path_old, relative_path_new);
    }

    return ret;
}

int vfs_unlink(const char *pathname)
{
    int path_len, ret = -1;
    char *relative_path = NULL;
    vfs_inode_t *inode = NULL;

    path_len = strlen(pathname);
    if (path_len > VFS_PATH_MAX) {
        return -VFS_ERR_PATH_TOO_LONG;
    }

    if (pathname[path_len - 1] == '/') {
        return -VFS_ERR_PARA_INVALID;
    }

    inode = vfs_inode_find(pathname, (const char **)&relative_path);
    if (!inode) {
        return -VFS_ERR_INODE_NOT_FOUND;
    }

    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return -VFS_ERR_INODE_INVALID;
    }

    if (!relative_path) {
        return -VFS_ERR_PARA_INVALID;
    }

    if (inode->ops.fs_ops->unlink) {
        ret = inode->ops.fs_ops->unlink(inode, relative_path);
    }

    return ret;
}

int vfs_mkdir(const char *pathname)
{
    int path_len, ret = -1;
    char *relative_path = NULL;
    vfs_inode_t *inode = NULL;

    path_len = strlen(pathname);
    if (path_len > VFS_PATH_MAX) {
        return -VFS_ERR_PATH_TOO_LONG;
    }

    inode = vfs_inode_find(pathname, (const char **)&relative_path);
    if (!inode) {
        return -VFS_ERR_INODE_NOT_FOUND;
    }

    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return -VFS_ERR_INODE_INVALID;
    }

    if (!relative_path) {
        return -VFS_ERR_PARA_INVALID;
    }

    if (inode->ops.fs_ops->mkdir) {
        ret = inode->ops.fs_ops->mkdir(inode, relative_path);
    }

    return ret;
}

int vfs_rmdir(const char *pathname)
{
    int path_len, ret = -1;
    char *relative_path = NULL;
    vfs_inode_t *inode = NULL;

    path_len = strlen(pathname);
    if (path_len > VFS_PATH_MAX) {
        return -VFS_ERR_PATH_TOO_LONG;
    }

    inode = vfs_inode_find(pathname, (const char **)&relative_path);
    if (!inode) {
        return -VFS_ERR_INODE_NOT_FOUND;
    }

    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return -VFS_ERR_INODE_INVALID;
    }

    if (!relative_path) {
        return -VFS_ERR_PARA_INVALID;
    }

    if (inode->ops.fs_ops->rmdir) {
        ret = inode->ops.fs_ops->rmdir(inode, relative_path);
    }

    return ret;
}

int vfs_rename(const char *oldpath, const char *newpath)
{
    int path_len, ret = -1;
    char *old_relative_path = NULL;
    char *new_relative_path = NULL;
    vfs_inode_t *old_inode = NULL, *new_inode = NULL;

    path_len = strlen(oldpath);
    if (path_len > VFS_PATH_MAX) {
        return -VFS_ERR_PATH_TOO_LONG;
    }

    path_len = strlen(newpath);
    if (path_len > VFS_PATH_MAX) {
        return -VFS_ERR_PATH_TOO_LONG;
    }

    old_inode = vfs_inode_find(oldpath, (const char **)&old_relative_path);
    if (!old_inode) {
        return -VFS_ERR_INODE_NOT_FOUND;
    }

    new_inode = vfs_inode_find(newpath, (const char **)&new_relative_path);
    if (!new_inode) {
        return -VFS_ERR_INODE_NOT_FOUND;
    }

    if (!VFS_INODE_IS_FILESYSTEM(old_inode)) {
        return -VFS_ERR_INODE_INVALID;
    }

    if (old_inode != new_inode) {
        return -VFS_ERR_PARA_INVALID;
    }

    if (!old_relative_path || !new_relative_path) {
        return -VFS_ERR_PARA_INVALID;
    }

    if (old_inode->ops.fs_ops->rename) {
        ret = old_inode->ops.fs_ops->rename(old_inode, old_relative_path, new_relative_path);
    }

    return ret;
}

int vfs_stat(const char *pathname, vfs_fstat_t *buf)
{
    int path_len, ret = -1;
    char *relative_path = NULL;
    vfs_inode_t *inode = NULL;

    // PTR_SANITY_CHECK(pathname);
    if (!buf) {
        return -VFS_ERR_BUFFER_NULL;
    }

    path_len = strlen(pathname);
    if (path_len > VFS_PATH_MAX) {
        return -VFS_ERR_PATH_TOO_LONG;
    }

    if (pathname[path_len - 1] == '/') {
        return -VFS_ERR_PARA_INVALID;
    }

    inode = vfs_inode_find(pathname, (const char **)&relative_path);
    if (!inode) {
        return -VFS_ERR_INODE_NOT_FOUND;
    }

    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return -VFS_ERR_INODE_INVALID;
    }

    if (!relative_path) {
        return -VFS_ERR_PARA_INVALID;
    }

    if (inode->ops.fs_ops->stat) {
        ret = inode->ops.fs_ops->stat(inode, relative_path, buf);
    }

    return ret;
}

