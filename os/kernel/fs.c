#include "fs.h"
#include "fatfs_drv.h"
#include "fatfs_vfs.h"
#include "vfs.h"
#include "cpu.h"
#include "printk.h"
#include "process.h"
#include "kmalloc.h"
#include "console.h"
#include "pipe.h"
#include "mm.h"
#include "page.h"
#include "vmm.h"
#include "sched.h"
#include "cfgfs.h"
#include "signal.h"
#include "timer.h"

extern volatile int is_sd_init;

int fs_init(void)
{
    is_sd_init = 0;
    vfs_err_t err;
    err = vfs_block_device_register("/dev/sdcard", &sd_dev);
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("register sd device error %d\n", err);
        #endif
        return 0;
    }

    err = vfs_char_device_register("/dev/stdin", &stdin);
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("register stdin device error %d\n", err);
        #endif
        return 0;
    }
    err = vfs_char_device_register("/dev/stdout", &stdout);
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("register stdout device error %d\n", err);
        #endif
        return 0;
    }

    err = vfs_char_device_register("/dev/stderr", &stderr);
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("register stderr device error %d\n", err);
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
    err = vfs_fs_mount("/dev/sdcard", "/root", "vfat");
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("mount fs error %d\n", err);
        #endif
        return 0;
    }

    err = vfs_char_device_register("/dev/etc", &etc_dev);
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("register etc device error %d\n", err);
        #endif
        return 0;
    }

    err = vfs_fs_register("etcfs", &etc_fs);
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("register etcfs error %d\n", err);
        #endif
        return 0;
    }
    err = vfs_fs_mount("/dev/etc", "/etc", "etcfs");
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("mount etcfs error %d\n", err);
        #endif
        return 0;
    }
    
    err = vfs_char_device_register("/dev/proc", &proc_dev);
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("register proc device error %d\n", err);
        #endif
        return 0;
    }

    err = vfs_fs_register("procfs", &proc_fs);
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("register procfs error %d\n", err);
        #endif
        return 0;
    }
    err = vfs_fs_mount("/dev/proc", "/proc", "procfs");
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("mount procfs error %d\n", err);
        #endif
        return 0;
    }

    err = vfs_char_device_register("/dev/zero", &zero);
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("register zero device error %d\n", err);
        #endif
        return 0;
    }

    err = vfs_char_device_register("/dev/null", &null);
    if(err != VFS_ERR_NONE)
    {
        #ifdef _DEBUG
        printk("register null device error %d\n", err);
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
    list *l = proc->ufiles_list_head.next;

    while(l!=&proc->ufiles_list_head)
    {
        ufile_t *f = list2ufile(l);
        if(l->next == &proc->ufiles_list_head) // ????????????????????????????????????????????????
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

    list *l = proc->ufiles_list_head.next;
    if(fd < list2ufile(l)->ufd)
    {
        kfree(file);
        return NULL;
    }

    while(l!=&proc->ufiles_list_head)
    {
        ufile_t *f = list2ufile(l);
        if(f->ufd == fd) // ?????????
            break;
        if(l->next == &proc->ufiles_list_head) // ????????????????????????????????????????????????
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
    list *l = proc->ufiles_list_head.next;
    while(l!=&proc->ufiles_list_head)
    {
        ufile_t *f = list2ufile(l);
        if(f->ufd == ufd)
            return f;
        l = l->next;
    }
    return NULL;
}

static void add_root(char *path)
{
    int len = strlen(path);
    if(strncmp(path, "/var", 4) == 0)
    {
        memmove(path + 5, path, len + 1);
        memcpy(path, "/root", 5);
    }
}

int calc_abspath(vfs_dir_t *dir, char *path, char *abspath)
{
    if(path[0] == '/') // ????????????
    {
        if(strlen(path) >= VFS_PATH_MAX)
        {
            // printk("to long path: %s\n", path);
            return -1;
        }
        strcpy(abspath, path);
        add_root(abspath);
        return 0;
    } else // ????????????
    {
        if(!dir) return -1;
        int len_filename = strlen(path);
        int len_inode = strlen(dir->inode->name);
        int len_relative = strlen(dir->relative_path);
        if(len_inode + len_relative >= VFS_PATH_MAX) return -1;
        strncpy(abspath, dir->inode->name, len_inode);
        strncpy(abspath+len_inode, dir->relative_path, len_relative);

        if(len_filename >= 2 && path[0]=='.' && path[1]=='/')
        {
            if(len_filename - 1 + len_inode + len_relative >= VFS_PATH_MAX) return -1;
            strcpy(abspath+len_inode+len_relative, path+1);
            add_root(abspath);
            return 0;
        } else if(len_filename == 1 && path[0]=='.')
        {
            abspath[len_inode+len_relative] = 0;
            return 1; // means open cwd dir
        } else {
            if(len_filename + len_inode + len_relative + 1 >= VFS_PATH_MAX) return -1;
            abspath[len_inode+len_relative] = '/';
            strcpy(abspath+len_inode+len_relative+1, path);
            add_root(abspath);
            return 0;
        }
    }
    return -1;
}

char* sys_getcwd(char *buf, int size)
{
    if(!buf || size <= 1) return NULL;
    buf[--size] = 0; // ?????????????????????0
    struct Process *proc = getcpu()->cur_proc;
    int leninode = strlen(proc->cwd->inode->name);
    strcpy(buf, proc->cwd->inode->name);
    int delt = size - leninode;
    if(delt < 0) return buf;
    strcpy(buf+leninode, proc->cwd->relative_path);
    if(!strlen(buf))
        buf[0] = '/';
    return buf;
}

int sys_chdir(const char *dirpath)
{
    if(!dirpath || !strlen(dirpath)) return -1;

    char abspath[VFS_PATH_MAX] = {0};

    int rt = calc_abspath(getcpu()->cur_proc->cwd, dirpath, abspath);

    if(rt < 0) return -1;

    vfs_dir_t *new_dir = vfs_opendir(abspath);
    if(!new_dir) return -1;
    vfs_closedir(getcpu()->cur_proc->cwd);
    getcpu()->cur_proc->cwd = new_dir;
    return 0;
}

int sys_openat(int ufd, char *filename, int flags, int mode)
{
    if(!filename || !strlen(filename)) return -1;
    char abspath[VFS_PATH_MAX] = {0};
    vfs_dir_t *dir = NULL;
    // printk("open filename %s\n", filename);
    if(ufd == AT_FDCWD) // ????????????????????????
    {
        dir = getcpu()->cur_proc->cwd;
    } else // ??????fd
    {
        struct ufile *ufile = ufd2ufile(ufd, getcpu()->cur_proc);
        if(ufile && ufile->type == UTYPE_DIR) dir = ufile->private;
    }

    int rt = calc_abspath(dir, filename, abspath);
    if(rt < 0) return -1;

    // printk("openat abspath: %s\n", abspath);

    if(!strcmp(abspath, "/dev/rtc"))
    {
        strcpy(abspath, "/etc/adjtime");
        abspath[12] = 0;
    }

    if(rt == 1)
    {
        dir = vfs_opendir(abspath);
        struct ufile *file = ufile_alloc(getcpu()->cur_proc);
        file->type = UTYPE_DIR;
        file->private = dir;
        return file->ufd;
    }
    else if((flags & O_DIRECTORY)) //??????????????????
    {
        dir = vfs_opendir(abspath);
        if(!dir)
        {
            if(!(flags & O_CREAT)) return -1;
            int rt = vfs_mkdir(abspath);
            if(rt < 0) return -1;
            dir = vfs_opendir(abspath);
            if(!dir) return -1;
        }
        struct ufile *file = ufile_alloc(getcpu()->cur_proc);
        file->type = UTYPE_DIR;
        file->private = dir;
        return file->ufd;
    }
    else // ??????????????????
    {
        int fd = vfs_open(abspath, flags);
        if(fd < 0) return -1;
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
    } else if(!vfs_close((int)file->private)) rt = 0;
    ufile_free(file);
    return rt;
}

int sys_mkdirat(int dirfd, char *dirpath, int mode)
{
    if(!dirpath || !strlen(dirpath)) return -1;

    char abspath[VFS_PATH_MAX] = {0};
    vfs_dir_t *dir = NULL;
    if(dirfd == AT_FDCWD) // ????????????????????????
    {
        dir = getcpu()->cur_proc->cwd;
    } else // ??????fd
    {
        struct ufile *ufile = ufd2ufile(dirfd, getcpu()->cur_proc);
        if(ufile && ufile->type == UTYPE_DIR) dir = ufile->private;
    }

    int rt = calc_abspath(dir, dirpath, abspath);

    if(strcmp(abspath, "/") == 0) return 0;

    if(rt != 0) return -1;

    int len = strlen(abspath);

    if(abspath[len-1] == '/')
    {
        abspath[len-1] = '\0';
        len--;
    }

    
    dir = vfs_opendir(abspath);
    if(dir) // ??????????????????????????????
    {
        vfs_closedir(dir);
        return 0;
    }
    int fd = vfs_open(abspath, O_RDONLY);
    if(fd > 0) // ?????????????????????
    {
        vfs_close(fd);
        return -1;
    }
    if(!vfs_mkdir(abspath)) return 0;
    else return -1;
}

uint64 sys_read(int ufd, uchar *buf, uint64 count)
{
    if(!buf) return -1;
    struct ufile *file = ufd2ufile(ufd, getcpu()->cur_proc);
    if(!file) return -1;
    if(file->type == UTYPE_STDIN || file->type == UTYPE_STDOUT || file->type == UTYPE_STDERR)
    {
        return vfs_read((int)file->private, buf, count);
    } else if(file->type == UTYPE_PIPEIN)
    {
        int rt = pipe_read(file->private, buf, count);
        return rt;
    }else if(file->type == UTYPE_FILE)
    {
        // printk("hartid %d pid %d read %s\n", gethartid(), getcpu()->cur_proc->pid, vfs_fd2file((int)file->private)->relative_path);
        return vfs_read((int)file->private, buf, count);
    }
    return -1;
}

uint64 XXXcnt = 0;

uint64 sys_write(int ufd, uchar *buf, uint64 count)
{
    if(!buf) return -1;
    struct ufile *file = ufd2ufile(ufd, getcpu()->cur_proc);
    if(!file) return -1;

    #ifdef _DEBUG
    printk("\n\npid %d write fd %d type: %d addr %lx count %ld\n\n", getcpu()->cur_proc->pid, file->ufd, file->type, file, count);
    #endif

    if(file->type == UTYPE_STDOUT || file->type == UTYPE_STDIN || file->type == UTYPE_STDERR)
    {
        return vfs_write((int)file->private, buf, count);
    } else if(file->type == UTYPE_PIPEOUT)
    {
        return pipe_write(file->private, buf, count);
    } else if(file->type == UTYPE_FILE)
    {
        // printk("hartid %d pid %d write %s\n", gethartid(), getcpu()->cur_proc->pid, vfs_fd2file((int)file->private)->relative_path);
        vfs_file_t *f = vfs_fd2file((int)file->private);
        if(strcmp(f->relative_path, "/root/var/tmp/XXX") == 0)
        {
            if(++XXXcnt > 128)
                return count;
        }
        //printk("filename %s\n", f->relative_path);
        int rt = vfs_write((int)file->private, buf, count);
        vfs_sync((int)file->private);
            return rt;
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
        vfs_file_t *vf = vfs_fd2file((int)file->private);
        int nfd = vfs_open(vf->relative_path, vf->flags);
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
        int nfd = vfs_open("/dev/stdin", O_RDONLY);
        if(nfd < 0)
        {
            ufile_free(newfile);
            return -1;
        }
        newfile->private = (void*)nfd;
        return newfile->ufd;
    } else if(file->type == UTYPE_STDOUT)
    {
        newfile->type = UTYPE_STDOUT;
        int nfd = vfs_open("/dev/stdout", O_WRONLY);
        if(nfd < 0)
        {
            ufile_free(newfile);
            return -1;
        }
        newfile->private = (void*)nfd;
        return newfile->ufd;
    } else if(file->type == UTYPE_STDERR)
    {
        newfile->type = UTYPE_STDERR;
        int nfd = vfs_open("/dev/stderr", O_WRONLY);
        if(nfd < 0)
        {
            ufile_free(newfile);
            return -1;
        }
        newfile->private = (void*)nfd;
        return newfile->ufd;
    } else if(file->type == UTYPE_DIR)
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
    struct Process *proc = getcpu()->cur_proc;
    ufile_t *file = ufd2ufile(fd, proc);
    if(!file) return -1;
    ufile_t *newfile = ufd2ufile(nfd, proc); 
    if(newfile)
    { 
        if(newfile->type == UTYPE_DIR)
        {
            vfs_closedir(newfile->private);
        } else if(newfile->type == UTYPE_PIPEIN)
        {
            pipe_close(newfile->private, UTYPE_PIPEIN);
        } else if(newfile->type == UTYPE_PIPEOUT)
        {
            pipe_close(newfile->private, UTYPE_PIPEOUT);
        } else vfs_close((int)newfile->private);
    } else
    {
        newfile = ufile_alloc_by_fd(nfd, proc);
        if(!newfile)
            return -1;
    }

    if(file->type == UTYPE_FILE)
    {
        newfile->type = UTYPE_FILE;
        vfs_file_t *vf = vfs_fd2file((int)file->private);
        int nfd = vfs_open(vf->relative_path, vf->flags);
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
        int nfd = vfs_open("/dev/stdin", O_RDONLY);
        if(nfd < 0)
        {
            ufile_free(newfile);
            return -1;
        }
        newfile->private = (void*)nfd;
        return newfile->ufd;
    } else if(file->type == UTYPE_STDOUT)
    {
        newfile->type = UTYPE_STDOUT;
        int nfd = vfs_open("/dev/stdout", O_WRONLY);
        if(nfd < 0)
        {
            ufile_free(newfile);
            return -1;
        }
        newfile->private = (void*)nfd;
        return newfile->ufd;
    } else if(file->type == UTYPE_STDERR)
    {
        newfile->type = UTYPE_STDERR;
        int nfd = vfs_open("/dev/stderr", O_WRONLY);
        if(nfd < 0)
        {
            ufile_free(newfile);
            return -1;
        }
        newfile->private = (void*)nfd;
        return newfile->ufd;
    } else if(file->type == UTYPE_DIR)
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

    uint64 nameoffset =  (uint64)&buf->d_name[0] - (uint64)buf;
    vfs_readdir(dir);
    int namelen = strlen(dir->dirent.name);
    if(!namelen) return 0;
    if(p + nameoffset + namelen + 1 >= (uint64)buf + len)
        return 0;
    struct linux_dirent64 *pbuf = p;
    pbuf->d_ino = dir->inode;
    pbuf->d_off = dir->pos;
    int reclen = nameoffset + namelen + 1;
    if(reclen % 8 != 0) // ????????????
    {
        reclen = ((reclen >> 3) + 1) << 3;
    }
    pbuf->d_reclen = reclen;
    pbuf->d_type = dir->dirent.type;
    strcpy(pbuf->d_name, dir->dirent.name);
    pbuf->d_name[namelen] = '\0';
    return reclen;
}

char *sh_abspath = "/root/busybox";
char *argvsh[10] = {"./busybox", "sh", NULL};

int sys_execve(const char *name, char *const argv[], char *const envp[])
{
    if(!name || !strlen(name)) return -1;

    char abspath[VFS_PATH_MAX] = {0};
    
    int rt = calc_abspath(getcpu()->cur_proc->cwd, name, abspath);
    if(rt != 0) return -1;
    
    int len = strlen(abspath);

    // printk("--------------------\n--------------------\npid %d sys execv %s\n--------------------\n", getcpu()->cur_proc->pid, abspath);
    
    if(abspath[len-1] == 'h' && abspath[len-2] == 's' && abspath[len-3] == '.')
    {
        int newi = 2;
        int i = 0;
        while(argv[i])
        {
            argvsh[newi++] = argv[i++];
        }
        argvsh[newi] = NULL;
        name = sh_abspath;
        argv = argvsh;
    } else name = abspath;
    
    struct Process *cur_proc = getcpu()->cur_proc;
    struct Process *new_proc = create_proc_by_elf(name, argv, envp);
    if(!new_proc) return -1;
    
    // ?????????????????????????????????????????????????????????????????????????????????????????????
    // ???????????????????????????
    vm_free_pages(new_proc->k_stack, 1);
    new_proc->k_stack = cur_proc->k_stack;
    cur_proc->k_stack = NULL;

    switch2kpgt();  // ????????????????????????????????????????????????????????????
    free_process_mem(cur_proc); // ?????????????????????
    free_pagetable(cur_proc->pg_t);
    
    if(cur_proc->kfd)
        vfs_close(cur_proc->kfd);

    list *l = new_proc->vma_list_head.next;
    struct vma *vma = NULL;
    while(l != &new_proc->vma_list_head)
    {
        vma = vma_list_node2vma(l);
        l = l->next;
        add_before(&cur_proc->vma_list_head, &vma->vma_list_node);
    }

    cur_proc->k_stack = new_proc->k_stack;
    new_proc->k_stack = NULL;
    new_proc->tcontext.k_sp = cur_proc->tcontext.k_sp;
    cur_proc->pg_t = new_proc->pg_t;
    new_proc->pg_t = NULL;
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
    cur_proc->kfd = new_proc->kfd;

    init_proc_sigactions(cur_proc); // ???????????????sigactions
    free_process_sigpendings(cur_proc);
    free_process_struct(new_proc);


    // free_ufile_list(cur_proc);
    // init_list(&cur_proc->ufiles_list_head);

    // ufile_t *file = kmalloc(sizeof(ufile_t)); // ?????????STDIN??????
    // file->ufd = 0;
    // file->type = UTYPE_STDIN;
    // file->private = (void*)vfs_open("/dev/stdin", O_RDONLY);
    // init_list(&file->list_node);
    // add_before(&cur_proc->ufiles_list_head, &file->list_node);

    // file = kmalloc(sizeof(ufile_t)); // ?????????STDOUT??????
    // file->ufd = 1;
    // file->type = UTYPE_STDOUT;
    // file->private = (void*)vfs_open("/dev/stdout", O_WRONLY);
    // init_list(&file->list_node);
    // add_before(&cur_proc->ufiles_list_head, &file->list_node);
    
    // file = kmalloc(sizeof(ufile_t)); // ?????????STDERR??????
    // file->ufd = 2;
    // file->type = UTYPE_STDERR;
    // file->private = (void*)vfs_open("/dev/stderr", O_WRONLY);
    // init_list(&file->list_node);
    // add_before(&cur_proc->ufiles_list_head, &file->list_node);
    
    set_sscratch(&cur_proc->tcontext);
    // ??????????????????????????????
    uint64 sapt = ((((uint64)cur_proc->pg_t) - PV_OFFSET) >> 12) | (8L << 60);
    set_satp(sapt);
    sfence_vma();

    cur_proc->utime_start = get_time();
    trap_ret();
    return -1;
}

void static_fstat(struct kstat *kst, ufile_type_t type)
{
    vfs_mode_t mode;
    if(type == UTYPE_STDOUT || type == UTYPE_STDIN || type == UTYPE_STDERR)
    {
        mode = VFS_MODE_IFCHR;
    } else if(type == UTYPE_DIR)
    {
        mode = VFS_MODE_IFDIR;
    }

    kst->st_dev = 1;
    kst->st_ino = 1;
    kst->st_mode = mode | 0600;
    kst->st_nlink = 1;
    kst->st_uid = 0;
    kst->st_gid = 0;
    kst->st_rdev = 1;
    kst->st_size = 512 * 4;
    kst->st_blksize = 512;
    kst->st_blocks = 4;
    kst->st_atime_sec = 1626566400;
    kst->st_atime_nsec = 0;
    kst->st_mtime_sec = 1626566400;
    kst->st_mtime_nsec = 0;
    kst->st_ctime_sec = 1626566400;
    kst->st_ctime_nsec = 0;    
}

int sys_fstat(int ufd, struct kstat *kst)
{
    if(!kst) return -1;
    ufile_t *file = ufd2ufile(ufd, getcpu()->cur_proc);
    if(!file) return -1;

    if(file->type != UTYPE_FILE)
    {
        static_fstat(kst, file->type);
        return 0;
    }

    vfs_fstat_t st;
    vfs_file_t *vfile = vfs_fd2file((int)file->private);
    if(is_cfg_file(vfile))
    {
        int err = vfs_fstat((int)file->private, &st);
        if(err < 0) return -1;
        kst->st_dev = 1;
        kst->st_mode = st.mode;
        kst->st_nlink = 1;
        kst->st_uid = 0;
        kst->st_gid = 0;
        kst->st_rdev = 0;
        kst->st_size = st.size;
        kst->st_blksize = st.blk_size;
        kst->st_blocks = st.blk_num;
        kst->st_atime_sec = st.atime;
        kst->st_atime_nsec = 0;
        kst->st_mtime_sec = st.mtime;
        kst->st_mtime_nsec = 0;
        kst->st_ctime_sec = st.ctime;
        kst->st_ctime_nsec = 0;
        return 0;
    }

    if(vfs_stat(vfile->relative_path, &st) < 0) return -1;
    
    kst->st_dev = 1;
    kst->st_mode = st.mode;
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

    char abspath[VFS_PATH_MAX] = {0};
    vfs_dir_t *dir = NULL;

    if(ufd == AT_FDCWD) // ????????????????????????
    {
        dir = getcpu()->cur_proc->cwd;
    } else // ??????fd
    {
        struct ufile *ufile = ufd2ufile(ufd, getcpu()->cur_proc);
        if(ufile && ufile->type == UTYPE_DIR) dir = ufile->private;
    }

    int rt = calc_abspath(dir, filename, abspath);

    if(rt != 0)
        return -1;
    
    if(flags & O_DIRECTORY) //??????????????????
    {
        if(!vfs_rmdir(abspath)) return 0;
        else return -1;
    }
    else // ??????????????????
    {
        if(!vfs_unlink(abspath)) return 0;
    }
    return -1;
}

int sys_mount(const char *dev, const char *mntpoint, const char *fstype, unsigned long flags, const void *data)
{
    if(!dev || !mntpoint || !fstype) return -1;

    char abspath[VFS_PATH_MAX] = {0};

    int rt = calc_abspath(getcpu()->cur_proc->cwd, mntpoint, abspath);

    if(rt != 0) return -1;

    vfs_err_t err;
    err = vfs_block_device_register(dev, &sd_dev);
    if(err != VFS_ERR_NONE && err != VFS_ERR_DEVICE_ALREADY_REGISTERED) return -1;
    err = vfs_fs_mount(dev, abspath, fstype);
    if(err != VFS_ERR_NONE)
        return -1;
    return 0;
}

int sys_umount2(const char *mntpoint, unsigned long flags)
{
    if(!mntpoint) return -1;

    char abspath[VFS_PATH_MAX] = {0};

    int rt = calc_abspath(getcpu()->cur_proc->cwd, mntpoint, abspath);

    if(rt != 0) return -1;

    int rd = vfs_fs_umount(abspath);
    if(rd != 0) return -1;
    return 0;
}

uint64 sys_readv(int ufd, struct iovec *iov, uint64 iovcnt)
{
    if(!iov) return -1;
    struct ufile *file = ufd2ufile(ufd, getcpu()->cur_proc);
    if(!file) return -1;
    int i;
    uint64 cnt = 0;
    if(file->type == UTYPE_FILE || file->type == UTYPE_STDIN || file->type == UTYPE_STDOUT || file->type == UTYPE_STDERR)
    {
        for(i=0; i<iovcnt; i++)
        {
            struct iovec *io = &iov[i];
            if(io->iov_base)
                cnt += vfs_read((int)file->private, io->iov_base, io->iov_len);
        }
        return cnt;
    } else if(file->type == UTYPE_PIPEIN)
    {
        for(i=0; i<iovcnt; i++)
        {
            struct iovec *io = &iov[i];
            if(io->iov_base)
                cnt += pipe_read(file->private, io->iov_base, io->iov_len);
        }
        return cnt;
    }
    return -1;
}

uint64 sys_writev(int ufd, struct iovec *iov, uint64 iovcnt)
{
    if(!iov) return -1;
    struct ufile *file = ufd2ufile(ufd, getcpu()->cur_proc);
    if(!file) return -1;
    int i;
    uint64 cnt = 0;
    if(file->type == UTYPE_FILE || file->type == UTYPE_STDOUT || file->type == UTYPE_STDIN || file->type == UTYPE_STDERR)
    {
        for(i=0; i<iovcnt; i++)
        {
            struct iovec *io = &iov[i];
            if(io->iov_base)
                cnt += vfs_write((int)file->private, io->iov_base, io->iov_len);
        }
        return cnt;
    } else if(file->type == UTYPE_PIPEOUT)
    {
        for(i=0; i<iovcnt; i++)
        {
            struct iovec *io = &iov[i];
            if(io->iov_base)
                cnt += pipe_write(file->private, io->iov_base, io->iov_len);
        }
        return cnt;
    }
    return -1;
}

uint64 sys_readlinkat(int ufd, char *path, char *buf, uint64 bufsize)
{
    if(strcmp(path, "/proc/self/exe") == 0)
    {
        struct Process *proc = getcpu()->cur_proc;
        vfs_file_t *file = vfs_fd2file(proc->kfd);
        uint64 len = strlen(file->relative_path);
        strncpy(buf, file->relative_path, len+1);
        return len;
    } else {
        printk("readlinkat %s\n", path);
        while(1) {}
    }
    return strlen("/root/busybox");
}

int sys_ioctl(int ufd, uint64 request)
{
    return 0;
}

int sys_fcntl(int ufd, uint cmd, uint64 arg)
{
    if(cmd == 0 || cmd == F_DUPFD_CLOEXEC)
        return sys_dup(ufd);
    else return 0;
}

int sys_fstatat(int dirfd, const char *pathname, struct kstat *buf, int flags)
{
    if(!pathname || !strlen(pathname)) return -1;
    char abspath[VFS_PATH_MAX] = {0};
    vfs_dir_t *dir = NULL;

    if(dirfd == AT_FDCWD) // ????????????????????????
    {
        dir = getcpu()->cur_proc->cwd;
    } else // ??????fd
    {
        struct ufile *ufile = ufd2ufile(dirfd, getcpu()->cur_proc);
        if(ufile && ufile->type == UTYPE_DIR) dir = ufile->private;
    }

    int rt = calc_abspath(dir, pathname, abspath);
    if(rt < 0) return -1;

    #ifdef _DEBUG
    printk("path name %s fstatat %s\n", pathname, abspath);
    #endif

    if(rt == 1 || (flags & O_DIRECTORY)) //??????????????????
    {
        dir = vfs_opendir(abspath);
        if(!dir) return -1;
        buf->st_dev = 1;
        buf->st_mode = VFS_MODE_IFDIR;
        buf->st_nlink = 1;
        buf->st_uid = 0;
        buf->st_gid = 0;
        buf->st_rdev = 0;
        buf->st_size = 0;//st.size;
        buf->st_blksize = 512;
        buf->st_blocks = 0;
        buf->st_atime_sec = 1626566400;
        buf->st_atime_nsec = 0;
        buf->st_mtime_sec = 1626566400;
        buf->st_mtime_nsec = 0;
        buf->st_ctime_sec = 1626566400;
        buf->st_ctime_nsec = 0;
        uint64 size = 0;
        
        while(1)
        {
            vfs_readdir(dir);
            int namelen = strlen(dir->dirent.name);
            if(!namelen) break;
            size += dir->dirent.size;
        }
        buf->st_size = size;
        buf->st_blocks = buf->st_size / buf->st_blocks;
        vfs_closedir(dir);
        return 0;
    }
    else // ??????????????????
    {
        vfs_fstat_t st;
        if(vfs_stat(abspath, &st) < 0) return -1;
        buf->st_dev = 1;
        buf->st_mode = st.mode;
        buf->st_nlink = 1;
        buf->st_uid = 0;
        buf->st_gid = 0;
        buf->st_rdev = 0;
        buf->st_size = st.size;
        buf->st_blksize = 512;
        buf->st_blocks = st.blk_num;
        buf->st_atime_sec = st.atime;
        buf->st_atime_nsec = 0;
        buf->st_mtime_sec = st.mtime;
        buf->st_mtime_nsec = 0;
        buf->st_ctime_sec = st.ctime;
        buf->st_ctime_nsec = 0;
        return 0;
    }
    return -1;
}

int sys_statfs(char *path, struct statfs *buf)
{
    buf->f_type = 0;
    buf->f_bsize = 512;
    buf->f_blocks = 128;
    buf->f_bfree = 100;
    buf->f_bfree = 128;
    buf->f_files = 100;
    buf->f_ffree = 90;
    buf->f_namelen = 64;
    return 0;
}

int sys_syslog(int type, char *buf, int len)
{
    switch (type)
    {
    case 0:
        /* code */
        return 0;
        break;
    case 1:
        return 0;
    case 2:
    {
        if(!buf) return 0;
        strcpy(buf, "syslog read");
        return strlen("syslog read");
    }
    case 3:
    {
        if(!buf) return 0;
        strcpy(buf, "syslog read");
        return strlen("syslog read");
    }
    case 4:
    {
        if(!buf) return 0;
        strcpy(buf, "syslog read");
        return strlen("syslog read");
    }
    case 5:
        return 0;
    case 6:
        return 0;
    case 7:
        return 0;
    case 8:
        return 0;
    case 9:
        return 0;
    case 10:
        return 512;
    default:
        return 0;
    }
}

int sys_faccessat(int fd, const char *pathname, int mode, int flag)
{
    return 0;
}

int sys_utimensat(int dirfd, const char *pathname, const struct TimeSpec times[2], int flags)
{
    if(!pathname || !strlen(pathname)) return -1;
    char abspath[VFS_PATH_MAX] = {0};
    vfs_dir_t *dir = NULL;

    if(dirfd == AT_FDCWD) // ????????????????????????
    {
        dir = getcpu()->cur_proc->cwd;
    } else // ??????fd
    {
        struct ufile *ufile = ufd2ufile(dirfd, getcpu()->cur_proc);
        if(ufile && ufile->type == UTYPE_DIR) dir = ufile->private;
    }

    int rt = calc_abspath(dir, pathname, abspath);
    if(rt < 0) return -1;

    dir = vfs_opendir(abspath);
    if(dir) // ??????????????????????????????
    {
        vfs_closedir(dir);
        return 0;
    }

    vfs_fstat_t st;
    rt = vfs_stat(abspath, &st);

    if(!rt) return 0;
    
    int fd = vfs_open(abspath, O_CREAT);

    if(fd < 0) return -1;

    vfs_close(fd);
    return 0;
}

int sys_ppoll(struct pollfd *fds, int nfds, struct TimeSpec *timeout, uint64 *sigmask, uint64 size)
{
    if(nfds < 0 || nfds >= FD_SETSIZE) return -1;
    return 1;

    uint rt = 0;
    int i;

    for(i = 0; i < nfds; i++)
    {
        struct pollfd *pfd = &fds[i];
        int ufd = pfd->fd;
        struct ufile *file = ufd2ufile(ufd, getcpu()->cur_proc);
        if(file)
        {
            switch (file->type)
            {
            case UTYPE_PIPEIN:
            {
                pipe_t *pipe = (pipe_t*)file->private;
                if(pipe->r_pos != pipe->w_pos)
                    rt++;
                break;
            }
            default:
                rt++;
                break;
            }
        }
    }
    return rt;
}

int sys_pselect6(int nfds, struct fd_set *readfds, struct fd_set *writefds, struct fd_set *exceptfds, struct TimeSpec *timeout, uint64 *sigmask)
{
    if(nfds < 0 || nfds >= FD_SETSIZE)
        return -1;

    if(!readfds && !writefds && !exceptfds)
    {
        if(timeout)
            sys_nanosleep(timeout, timeout);
        return 0;
    }
    uint64 endtimeclock = 0;
    
    if(timeout)
        endtimeclock = timeout->sec * TIMER_FRQ + timeout->nsec * TIMER_FRQ / 1000000000;
    
    int cnt = 0;
    int i;
    endtimeclock += get_time();

    do
    {
        for(i=0; i<nfds; i++)
        {
            if(readfds && FD_ISSET(i, readfds))
            {
                struct ufile *file = ufd2ufile(i, getcpu()->cur_proc);
                if(file)
                {
                    switch (file->type)
                    {
                    case UTYPE_PIPEIN:
                    {
                        pipe_t *pipe = (pipe_t*)file->private;
                        if(pipe->r_pos != pipe->w_pos)
                        {
                            FD_SET(i, readfds);
                            cnt++;
                        } else
                        {                            
                            FD_CLR(i, readfds);
                        }
                        break;
                    }
                    case UTYPE_PIPEOUT:
                    {
                        FD_CLR(i, readfds);
                        break;
                    }
                    default:
                        cnt++;
                        FD_SET(i, readfds);
                        break;
                    }
                } else
                {
                    FD_CLR(i, readfds);
                }
            }

            if(writefds && FD_ISSET(i, writefds))
            {
                struct ufile *file = ufd2ufile(i, getcpu()->cur_proc);
                if(file)
                {
                    switch (file->type)
                    {
                    case UTYPE_PIPEOUT:
                    {
                        FD_SET(i, writefds);
                        cnt++;
                        break;
                    }
                    case UTYPE_PIPEIN:
                    {
                        FD_CLR(i, writefds);
                        break;
                    }
                    default:
                        cnt++;
                        FD_SET(i, writefds);
                        break;
                    }
                } else
                {
                    FD_CLR(i, writefds);
                }
            }
        }
        if(!cnt && timeout)
        {
            struct TimeSpec ts;
            ts.sec = timeout->sec / 4;
            ts.nsec = timeout->nsec / 4;
            timeout->sec -= ts.sec;
            timeout->nsec -= ts.nsec;
            sys_nanosleep(&ts, &ts);
        }
    } while(endtimeclock >= get_time());
    
    if(exceptfds)
        FD_ZERO(exceptfds);

    return cnt;
}

int sys_msync(uint64 start, uint64 len, int flags)
{
    return 0;
}