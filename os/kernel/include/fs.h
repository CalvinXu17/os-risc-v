#ifndef FS_H
#define FS_H

#include "type.h"
#include "list.h"
#include "process.h"

#define O_RDONLY 0x000
#define O_WRONLY 0x001
#define O_RDWR 0x002 // 可读可写
//#define O_CREATE 0x200
#define O_CREATE 0x40
#define O_DIRECTORY 0x0200000

#define DIR 0x040000
#define FILE 0x100000

#define AT_FDCWD -100

typedef struct
{
    uint64 dev;    // 文件所在磁盘驱动器号，不考虑
    uint64 ino;    // inode 文件所在 inode 编号
    uint32 mode;   // 文件类型
    uint32 nlink;  // 硬链接数量，初始为1
    uint64 pad[7]; // 无需考虑，为了兼容性设计
} Stat;

typedef unsigned int mode_t;
typedef long int off_t;

struct kstat {
        uint64 st_dev;
        uint64 st_ino;
        mode_t st_mode;
        uint32 st_nlink;
        uint32 st_uid;
        uint32 st_gid;
        uint64 st_rdev;
        unsigned long __pad;
        off_t st_size;
        uint32 st_blksize;
        int __pad2;
        uint64 st_blocks;
        long st_atime_sec;
        long st_atime_nsec;
        long st_mtime_sec;
        long st_mtime_nsec;
        long st_ctime_sec;
        long st_ctime_nsec;
        unsigned __unused[2];
};

struct linux_dirent64 {
        uint64        d_ino;
        int64         d_off;
        unsigned short  d_reclen;
        unsigned char   d_type;
        char            d_name[];
};

typedef enum ufile_type {
    UTYPE_STDIN,
    UTYPE_STDOUT,
    UTYPE_PIPEIN,
    UTYPE_PIPEOUT,
    UTYPE_LINK,
    UTYPE_FILE,
    UTYPE_DIR,
} ufile_type_t;

typedef struct ufile {
    ufile_type_t type;
    int ufd;
    void *private;
    list list_node;
} ufile_t;
#define list2ufile(l)    GET_STRUCT_ENTRY(l, ufile_t, list_node)

int fs_init(void);
char* sys_getcwd(char *buf, int size);
int sys_chdir(const char *path);
int sys_openat(int fd, char *filename, int flags, int mode);
int sys_close(int fd);
int sys_mkdirat(int dirfd, char *dirpath, int mode);
uint64 sys_read(int ufd, uchar *buf, uint64 count);
uint64 sys_write(int ufd, uchar *buf, uint64 count);
int sys_pipe2(int fd[2]);
int sys_dup(int fd);
int sys_dup2(int fd, int nfd);
int sys_getdents64(int ufd, struct linux_dirent64 *buf, int len);
int sys_execve(const char *name, char *const argv[], char *const argp[]);
int sys_fstat(int ufd, struct kstat *kst);
int sys_unlinkat(int ufd, char *filename, int flags);
int sys_mount(const char *dev, const char *mntpoint, const char *fstype, unsigned long flags, const void *data);
int sys_umount2(const char *mntpoint, unsigned long flags);


struct ufile* ufile_alloc(struct Process *proc);
struct ufile* ufile_alloc_by_fd(int fd, struct Process *proc);
void ufile_free(ufile_t *file);
struct ufile* ufd2ufile(int ufd, struct Process *proc);

#endif