#include "fs.h"
#include "tos_fatfs_drv.h"
#include "tos_fatfs_vfs.h"
#include "vfs.h"
#include "printk.h"
#include "string.h"
#include "process.h"
#include "kmalloc.h"
#include "console.h"
#include "pipe.h"
#include "mm.h"
#include "page.h"
#include "sched.h"

int fs_init(void)
{
    vfs_err_t err;
    err = vfs_block_device_register("/dev/sdcard", &sd_dev);
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("register sd device error %d\n", err);
        #endif
        return 0;
    }
    err = vfs_fs_register("vfat", &fatfs_ops);
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("register fs error %d\n", err);
        #endif
        return 0;
    }
    err = vfs_fs_mount("/dev/sdcard", "", "vfat");
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("mount fs error %d\n", err);
        #endif
        return 0;
    }
    return 1;
}

struct ufile* ufile_alloc(struct Process *proc)
{
    ufile_t *file = kmalloc(sizeof(ufile_t));
    if(!file) return NULL;

    init_list(&file->list_node);

    list *l = proc->ufiles_list.next;

    while(l!=&proc->ufiles_list)
    {
        ufile_t *f = list2ufile(l);
        if(l->next == &proc->ufiles_list) // 遍历到最后还未找到，则插入到最后
        {
            file->ufd = f->ufd + 1;
            add_after(l, &file->list_node);
            return file;
        }
        ufile_t *next = list2ufile(l->next);
        if(next->ufd > f->ufd + 1)
        {
            file->ufd = f->ufd + 1;
            add_after(l, &file->list_node);
            return file;
        }
        l = l->next;
    }
    kfree(file);
    return NULL;
}

struct ufile* ufile_alloc_by_fd(int fd, struct Process *proc)
{
    ufile_t *file = kmalloc(sizeof(ufile_t));
    if(!file) return NULL;

    init_list(&file->list_node);

    list *l = proc->ufiles_list.next;
    if(fd < list2ufile(l)->ufd)
    {
        kfree(file);
        return NULL;
    }

    while(l!=&proc->ufiles_list)
    {
        ufile_t *f = list2ufile(l);
        if(f->ufd == fd) // 已存在
            break;
        if(l->next == &proc->ufiles_list) // 遍历到最后还未找到，则插入到最后
        {
            file->ufd = fd;
            add_after(l, &file->list_node);
            return file;
        }
        
        ufile_t *next = list2ufile(l->next);
        if(next->ufd > fd && f->ufd < fd)
        {
            file->ufd = fd;
            add_after(l, &file->list_node);
            return file;
        }
        l = l->next;
    }
    kfree(file);
    return NULL;
}

void ufile_free(ufile_t *file)
{
    del_list(&file->list_node);
    kfree(file);
}

struct ufile* ufd2ufile(int ufd, struct Process *proc)
{
    list *l = proc->ufiles_list.next;
    while(l!=&proc->ufiles_list)
    {
        ufile_t *f = list2ufile(l);
        if(f->ufd == ufd)
            return f;
        l = l->next;
    }
    return NULL;
}

char* sys_getcwd(char *buf, int size)
{
    if(!buf || size <= 1) return NULL;
    buf[--size] = 0; // 保证最后一位为0
    struct Process *proc = getcpu()->cur_proc;
    int leninode = strlen(proc->cwd->inode->name);
    strcpy(buf, proc->cwd->inode->name);
    int delt = size - leninode;
    if(delt < 0) return buf;
    strcpy(buf+leninode, proc->cwd->relative_path);
    if(!strlen(buf))
    {
        buf[0] = '/';
    }
    return buf;
}

