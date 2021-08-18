#include "cfgfs.h"
#include "string.h"
#include "printk.h"
#include "fs.h"

static const char *etc_localtime_path = "/localtime";
static const char *etc_localtime_data = "2021-07-19 08:00:00";

static const char *etc_adjtime_path = "/adjtime";
static const char etc_adjtime_data[CFG_FILE_MAX_SIZE] = "2021-7-19 18:00:00";

int common_cfg_dev_opt(void)
{
    return 0;
}

int is_cfg_file(vfs_file_t *file)
{
    int type = (int)file->private;
    return type == ETC_LOCALTIME || type == ETC_ADJTIME || type == PROC_MOUNTS || type == PROC_MEMINFO;
}

int etc_open(vfs_file_t *file, const char *pathname, vfs_oflag_t flags)
{
    if(!file || !pathname) return -1;

    if(strcmp(pathname, etc_localtime_path) == 0)
    {
        file->flags = flags;
        file->private = (void*)ETC_LOCALTIME;
        file->pos = 0;
        return 0;
    } else if(strcmp(pathname, etc_adjtime_path) == 0)
    {
        file->flags = flags;
        file->private = (void*)ETC_ADJTIME;
        file->pos = 0;
        return 0;
    }
    return -1;
}

int etc_close(vfs_file_t *file)
{
    if(!file) return -1;
    file->private = NULL;
    return 0;
}

uint64 etc_read(vfs_file_t *file, void *buf, uint64 count)
{
    if(!file || !buf) return -1;
    int type = (int)file->private;
    switch (type)
    {
    case ETC_LOCALTIME:
    {
        uint64 proc_size = strlen(etc_localtime_data);
        uint64 remain = proc_size - file->pos;
        if(count <= remain)
        {
            memcpy(buf, etc_localtime_data, count);
            file->pos += count;
            return count;
        } else {
            memcpy(buf, etc_localtime_data, remain);
            file->pos += remain;
            return remain;
        }
        break;
    }
    case ETC_ADJTIME:
    {
        uint64 proc_size = strlen(etc_adjtime_data);
        uint64 remain = proc_size - file->pos;
        if(count <= remain)
        {
            memcpy(buf, etc_adjtime_data, count);
            file->pos += count;
            return count;
        } else {
            memcpy(buf, etc_adjtime_data, remain);
            file->pos += remain;
            return remain;
        }
        break;
    }
    default:
        break;
    }
    return 0;
}

uint64 etc_write(vfs_file_t *file, const void *buf, uint64 count)
{
    return count;
    int type = (int)file->private;
    switch (type)
    {
    case ETC_LOCALTIME:
        
        break;
    default:
        break;
    }
}
vfs_off_t etc_lseek(vfs_file_t *file, vfs_off_t offset, vfs_whence_t whence)
{
    if(!file) return -1;
    int type = (int)file->private;
    switch (type)
    {
    case ETC_LOCALTIME:
    {
        if(whence == VFS_SEEK_SET)
        {
            if(offset >= strlen(etc_localtime_data))
                return -1;
            file->pos = offset;
            return file->pos;
        } else if(whence == VFS_SEEK_CUR)
        {
            if(file->pos + offset >= strlen(etc_localtime_data))
                return -1;
            file->pos += offset;
            return file->pos;
        } else if(whence == VFS_SEEK_END)
        {
            file->pos = strlen(etc_localtime_data) - 1;
            return file->pos;
        }
        break;
    }
    default:
        break;
    }
    
    return -1;
}

int etc_dup(const vfs_file_t *old_file, vfs_file_t *new_file)
{
    if(!old_file || !new_file) return -1;
    new_file->private = old_file->private;
    new_file->pos = 0;
    return 0;
}

int etc_fstat(const vfs_file_t *file, vfs_fstat_t *buf)
{
    if(!file || !buf) return -1;
    int type = (int)file->private;
    int rt = -1;
    switch (type)
    {
    case ETC_LOCALTIME:
        buf->blk_num = 1;
        buf->blk_size = 512;
        buf->mode = VFS_MODE_IFREG | 0644;
        buf->type = VFS_TYPE_FILE;
        buf->ctime = 1626566400;
        buf->atime = 1626566400;
        buf->mtime = 1626566400;
        buf->size = strlen(etc_localtime_data);
        rt = 0;
        break;
    case ETC_ADJTIME:
        buf->blk_num = 1;
        buf->blk_size = 512;
        buf->mode = VFS_MODE_IFREG | 0644;
        buf->type = VFS_TYPE_FILE;
        buf->ctime = 1626566400;
        buf->atime = 1626566400;
        buf->mtime = 1626566400;
        buf->size = strlen(etc_localtime_data);
        rt = 0;
        break;
    default:
        buf->blk_num = 1;
        buf->blk_size = 512;
        buf->mode = VFS_MODE_IFREG | 0644;
        buf->type = VFS_TYPE_FILE;
        buf->ctime = 1626566400;
        buf->atime = 1626566400;
        buf->mtime = 1626566400;
        buf->size = strlen(etc_localtime_data);
        rt = 0;
        break;
    }
    return rt;
}

