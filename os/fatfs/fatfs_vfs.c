#include "ff.h"
#include "vfs.h"
#include "kmalloc.h"
#include "string.h"

static BYTE fatfs_translate_oflag2mode(vfs_oflag_t flags)
{
    BYTE res      = 0;
    BYTE acc_mode = flags & O_ACCMODE;

    if (acc_mode == O_RDONLY) {
        res |= FA_READ;
    } else if (acc_mode == O_WRONLY) {
        res |= FA_WRITE;
    } else if (acc_mode == O_RDWR) {
        res |= FA_READ | FA_WRITE;
    }

    if (flags & O_CREAT) {
        res |= FA_OPEN_ALWAYS;
    }

    if (flags & O_TRUNC) {
        res |= FA_CREATE_ALWAYS;
    }

    if (flags & O_EXCL) {
        res |= FA_CREATE_NEW;
    }

    if (flags & O_APPEND) {
        res |= FA_OPEN_APPEND;
    }

    if (res == 0) {
        res |= FA_OPEN_EXISTING;
    }
    return res;
}

static int fatfs_open(vfs_file_t *file, const char *pathname, vfs_oflag_t flags)
{
    FIL *fp = NULL;
    FATFS *fatfs = NULL;
    FRESULT res;
    BYTE mode = 0;

    fp = kmalloc(sizeof(FIL));
    if (!fp) {
        return -1;
    }

    file->private = fp;
    fatfs = (FATFS *)file->inode->private;
    mode = fatfs_translate_oflag2mode(flags);

    res = f_open(fatfs, fp, pathname, mode);
    if (res != FR_OK) {
        file->private = NULL;
        kfree(fp);
        return -1;
    }

    return 0;
}

static uint64 fatfs_read(vfs_file_t *file, void *buf, uint64 count)
{
    FIL *fp = NULL;
    uint32 length;
    FRESULT res;

    fp = (FIL *)file->private;

    res = f_read(fp, buf, count, &length);
    if (res != FR_OK) {
        return -1;
    }

    return length;
}

static uint64 fatfs_write(vfs_file_t *file, const void *buf, uint64 count)
{
    FIL *fp = NULL;
    uint32 length;
    FRESULT res;

    fp = (FIL *)file->private;

    res = f_write(fp, buf, count, &length);
    if (res != FR_OK) {
        return -1;
    }

    return length;
}

static int fatfs_close(vfs_file_t *file)
{
    FIL *fp = NULL;
    FRESULT res;

    fp = (FIL *)file->private;

    res = f_close(fp);
    if (res != FR_OK) {
        return -1;
    }

    kfree(fp);
    file->private = NULL;

    return 0;
}

static vfs_off_t fatfs_lseek(vfs_file_t *file, vfs_off_t offset, vfs_whence_t whence)
{
    FIL *fp = NULL;
    FRESULT res;

    fp = (FIL *)file->private;

    res = f_lseek(fp, offset);
    if (res != FR_OK) {
        return -1;
    }

    return offset;
}

static int fatfs_sync(vfs_file_t *file)
{
    FIL *fp = NULL;
    FRESULT res;

    fp = (FIL *)file->private;

    res = f_sync(fp);
    if (res != FR_OK) {
        return -1;
    }

    return 0;
}

static int fatfs_truncate(vfs_file_t *file, vfs_off_t length)
{
    FIL *fp = NULL;
    FRESULT res;

    fp = (FIL *)file->private;

    res = f_truncate(fp);
    if (res != FR_OK) {
        return -1;
    }

    return 0;
}

static int fatfs_opendir(vfs_dir_t *dir, const char *pathname)
{
    DIR *dp = NULL;
    FATFS *fatfs = NULL;
    FRESULT res;

    dp = kmalloc(sizeof(DIR));
    if (!dp) {
        return -1;
    }

    dir->private = dp;
    fatfs = (FATFS *)dir->inode->private;

    res = f_opendir(fatfs, dp, pathname);
    if (res != FR_OK) {
        dir->private = NULL;
        kfree(dp);
        return -1;
    }

    return 0;
}


static int fatfs_closedir(vfs_dir_t *dir)
{
    DIR *dp = NULL;
    FRESULT res;

    dp = (DIR *)dir->private;

    res = f_closedir(dp);
    if (res != FR_OK) {
        return -1;
    }

    kfree(dp);
    dir->private = NULL;

    return 0;
}

static void fatfs_translate_filinfo2dirent(FILINFO *info, vfs_dirent_t *dirent)
{
    switch (info->fattrib) {
        case AM_DIR:
            dirent->type = VFS_TYPE_DIRECTORY;
            break;

        case AM_ARC:
            dirent->type = VFS_TYPE_FILE;
            break;

        default:
            dirent->type = VFS_TYPE_OTHER;
            break;
    }

    dirent->size = info->fsize;
    dirent->date = info->fdate;
    dirent->time = info->ftime;
    strncpy(dirent->name, info->fname, VFS_PATH_MAX);
    dirent->name[VFS_PATH_MAX - 1] = '\0';
}

static int fatfs_readdir(vfs_dir_t *dir, vfs_dirent_t *dirent)
{
    FILINFO info;
    DIR *dp = NULL;
    FRESULT res;

    dp = (DIR *)dir->private;
    res = f_readdir(dp, &info);
    if (res != FR_OK) {
        return -1;
    }

    fatfs_translate_filinfo2dirent(&info, dirent);

    return 0;
}

static int fatfs_unlink(vfs_inode_t *fs, const char *pathname)
{
    FATFS *fatfs = NULL;
    FRESULT res;

    fatfs = (FATFS *)fs->private;

    res = f_unlink(fatfs, pathname);
    if (res != FR_OK) {
        return -1;
    }
    return 0;
}