int sys_chdir(const char *dirpath)
{
    if(!dirpath || !strlen(dirpath)) return -1;

    char *path = NULL;
    char abspath[VFS_PATH_MAX];

    if(dirpath[0] == '/') // 绝对路径
    {
        path = dirpath;
    } else // 相对路径
    {
        vfs_dir_t *dir = getcpu()->cur_proc->cwd;
        
        memset(abspath, 0, VFS_PATH_MAX);
        int len_filename = strlen(dirpath);
        int len_inode = strlen(dir->inode->name);
        int len_relative = strlen(dir->relative_path);
        if(len_inode + len_relative >= VFS_PATH_MAX) return -1;
        strcpy(abspath, dir->inode->name);
        strcpy(abspath+len_inode, dir->relative_path);

        if(len_filename >= 2 && dirpath[0]=='.' && dirpath[1]=='/')
        {
            if(len_filename - 1 + len_inode + len_relative >= VFS_PATH_MAX) return -1;
            strcpy(abspath+len_inode+len_relative, dirpath+1);
        } else if(len_filename == 1 && dirpath[0]=='.')
        {
            return 0;
        } else {
            if(len_filename + len_inode + len_relative + 1 >= VFS_PATH_MAX) return -1;
            abspath[len_inode+len_relative] = '/';
            strcpy(abspath+len_inode+len_relative+1, dirpath);
        }
        path = abspath;
    }
    vfs_dir_t *new_dir = vfs_opendir(path);
    if(!new_dir) return -1;
    vfs_closedir(getcpu()->cur_proc->cwd);
    getcpu()->cur_proc->cwd = new_dir;
    return 0;
}

int sys_openat(int ufd, char *filename, int flags, int mode)
{
    if(!filename || !strlen(filename)) return -1;

    char *path = NULL;
    int iscwddir = 0;
    char abspath[VFS_PATH_MAX];
    vfs_dir_t *dir = NULL;

    if(filename[0] == '/') // 绝对路径
    {
        path = filename;
    } else // 相对路径
    {
        if(ufd == AT_FDCWD) // 相对当前工作目录
        {
            dir = getcpu()->cur_proc->cwd;
        } else // 相对fd
        {
            struct ufile *ufile = ufd2ufile(ufd, getcpu()->cur_proc);
            if(!ufile) return -1;
            if(ufile->type != UTYPE_DIR) return -1;
            dir = ufile->private;
        }
        
        memset(abspath, 0, VFS_PATH_MAX);
        int len_filename = strlen(filename);
        int len_inode = strlen(dir->inode->name);
        int len_relative = strlen(dir->relative_path);
        if(len_inode + len_relative >= VFS_PATH_MAX) return -1;
        strncpy(abspath, dir->inode->name, len_inode);
        strncpy(abspath+len_inode, dir->relative_path, len_relative);

        if(len_filename >= 2 && filename[0]=='.' && filename[1]=='/')
        {
            if(len_filename - 1 + len_inode + len_relative >= VFS_PATH_MAX) return -1;
            strcpy(abspath+len_inode+len_relative, filename+1);
        } else if(len_filename == 1 && filename[0]=='.')
        {
            abspath[len_inode+len_relative] = 0;
            dir = getcpu()->cur_proc->cwd;
            iscwddir = 1;
        } else {
            if(len_filename + len_inode + len_relative + 1 >= VFS_PATH_MAX) return -1;
            abspath[len_inode+len_relative] = '/';
            strcpy(abspath+len_inode+len_relative+1, filename);
        }
        path = abspath;
    }

    if(iscwddir)
    {
        dir = vfs_opendir(path);
        struct ufile *file = ufile_alloc(getcpu()->cur_proc);
        file->type = UTYPE_DIR;
        file->private = dir;
        return file->ufd;
    }
    else if((flags & O_DIRECTORY)) //打开的是目录
    {
        dir = vfs_opendir(path);
        if(!dir)
        {
            if(!(flags & O_CREATE)) return -1;
            int rt = vfs_mkdir(path);
            if(rt < 0) return -1;
            dir = vfs_opendir(path);
            if(!dir) return -1;
        }
        struct ufile *file = ufile_alloc(getcpu()->cur_proc);
        file->type = UTYPE_DIR;
        file->private = dir;
        return file->ufd;
    }
    else // 打开的是文件
    {
        int flag = VFS_OFLAG_READ;
        if((flags & O_RDWR) || (flags & O_WRONLY)) flag |= VFS_OFLAG_WRITE;
    
        int fd = vfs_open(path, flag);
        if(fd < 0)
        {
            if(!(flags & O_CREATE)) return -1;
            flag |= (VFS_OFLAG_CREATE_ALWAYS | VFS_OFLAG_WRITE);
            fd = vfs_open(path, flag);
            if(fd < 0) return -1;
        }
        struct ufile *file = ufile_alloc(getcpu()->cur_proc);
        file->type = UTYPE_FILE;
        file->private = fd;
        return file->ufd;
    }
    return -1;
}