vfs_chrdev_ops_t etc_dev = {
    .open = common_cfg_dev_opt,
    .close = common_cfg_dev_opt,
    .read = common_cfg_dev_opt,
    .write = common_cfg_dev_opt,
    .lseek = common_cfg_dev_opt,
    .ioctl = common_cfg_dev_opt,
    .resume = common_cfg_dev_opt,
    .suspend = common_cfg_dev_opt
};

vfs_fs_ops_t etc_fs = {
    .open = etc_open,
    .close = etc_close,
    .read = etc_read,
    .write = etc_write,
    .fstat = etc_fstat,
    .dup = etc_dup,
    .lseek = etc_lseek,
    .bind = common_cfg_dev_opt
};

static const char *proc_mounts_path = "/mounts";
static const char proc_mounts_data[CFG_FILE_MAX_SIZE] = "fat32 vfat";

static const char *proc_meminfo_path = "/meminfo";
static const char *proc_meminfo_data = "6MB";

static const char *proc_proc_path = "";
static const char proc_proc_data[CFG_FILE_MAX_SIZE] = "busybox_testcode.sh";

int proc_open(vfs_file_t *file, const char *pathname, vfs_oflag_t flags)
{
    if(!file || !pathname) return -1;
    
    if(strcmp(pathname, proc_mounts_path) == 0)
    {
        file->flags = flags;
        file->private = (void*)PROC_MOUNTS;
        file->pos = 0;
        return 0;
    } else if(strcmp(pathname, proc_meminfo_path) == 0)
    {
        file->flags = flags;
        file->private = (void*)PROC_MEMINFO;
        file->pos = 0;
        return 0;
    }
    return -1;
}

int proc_close(vfs_file_t *file)
{
    if(!file) return -1;
    file->private = NULL;
    return 0;
}

uint64 proc_read(vfs_file_t *file, void *buf, uint64 count)
{
    if(!file || !buf) return -1;
    int type = (int)file->private;
    switch (type)
    {
    case PROC_MOUNTS:
    {
        uint64 proc_size = strlen(proc_mounts_data);
        uint64 remain = proc_size - file->pos;
        if(count <= remain)
        {
            memcpy(buf, proc_mounts_data, count);
            file->pos += count;
            return count;
        } else {
            memcpy(buf, proc_mounts_data, remain);
            file->pos += remain;
            return remain;
        }
        // memcpy(buf, proc_mounts_data)
        break;
    }
    case PROC_MEMINFO:
    {
        uint64 proc_size = strlen(proc_meminfo_data);
        uint64 remain = proc_size - file->pos;
        if(count <= remain)
        {
            memcpy(buf, proc_meminfo_data, count);
            file->pos += count;
            return count;
        } else {
            memcpy(buf, proc_meminfo_data, remain);
            file->pos += remain;
            return remain;
        }
        break;
    }
    case PROC_PROC:
    {
        uint64 proc_size = strlen(proc_proc_data);
        uint64 remain = proc_size - file->pos;
        if(count <= remain)
        {
            memcpy(buf, proc_proc_data, count);
            file->pos += count;
            return count;
        } else {
            memcpy(buf, proc_proc_data, remain);
            file->pos += remain;
            return remain;
        }
        break;
    }
    default:
        break;
    }
    return 0;
}

uint64 proc_write(vfs_file_t *file, const void *buf, uint64 count)
{
    return count;
    int type = (int)file->private;
    switch (type)
    {
    case PROC_MOUNTS:
    {
        break;
    }
    default:
        break;
    }
}
vfs_off_t proc_lseek(vfs_file_t *file, vfs_off_t offset, vfs_whence_t whence)
{
    if(!file) return -1;
    int type = (int)file->private;
    switch (type)
    {
    case PROC_MOUNTS:
    {
        if(whence == VFS_SEEK_SET)
        {
            if(offset >= strlen(etc_localtime_data))
                return -1;
            file->pos = offset;
            return file->pos;
        } else if(whence == VFS_SEEK_CUR)
        {
            if(file->pos + offset >= strlen(etc_localtime_data))
                return -1;
            file->pos += offset;
            return file->pos;
        } else if(whence == VFS_SEEK_END)
        {
            file->pos = strlen(etc_localtime_data) - 1;
            return file->pos;
        }
        break;
    }
    default:
        break;
    }
    
    return -1;
}

int proc_dup(const vfs_file_t *old_file, vfs_file_t *new_file)
{
    if(!old_file || !new_file) return -1;
    new_file->private = old_file->private;
    new_file->pos = 0;
    return 0;
}

