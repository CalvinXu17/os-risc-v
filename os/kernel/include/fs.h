#ifndef FS_H
#define FS_H

#include "type.h"
#include "list.h"
#include "process.h"
#include "syscall.h"
#include "string.h"

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
        uint64 __pad;
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

struct iovec {
    void   *iov_base;   /* 数据区的起始地址 */
    uint64 iov_len;     /* 数据区的大小 */
};

struct pollfd
{
  int   fd;      /* File descriptor to poll.  */
  short events;  /* Types of events poller cares about.  */
  short revents; /* Types of events that actually occurred.  */
};

typedef struct {
	int	val[2];
} fsid_t;

#define FD_SETSIZE 1024
#define F_DUPFD_CLOEXEC (FD_SETSIZE+6)

struct fd_set
{
    unsigned long fds_bits[FD_SETSIZE / (8 * sizeof(long))];
};

struct statfs
{ 
   long    f_type;     /* 文件系统类型  */ 
   long    f_bsize;    /* 经过优化的传输块大小  */ 
   long    f_blocks;   /* 文件系统数据块总数 */ 
   long    f_bfree;    /* 可用块数 */ 
   long    f_bavail;   /* 非超级用户可获取的块数 */ 
   long    f_files;    /* 文件结点总数 */ 
   long    f_ffree;    /* 可用文件结点数 */ 
   fsid_t  f_fsid;     /* 文件系统标识 */ 
   long    f_namelen;  /* 文件名的最大长度 */ 
}; 

#define POLLIN  01  /* There is data to read.  */
#define POLLPRI 02  /* There is urgent data to read.  */
#define POLLOUT 04  /* Writing now will not block.  */

typedef enum ufile_type {
    UTYPE_STDIN,
    UTYPE_STDOUT,
    UTYPE_STDERR,
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
int sys_execve(const char *name, char *const argv[], char *const envp[]);
int sys_fstat(int ufd, struct kstat *kst);
int sys_unlinkat(int ufd, char *filename, int flags);
int sys_mount(const char *dev, const char *mntpoint, const char *fstype, unsigned long flags, const void *data);
int sys_umount2(const char *mntpoint, unsigned long flags);

uint64 sys_readv(int ufd, struct iovec *iov, uint64 iovcnt);
uint64 sys_writev(int ufd, struct iovec *iov, uint64 iovcnt);
uint64 sys_readlinkat(int ufd, char *path, char *buf, uint64 bufsize);
int sys_ioctl(int ufd, uint64 request);
int sys_fcntl(int ufd, uint cmd, uint64 arg);
int sys_fstatat(int dirfd, const char *pathname, struct kstat *buf, int flags);
int sys_statfs(char *path, struct statfs *buf);
int sys_syslog(int type, char *buf, int len);
int sys_faccessat(int fd, const char *pathname, int mode, int flag);
int sys_utimensat(int dirfd, const char *pathname, const struct TimeSpec times[2], int flags);

int sys_ppoll(struct pollfd *fds, int nfds, struct TimeSpec *timeout, uint64 *sigmask, uint64 size);
int sys_pselect6(int nfds, struct fd_set *readfds, struct fd_set *writefds, struct fd_set *exceptfds, struct TimeSpec *timeout, uint64 *sigmask);
int sys_msync(uint64 start, uint64 len, int flags);

struct ufile* ufile_alloc(struct Process *proc);
struct ufile* ufile_alloc_by_fd(int fd, struct Process *proc);
void ufile_free(ufile_t *file);
struct ufile* ufd2ufile(int ufd, struct Process *proc);


static inline void FD_ZERO(struct fd_set *set)
{
	memset(set, 0, 16);
}

static inline void FD_SET(int fd, struct fd_set *set)
{
	if (fd < 0 || fd >= FD_SETSIZE)
		return;
	set->fds_bits[fd / (sizeof(long) * 8)] |= 1 << (fd & ((sizeof(long) * 8)-1));
}

static inline void FD_CLR(int fd, struct fd_set *set)
{
	if (fd < 0 || fd >= FD_SETSIZE)
		return;
	set->fds_bits[fd / (sizeof(long) * 8)] &= ~(1 << (fd & ((sizeof(long) * 8)-1)));
}

static inline int FD_ISSET(int fd, struct fd_set *set)
{
    if (fd < 0 || fd >= FD_SETSIZE)
		return 0;
    long bit = set->fds_bits[fd / (sizeof(long) * 8)] & (1 << (fd & ((sizeof(long) * 8)-1)));
    return bit != 0;
}

#endif