int sys_close(int fd)
{
    struct ufile *file = ufd2ufile(fd, getcpu()->cur_proc);
    if(!file) return -1;
    int rt = -1;
    if(file->type == UTYPE_FILE)
    {
        if(!vfs_close((int)file->private)) rt = 0;
    } else if(file->type == UTYPE_DIR)
    {
        if(!vfs_closedir(file->private)) rt = 0;
    } else if(file->type == UTYPE_PIPEIN)
    {
        rt = pipe_close(file->private, UTYPE_PIPEIN);
    } else if(file->type == UTYPE_PIPEOUT)
    {
        rt = pipe_close(file->private, UTYPE_PIPEOUT);
    }
    ufile_free(file);
    return rt;
}

int sys_mkdirat(int dirfd, char *dirpath, int mode)
{
    if(!dirpath || !strlen(dirpath)) return -1;

    char *path = NULL;

    if(dirpath[0] == '/') // 绝对路径
    {
        path = dirpath;
    } else // 相对路径
    {
        vfs_dir_t *dir = NULL;
        if(dirfd == AT_FDCWD) // 相对当前工作目录
        {
            dir = getcpu()->cur_proc->cwd;
        } else // 相对fd
        {
            struct ufile *ufile = ufd2ufile(dirfd, getcpu()->cur_proc);
            if(!ufile) return -1;
            if(ufile->type != UTYPE_DIR) return -1;
            dir = ufile->private;
        }
        char abspath[VFS_PATH_MAX];
        memset(abspath, 0, VFS_PATH_MAX);
        int len_filename = strlen(dirpath);
        int len_inode = strlen(dir->inode->name);
        int len_relative = strlen(dir->relative_path);
        if(len_inode + len_relative >= VFS_PATH_MAX) return -1;
        strcpy(abspath, dir->inode->name);
        strcpy(abspath+len_inode, dir->relative_path);

        if(len_filename >= 2 && dirpath[0]=='.' && dirpath[1]=='/')
        {
            if(len_filename - 1 + len_inode + len_relative >= VFS_PATH_MAX) return -1;
            strcpy(abspath+len_inode+len_relative, dirpath+1);
        } else {
            if(len_filename + len_inode + len_relative + 1 >= VFS_PATH_MAX) return -1;
            abspath[len_inode+len_relative] = '/';
            strcpy(abspath+len_inode+len_relative+1, dirpath);
        }
        path = abspath;
    }
    if(!vfs_mkdir(path)) return 0;
    else return -1;
}

uint64 sys_read(int ufd, uchar *buf, uint64 count)
{
    if(!buf) return -1;
    struct ufile *file = ufd2ufile(ufd, getcpu()->cur_proc);
    if(!file) return -1;
    if(file->type == UTYPE_STDIN)
    {
        return console_read(buf, count);
    } else if(file->type == UTYPE_PIPEIN)
    {
        return pipe_read(file->private, buf, count);
    }else if(file->type == UTYPE_FILE)
    {
        return vfs_read((int)file->private, buf, count);
    }
    return -1;
}

uint64 sys_write(int ufd, uchar *buf, uint64 count)
{
    if(!buf) return -1;
    struct ufile *file = ufd2ufile(ufd, getcpu()->cur_proc);
    if(!file) return -1;
    if(file->type == UTYPE_STDOUT)
    {
        return console_write(buf, count);
    } else if(file->type == UTYPE_PIPEOUT)
    {
        return pipe_write(file->private, buf, count);
    } else if(file->type == UTYPE_FILE)
    {
        int rt = vfs_write((int)file->private, buf, count);
        vfs_sync((int)file->private);
        return -1;
    }
    return -1;
}

int sys_pipe2(int fd[2])
{
    if(!fd) return -1;
    struct Process *proc = getcpu()->cur_proc;
    ufile_t *pipin = ufile_alloc(proc);
    if(!pipin) return -1;
    ufile_t *pipout = ufile_alloc(proc);
    if(!pipout) return -1;
    pipe_t *pip = pipe_alloc();
    pipin->type = UTYPE_PIPEIN;
    pipin->private = pip;
    pipout->type = UTYPE_PIPEOUT;
    pipout->private = pip;
    fd[0] = pipin->ufd;
    fd[1] = pipout->ufd;
    return 0;
}