int proc_fstat(const vfs_file_t *file, vfs_fstat_t *buf)
{
    if(!file || !buf) return -1;
    int type = (int)file->private;
    int rt = -1;
    switch (type)
    {
    case PROC_MOUNTS:
        buf->blk_num = 1;
        buf->blk_size = 512;
        buf->mode = VFS_MODE_IFREG | 0644;
        buf->type = VFS_TYPE_FILE;
        buf->ctime = 1626566400;
        buf->atime = 1626566400;
        buf->mtime = 1626566400;
        buf->size = strlen(proc_mounts_data);
        rt = 0;
        break;
    case PROC_MEMINFO:
        buf->blk_num = 1;
        buf->blk_size = 512;
        buf->mode = VFS_MODE_IFREG | 0644;
        buf->type = VFS_TYPE_FILE;
        buf->ctime = 1626566400;
        buf->atime = 1626566400;
        buf->mtime = 1626566400;
        buf->size = strlen(proc_meminfo_data);
        rt = 0;
        break;
    case PROC_PROC:
        buf->blk_num = 1;
        buf->blk_size = 512;
        buf->mode = VFS_MODE_IFDIR | 0644;
        buf->type = VFS_TYPE_FILE;
        buf->ctime = 1626566400;
        buf->atime = 1626566400;
        buf->mtime = 1626566400;
        buf->size = strlen(proc_proc_data);
        rt = 0;
        break;
    default:
        buf->blk_num = 1;
        buf->blk_size = 512;
        buf->mode = VFS_MODE_IFREG | 0644;
        buf->type = VFS_TYPE_FILE;
        buf->ctime = 1626566400;
        buf->atime = 1626566400;
        buf->mtime = 1626566400;
        buf->size = strlen(proc_meminfo_data);
        rt = 0;
        break;
    }
    return rt;
}

int proc_opendir(vfs_dir_t *dir, const char *pathname)
{
    if(strcmp(pathname, proc_proc_path) == 0)
    {
        dir->private = PROC_PROC;
        return 0;
    }
    return -1;
}

int proc_closedir(vfs_dir_t *dir)
{
    dir->private = NULL;
    return 0;
}

int proc_readdir(vfs_dir_t *dir, vfs_dirent_t *dirent)
{
    if(dir->pos == 0)
    {
        strcpy(dirent->name, "busybox");
        dirent->size = 512;
        dirent->type = VFS_TYPE_FILE;
        dirent->date = 1626566400;
        dirent->time = 1626566400;
        dir->pos++;
        return 0;
    } else
    {
        dirent->name[0] = '\0';
        return -1;
    }
}

int proc_gentdents(vfs_file_t *file, struct linux_dirent64 *buf, int len)
{
    if(file->pos == 0)
    {
        uint64 nameoffset =  (uint64)&buf->d_name[0] - (uint64)buf;
        int namelen = strlen("busybox");
        buf->d_ino = 1;
        buf->d_off = 0;
        int reclen = nameoffset + namelen + 1;
        if(reclen % 8 != 0) // 内存对齐
        {
            reclen = ((reclen >> 3) + 1) << 3;
        }
        buf->d_reclen = reclen;
        buf->d_type = 1;
        strcpy(buf->d_name, "busybox");
        buf->d_name[namelen] = '\0';
        file->pos += 1;
        return reclen;
    } else return 0;
    
}

vfs_chrdev_ops_t proc_dev = {
    .open = common_cfg_dev_opt,
    .close = common_cfg_dev_opt,
    .read = common_cfg_dev_opt,
    .write = common_cfg_dev_opt,
    .lseek = common_cfg_dev_opt,
    .ioctl = common_cfg_dev_opt,
    .resume = common_cfg_dev_opt,
    .suspend = common_cfg_dev_opt
};

vfs_fs_ops_t proc_fs = {
    .open = proc_open,
    .close = proc_close,
    .read = proc_read,
    .write = proc_write,
    .fstat = proc_fstat,
    .dup = proc_dup,
    .lseek = proc_lseek,
    .bind = common_cfg_dev_opt,
    .opendir = proc_opendir,
    .closedir = proc_closedir,
    .readdir = proc_readdir
};


static int vfs_zero_open(vfs_file_t *file)
{
    return 0;
}

static int vfs_zero_close(vfs_file_t *file)
{
    return 0;
}

static uint64 vfs_zero_read(vfs_file_t *file, void *buf, uint64 count)
{
    char *p = buf;
    while(p < buf+count)
    {
        *p = 0;
        p++;
    }
    return count;
}

static uint64 vfs_zero_write(vfs_file_t *file, const void *buf, uint64 count)
{
    return count;
}

vfs_chrdev_ops_t zero = {
    .open = vfs_zero_open,
    .close = vfs_zero_close,
    .read = vfs_zero_read,
    .write = vfs_zero_write,
};

static int vfs_null_open(vfs_file_t *file)
{
    return 0;
}

static int vfs_null_close(vfs_file_t *file)
{
    return 0;
}

static uint64 vfs_null_read(vfs_file_t *file, void *buf, uint64 count)
{
    char *p = buf;
    while(p < buf+count)
    {
        *p = 0;
        p++;
    }
    return count;
}

static uint64 vfs_null_write(vfs_file_t *file, const void *buf, uint64 count)
{
    return count;
}

vfs_chrdev_ops_t null = {
    .open = vfs_null_open,
    .close = vfs_null_close,
    .read = vfs_null_read,
    .write = vfs_null_write,
};