static int fatfs_mkdir(vfs_inode_t *fs, const char *pathname)
{
    FATFS *fatfs = NULL;
    FRESULT res;

    fatfs = (FATFS *)fs->private;

    res = f_mkdir(fatfs, pathname);
    if (res != FR_OK) {
        return -1;
    }
    return 0;
}

static int fatfs_rename(vfs_inode_t *fs, const char *oldpath, const char *newpath)
{
    FATFS *fatfs = NULL;
    FRESULT res;

    fatfs = (FATFS *)fs->private;

    res = f_rename(fatfs, oldpath, newpath);
    if (res != FR_OK) {
        return -1;
    }
    return 0; 
}

static unsigned int month_day[12]      = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //平年
static unsigned int Leap_month_day[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //闰年
static int isLeapYear(unsigned int year)
{
    if((year%4 == 0 && year % 100 != 0) || year % 400 == 0)
        return 1;
    return 0;
}
static unsigned int covDate2UnixTimeStp(unsigned int year, unsigned int month, unsigned int day)
{
    unsigned int daynum=0, SecNum=0;
    unsigned int tempYear=1970, tempMonth=0;
    //1.年的天数
    while(tempYear < year)
    {
        if(isLeapYear(tempYear)){
            daynum += 366;
        }
        else{
            daynum += 365;
        }
        tempYear++;
    }
        //2.月的天数
    while(tempMonth+1 < month)
    {
        if(isLeapYear(year)){ //闰年
            daynum += Leap_month_day[tempMonth];
        }
        else{
            daynum += month_day[tempMonth];
        }
        tempMonth++;
    }
    //3.天数
    daynum += (day-1);
    //4.时分秒
    SecNum = daynum*24*60*60;
    return SecNum;
}

unsigned int fatdate2unixtime(WORD date)
{
    unsigned short d = date;
    unsigned int year = (unsigned int)((d >> 9) & 0x7f) + 1980; // since 1980
    unsigned int month = (unsigned int)((d >> 5) & 0xf);
    unsigned int day = (unsigned int)((d) & 0x1f);
    return covDate2UnixTimeStp(year, month, day);
}

unsigned int fattime2unixtime(WORD time)
{
    unsigned short t = time;
    unsigned int hour = (unsigned int)((t >> 11) & 0x1f);
    unsigned int min = (unsigned int)((t >> 5) & 0x3f);
    unsigned int sec = (unsigned int)((t) & 0x1f) * 2;
    return hour * 60 * 60 + min * 60 + sec;
}

static void fatfs_translate_filinfo2fstat(FILINFO *info, vfs_fstat_t *buf)
{
    buf->mode = 0;
    switch (info->fattrib) {
        case AM_DIR:
            buf->type = VFS_TYPE_DIRECTORY;
            break;
    
        case AM_ARC:
            buf->type = VFS_TYPE_FILE;
            break;
    
        default:
            buf->type = VFS_TYPE_OTHER;
            break;
    }
    buf->mode = VFS_MODE_IRWXU | VFS_MODE_IRWXG | VFS_MODE_IRWXO | ((info->fattrib & AM_DIR) ? VFS_MODE_IFDIR : VFS_MODE_IFREG);
    buf->size   = info->fsize;
    buf->atime  = fatdate2unixtime(info->adate);
    buf->mtime  = fatdate2unixtime(info->fdate) + fattime2unixtime(info->ftime);
    buf->ctime  = fatdate2unixtime(info->cdate) + fattime2unixtime(info->ctime);
    unsigned int sec = (info->csec);
    sec *= 100;
    if(sec >= 500) buf->ctime++;
}

static int fatfs_stat(vfs_inode_t *fs, const char *pathname, vfs_fstat_t *buf)
{
    FATFS *fatfs = NULL;
    FILINFO info;
    FRESULT res;

    fatfs = (FATFS *)fs->private;

    res = f_stat(fatfs, pathname, &info);
    if (res != FR_OK) {
        return -1;
    }

    fatfs_translate_filinfo2fstat(&info, buf);

    return 0;
}

static int fatfs_bind(vfs_inode_t *fs, vfs_inode_t *dev)
{
    FATFS *fatfs = NULL;
    FRESULT res;

    fatfs = kmalloc(sizeof(FATFS));
    if (!fatfs) {
        return -1;
    }

    fatfs->pdrv = dev;
    fs->private = (void *)fatfs;

    res = f_mount(fatfs, 1);
    if (res != FR_OK) {
        fs->private = NULL;
        kfree(fatfs);
        return -1;
    }

    return 0;
}

static char workbuf[4096];

static int fatfs_mkfs(vfs_inode_t *dev, int opt, unsigned long arg)
{
    FRESULT res;

    res = f_mkfs(dev, opt, arg, workbuf, sizeof(workbuf));
    if (res != FR_OK) {
        return -1;
    }

    return 0;
}

vfs_fs_ops_t fatfs_ops = {
    .open       = fatfs_open,
    .close      = fatfs_close,
    .read       = fatfs_read,
    .write      = fatfs_write,

    .lseek      = fatfs_lseek,
    .truncate   = fatfs_truncate,
    .sync       = fatfs_sync,
    .opendir    = fatfs_opendir,
    .closedir   = fatfs_closedir,
    .readdir    = fatfs_readdir,
    .mkdir      = fatfs_mkdir,
    .unlink     = fatfs_unlink,
    .rename     = fatfs_rename,
    .stat       = fatfs_stat,

    .bind       = fatfs_bind,
    .mkfs       = fatfs_mkfs,
};