int sys_dup(int fd)
{
    if(fd < 0) return -1;
    ufile_t *file = ufd2ufile(fd, getcpu()->cur_proc);
    if(!file) return -1;
    ufile_t *newfile = ufile_alloc(getcpu()->cur_proc);
    if(!newfile) return -1;
    if(file->type == UTYPE_FILE)
    {
        newfile->type = UTYPE_FILE;
        int nfd = vfs_dup((int)file->private);
        if(nfd < 0)
        {
            ufile_free(newfile);
            return -1;
        }
        newfile->private = nfd;
        return newfile->ufd;
    } else if(file->type == UTYPE_PIPEIN)
    {
        newfile->type = UTYPE_PIPEIN;
        newfile->private = file->private;
        pipe_t *pip = newfile->private;
        lock(&pip->mutex);
        pip->r_ref++;
        unlock(&pip->mutex);
        return newfile->ufd;
    } else if(file->type == UTYPE_PIPEOUT)
    {
        newfile->type = UTYPE_PIPEOUT;
        newfile->private = file->private;
        pipe_t *pip = newfile->private;
        lock(&pip->mutex);
        pip->w_ref++;
        unlock(&pip->mutex);
        return newfile->ufd;
    } else if(file->type == UTYPE_STDIN)
    {
        newfile->type = UTYPE_STDIN;
        newfile->private = file->private;
        return newfile->ufd;
    } else if(file->type == UTYPE_STDOUT)
    {
        newfile->type = UTYPE_STDOUT;
        newfile->private = file->private;
        return newfile->ufd;
    }else if(file->type == UTYPE_DIR)
    {
        newfile->type = UTYPE_DIR;
        char abspath[VFS_PATH_MAX];
        vfs_dir_t *dir = file->private;
        memset(abspath, 0, VFS_PATH_MAX);
        int len_inode = strlen(dir->inode->name);
        int len_relative = strlen(dir->relative_path);
        if(len_inode + len_relative >= VFS_PATH_MAX)
        {
            ufile_free(newfile);
            return -1;
        }
        strcpy(abspath, dir->inode->name);
        strcpy(abspath+len_inode, dir->relative_path);
        vfs_dir_t *ndir = vfs_opendir(abspath);
        if(!ndir)
        {
            ufile_free(newfile);
            return -1;
        }
        newfile->private = ndir;
        return newfile->ufd;
    }
    return -1;
}

int sys_dup2(int fd, int nfd)
{
    if(fd < 0 || nfd < 0) return -1;
    ufile_t *file = ufd2ufile(fd, getcpu()->cur_proc);
    if(!file) return -1;
    ufile_t *newfile = ufile_alloc_by_fd(nfd, getcpu()->cur_proc);
    if(!newfile) return -1;
    if(file->type == UTYPE_FILE)
    {
        newfile->type = UTYPE_FILE;
        int nfd = vfs_dup((int)file->private);
        if(nfd < 0)
        {
            ufile_free(newfile);
            return -1;
        }
        newfile->private = nfd;
        return newfile->ufd;
    } else if(file->type == UTYPE_PIPEIN)
    {
        newfile->type = UTYPE_PIPEIN;
        newfile->private = file->private;
        pipe_t *pip = newfile->private;
        lock(&pip->mutex);
        pip->r_ref++;
        unlock(&pip->mutex);
        return newfile->ufd;
    } else if(file->type == UTYPE_PIPEOUT)
    {
        newfile->type = UTYPE_PIPEOUT;
        newfile->private = file->private;
        pipe_t *pip = newfile->private;
        lock(&pip->mutex);
        pip->w_ref++;
        unlock(&pip->mutex);
        return newfile->ufd;
    } else if(file->type == UTYPE_STDIN)
    {
        newfile->type = UTYPE_STDIN;
        newfile->private = file->private;
        return newfile->ufd;
    } else if(file->type == UTYPE_STDOUT)
    {
        newfile->type = UTYPE_STDOUT;
        newfile->private = file->private;
        return newfile->ufd;
    }else if(file->type == UTYPE_DIR)
    {
        newfile->type = UTYPE_DIR;
        char abspath[VFS_PATH_MAX];
        vfs_dir_t *dir = file->private;
        memset(abspath, 0, VFS_PATH_MAX);
        int len_inode = strlen(dir->inode->name);
        int len_relative = strlen(dir->relative_path);
        if(len_inode + len_relative >= VFS_PATH_MAX)
        {
            ufile_free(newfile);
            return -1;
        }
        strcpy(abspath, dir->inode->name);
        strcpy(abspath+len_inode, dir->relative_path);
        vfs_dir_t *ndir = vfs_opendir(abspath);
        if(!ndir)
        {
            ufile_free(newfile);
            return -1;
        }
        newfile->private = ndir;
        return newfile->ufd;
    }
    return -1;
}

int sys_getdents64(int ufd, struct linux_dirent64 *buf, int len)
{
    if(!buf || !len) return -1;
    ufile_t *file = ufd2ufile(ufd, getcpu()->cur_proc);
    if(!file) return -1;
    if(file->type != UTYPE_DIR) return -1;
    vfs_dir_t *dir = file->private;
    
    uchar *p = buf;
    int cnt = 0;
    uint64 nameoffset =  (uint64)&buf->d_name[0] - (uint64)buf;
    while(p < (uchar*)buf + len)
    {
        vfs_readdir(dir);
        int namelen = strlen(dir->dirent.name);
        if(!namelen) break;
        if(p + nameoffset + namelen + 1 >= (uint64)buf + len)
            break;
        struct linux_dirent64 *pbuf = p;
        pbuf->d_ino = dir->inode;
        pbuf->d_off = dir->pos;
        int reclen = nameoffset + namelen + 1;
        if(reclen % 8 != 0) // 内存对齐
        {
            reclen = ((reclen >> 3) + 1) << 3;
        }
        pbuf->d_reclen = reclen;
        pbuf->d_type = dir->dirent.type;
        strcpy(pbuf->d_name, dir->dirent.name);
        pbuf->d_name[namelen] = '\0';
        cnt++;
        p += reclen;
    }
    return cnt;
}

int sys_execve(const char *name, char *const argv[], char *const argp[])
{
    if(!name || !strlen(name)) return -1;

    char *path = NULL;
    char abspath[VFS_PATH_MAX];

    if(name[0] == '/') // 绝对路径
    {
        path = name;
    } else // 相对路径
    {
        vfs_dir_t *dir = getcpu()->cur_proc->cwd;
        
        memset(abspath, 0, VFS_PATH_MAX);
        int len_filename = strlen(name);
        int len_inode = strlen(dir->inode->name);
        int len_relative = strlen(dir->relative_path);
        if(len_inode + len_relative >= VFS_PATH_MAX) return -1;
        strcpy(abspath, dir->inode->name);
        strcpy(abspath+len_inode, dir->relative_path);

        if(len_filename >= 2 && name[0]=='.' && name[1]=='/')
        {
            if(len_filename - 1 + len_inode + len_relative >= VFS_PATH_MAX) return -1;
            strcpy(abspath+len_inode+len_relative, name+1);
        } else if(len_filename == 1 && name[0]=='.')
        {
            return -1;
        } else {
            if(len_filename + len_inode + len_relative + 1 >= VFS_PATH_MAX) return -1;
            abspath[len_inode+len_relative] = '/';
            strcpy(abspath+len_inode+len_relative+1, name);
        }
        path = abspath;
    }
    struct Process *cur_proc = getcpu()->cur_proc;
    struct Process *new_proc = create_proc_by_elf(abspath);
    if(!new_proc) return -1;


    uint64 *sp = userva2kernelva(new_proc->pg_t, new_proc->tcontext.sp);
    uchar *data = userva2kernelva(new_proc->pg_t, PAGE_SIZE);
    
    int argc = 0;
    int i, j;
    for(i=0;argv[i] != 0;i++) ;

    for(j=i-1; j>=0; j--)
    {
        char *v = argv[j];
        uint64 len = strlen(v) + 1;
        if(len % 8 != 0) // 地址对齐
        {
            len = ((len >> 3) + 1 ) << 3;
        }
        data = data - len;
        strcpy(data, v);
        *sp = data;
        argc++;
        sp--;
        new_proc->tcontext.sp -= 8;
    }
    *sp = argc;
    // 到这里新的进程全部初始化完毕，此时将当前进程替换为新的进程即可
    // 重置当前进程结构体
    kfree(new_proc->k_stack);
    new_proc->k_stack = cur_proc->k_stack;
    cur_proc->k_stack = NULL;
    free_process_mem(cur_proc); // 释放当前进程内存

    cur_proc->k_stack = new_proc->k_stack;
    cur_proc->pg_t = new_proc->pg_t;
    memcpy(&cur_proc->tcontext, &new_proc->tcontext, sizeof(struct trap_context));
    memcpy(&cur_proc->pcontext, &new_proc->pcontext, sizeof(struct proc_context));
    cur_proc->status = PROC_READY;
    cur_proc->t_wait = 0;
    cur_proc->t_slice = new_proc->t_slice;
    cur_proc->start_time = new_proc->start_time;
    cur_proc->end_time = new_proc->end_time;
    cur_proc->utime_start = new_proc->utime_start;
    cur_proc->stime_start = new_proc->stime_start;
    cur_proc->utime = new_proc->utime;
    cur_proc->stime = new_proc->stime;
    vfs_closedir(cur_proc->cwd);
    cur_proc->cwd = new_proc->cwd;
    new_proc->cwd = NULL;
    cur_proc->brk = new_proc->brk;
    cur_proc->minbrk = new_proc->minbrk;
    cur_proc->tcontext.hartid = gethartid();

    free_ufile_list(cur_proc);
    init_list(&cur_proc->ufiles_list);
    
    ufile_t *file = kmalloc(sizeof(ufile_t)); // 初始化STDIN文件
    file->ufd = 0;
    file->type = UTYPE_STDIN;
    init_list(&file->list_node);
    add_before(&cur_proc->ufiles_list, &file->list_node);

    file = kmalloc(sizeof(ufile_t)); // 初始化STDOUT文件
    file->ufd = 1;
    file->type = UTYPE_STDOUT;
    init_list(&file->list_node);
    add_before(&cur_proc->ufiles_list, &file->list_node);
    
    // 切换到新进程内存空间
    set_sscratch(&cur_proc->tcontext);
    uint64 sapt = ((((uint64)cur_proc->pg_t) - PV_OFFSET) >> 12) | (8L << 60);
    set_satp(sapt);
    sfence_vma();
    trap_ret();
    return -1;
}

int sys_fstat(int ufd, struct kstat *kst)
{
    if(!kst) return -1;
    ufile_t *file = ufd2ufile(ufd, getcpu()->cur_proc);
    if(!file) return -1;
    if(file->type != UTYPE_FILE) return -1;
    vfs_fstat_t st;
    vfs_file_t *vfile = vfs_fd2file((int)file->private);
    if(vfs_stat(vfile->relative_path, &st) < 0) return -1;
    
    kst->st_dev = 1;
    kst->st_mode = 0666;
    kst->st_nlink = 1;
    kst->st_uid = 0;
    kst->st_gid = 0;
    kst->st_rdev = 0;
    kst->st_size = st.size;
    kst->st_blksize = 512;
    kst->st_blocks = st.blk_num;
    kst->st_atime_sec = st.atime;
    kst->st_atime_nsec = 0;
    kst->st_mtime_sec = st.mtime;
    kst->st_mtime_nsec = 0;
    kst->st_ctime_sec = st.ctime;
    kst->st_ctime_nsec = 0;
    return 0;
}

int sys_unlinkat(int ufd, char *filename, int flags)
{
    if(!filename || !strlen(filename)) return -1;

    char *path = NULL;
    int iscwddir = 0;
    char abspath[VFS_PATH_MAX];
    vfs_dir_t *dir = NULL;

    if(filename[0] == '/') // 绝对路径
    {
        path = filename;
    } else // 相对路径
    {
        if(ufd == AT_FDCWD) // 相对当前工作目录
        {
            dir = getcpu()->cur_proc->cwd;
        } else // 相对fd
        {
            struct ufile *ufile = ufd2ufile(ufd, getcpu()->cur_proc);
            if(!ufile) return -1;
            if(ufile->type != UTYPE_DIR) return -1;
            dir = ufile->private;
        }
        
        memset(abspath, 0, VFS_PATH_MAX);
        int len_filename = strlen(filename);
        int len_inode = strlen(dir->inode->name);
        int len_relative = strlen(dir->relative_path);
        if(len_inode + len_relative >= VFS_PATH_MAX) return -1;
        strncpy(abspath, dir->inode->name, len_inode);
        strncpy(abspath+len_inode, dir->relative_path, len_relative);

        if(len_filename >= 2 && filename[0]=='.' && filename[1]=='/')
        {
            if(len_filename - 1 + len_inode + len_relative >= VFS_PATH_MAX) return -1;
            strcpy(abspath+len_inode+len_relative, filename+1);
        } else if(len_filename == 1 && filename[0]=='.')
        {
            abspath[len_inode+len_relative] = 0;
            dir = getcpu()->cur_proc->cwd;
            iscwddir = 1;
        } else {
            if(len_filename + len_inode + len_relative + 1 >= VFS_PATH_MAX) return -1;
            abspath[len_inode+len_relative] = '/';
            strcpy(abspath+len_inode+len_relative+1, filename);
        }
        path = abspath;
    }

    if(iscwddir)
    {
        return -1;
    }
    else if(flags) //删除的是目录
    {
        if(!vfs_rmdir(abspath)) return 0;
        else return -1;
    }
    else // 删除的是文件
    {
        if(!vfs_unlink(abspath)) return 0;
    }
    return -1;
}

int sys_mount(const char *dev, const char *mntpoint, const char *fstype, unsigned long flags, const void *data)
{
    if(!dev || !mntpoint || !fstype) return -1;

    char *path = NULL;
    char abspath[VFS_PATH_MAX];
    vfs_dir_t *dir = NULL;

    if(mntpoint[0] == '/') // 绝对路径
    {
        path = mntpoint;
    } else // 相对路径
    {
        dir = getcpu()->cur_proc->cwd;
        memset(abspath, 0, VFS_PATH_MAX);
        int len_filename = strlen(mntpoint);
        int len_inode = strlen(dir->inode->name);
        int len_relative = strlen(dir->relative_path);
        if(len_inode + len_relative >= VFS_PATH_MAX) return -1;
        strncpy(abspath, dir->inode->name, len_inode);
        strncpy(abspath+len_inode, dir->relative_path, len_relative);

        if(len_filename >= 2 && mntpoint[0]=='.' && mntpoint[1]=='/')
        {
            if(len_filename - 1 + len_inode + len_relative >= VFS_PATH_MAX) return -1;
            strcpy(abspath+len_inode+len_relative, mntpoint+1);
        } else if(len_filename == 1 && mntpoint[0]=='.')
        {
            return -1;
        } else {
            if(len_filename + len_inode + len_relative + 1 >= VFS_PATH_MAX) return -1;
            abspath[len_inode+len_relative] = '/';
            strcpy(abspath+len_inode+len_relative+1, mntpoint);
        }
        path = abspath;
    }

    vfs_err_t err;
    err = vfs_block_device_register(dev, &sd_dev);
    if(err != VFS_ERR_NONE && err != VFS_ERR_DEVICE_ALREADY_REGISTERED) return -1;
    err = vfs_fs_mount(dev, path, fstype);
    if(err != VFS_ERR_NONE)
        return -1;
    return 0;
}

int sys_umount2(const char *mntpoint, unsigned long flags)
{
    if(!mntpoint) return -1;

    char *path = NULL;
    char abspath[VFS_PATH_MAX];
    vfs_dir_t *dir = NULL;

    if(mntpoint[0] == '/') // 绝对路径
    {
        path = mntpoint;
    } else // 相对路径
    {
        dir = getcpu()->cur_proc->cwd;
        memset(abspath, 0, VFS_PATH_MAX);
        int len_filename = strlen(mntpoint);
        int len_inode = strlen(dir->inode->name);
        int len_relative = strlen(dir->relative_path);
        if(len_inode + len_relative >= VFS_PATH_MAX) return -1;
        strncpy(abspath, dir->inode->name, len_inode);
        strncpy(abspath+len_inode, dir->relative_path, len_relative);

        if(len_filename >= 2 && mntpoint[0]=='.' && mntpoint[1]=='/')
        {
            if(len_filename - 1 + len_inode + len_relative >= VFS_PATH_MAX) return -1;
            strcpy(abspath+len_inode+len_relative, mntpoint+1);
        } else if(len_filename == 1 && mntpoint[0]=='.')
        {
            return -1;
        } else {
            if(len_filename + len_inode + len_relative + 1 >= VFS_PATH_MAX) return -1;
            abspath[len_inode+len_relative] = '/';
            strcpy(abspath+len_inode+len_relative+1, mntpoint);
        }
        path = abspath;
    }
    int rd = vfs_fs_umount(path);
    if(rd != 0) return -1;
    return 0;
}
