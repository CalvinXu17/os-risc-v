#include "fat32.h"
#include "disk.h"
#include "sdcard_test.h"
#include "printk.h"
#include "string.h"

struct disk dsk;
struct fat_dpt dpt;
struct fat_dbr dbr;
struct fat_fsinfo fsi;
struct fat fat32;
struct fat_file root_dir;

char to_upper(char c)
{
    if(c >= 'a' && c <= 'z')
        c -= 32;
    return c;
}
char to_lower(char c)
{
    if(c >= 'A' && c <= 'Z')
        c += 32;
    return c;
}

void to_upper_s(char *s)
{
    while (*s != '\0')
    {
        *s = to_upper(*s);
        s++;
    }
}

void to_lower_s(char *s)
{
    while (*s != '\0')
    {
        *s = to_lower(*s);
        s++;
    }
}

#define print_p(param) \
{ \
    printk(#param ": %ld\n", fat32->param); \
}
void print_fat32_params(struct fat *fat32)
{
    printk("=======================================\n");
    printk("FAT32 PARAMS:\n");
    printk("name: %s\n", fat32->name);
    print_p(bpb_sector);
    print_p(total_sectors);
    print_p(bytes_per_sector);
    print_p(sectors_per_cluster);
    print_p(total_size);

    print_p(fat_nr);
    print_p(fat_sectors);
    print_p(fat_start_sector);

    print_p(root_cluster);
    print_p(root_sector);

    print_p(fsi_sector);
    print_p(backup_sector);
    print_p(cluster_next_free);
    print_p(cluster_total_free);
    printk("=======================================\n");
}
void init_fat32_params(struct fat *fat32, struct fat_dpt *dpt, struct fat_dbr *dbr, struct fat_fsinfo *fsi)
{
    fat32->bpb_sector = dpt->start_lba;
    fat32->total_sectors = dbr->bpb.TotalSec32;
    fat32->bytes_per_sector = dbr->bpb.BytesPerSec;
    fat32->sectors_per_cluster = dbr->bpb.SecPerClus;
    fat32->total_size = (uint64)fat32->bytes_per_sector * (uint64)fat32->total_sectors;

    fat32->fat_nr = dbr->bpb.FATs_nr;
    fat32->fat_sectors = dbr->hdr.FATSize32;
    fat32->fat_start_sector = fat32->bpb_sector + dbr->bpb.RsvdSecCnt;
    
    fat32->root_cluster = dbr->hdr.RootClus;
    fat32->root_sector = fat32->fat_start_sector + fat32->fat_sectors * fat32->fat_nr;

    fat32->fsi_sector = fat32->bpb_sector + dbr->hdr.FsInfo;
    fat32->backup_sector = fat32->bpb_sector + dbr->hdr.BkBootSec;
    fat32->cluster_next_free = fsi->FSI_Next_Free;
    fat32->cluster_total_free = fsi->FSI_Free_Count;
    int i;
    for(i=0; i < 11; i++)
    {
        fat32->name[i] = dbr->hdr.VolLab[i];
    }
    fat32->name[i] = '\0';
}

void init_root_dir(struct fat *fat32, struct fat_file *root)
{
    root->fat32 = fat32;
    root->finfo.type = FAT_DIR;
    root->finfo.file_name[0] = 'r';
    root->finfo.file_name[1] = 'o';
    root->finfo.file_name[2] = 'o';
    root->finfo.file_name[3] = 't';
    root->finfo.file_name[4] = '\0';
    root->start_cluster = fat32->root_cluster;
    root->cur_cluster = fat32->root_cluster;
    root->cur_sector_offset = 0;
    root->cur_byte_offset = 0;
    root->cur_from_file_start = 0;
    root->dir_cluster = 0;
    root->dir_cluster_offset = 0;
}

int fat32_init(struct disk *dsk)
{
    dsk->read(dsk, fat32.buf, 0, 1);
    
    if(!(fat32.buf[510]==0x55 && fat32.buf[511] == 0xaa))
        return 0;
    struct fat_dpt *dpt_p = &(fat32.buf[DPT_OFFSET]);
    if(!(dpt_p->system_id==FAT_NOT_VALID || dpt_p->system_id==FAT32 || dpt_p->system_id==WIN95_FAT32_0 || dpt_p->system_id==WIN95_FAT32_1))
        return 0;
    dpt = *dpt_p;
    dsk->read(dsk, fat32.buf, dpt.start_lba, 1);
    struct fat_dbr *dbr_p = (struct fat_dbr *)fat32.buf;
    dbr = *dbr_p;

    int fsi_sec = dbr.hdr.FsInfo + dpt.start_lba;
    dsk->read(dsk, fat32.buf, fsi_sec, 1);
    struct fat_fsinfo *fsi_p = (struct fat_fsinfo *)fat32.buf;
    fsi = *fsi_p;

    init_fat32_params(&fat32, &dpt, &dbr, &fsi);
    fat32.dsk = dsk;
    init_root_dir(&fat32, &root_dir);
    #ifdef _DEBUG
    print_fat32_params(&fat32);
    #endif
    return 1;
}

int get_sector_by_cluster(struct fat *fat32, int cluster)
{
    return (cluster - fat32->root_cluster) * fat32->sectors_per_cluster + fat32->root_sector;
}

// need to add lock
int get_fat(struct fat *fat32, int index)
{
    if(index < fat32->root_cluster || index >= (fat32->fat_sectors * fat32->bytes_per_sector / 4))
    {
        #ifdef _DEBUG
        printk("invalid fat index\n");
        #endif
        return CLUSTER_INVALID;
    }
    
    int items_per_sector = fat32->bytes_per_sector / 4;
    int sectors = index / items_per_sector;
    index = index % items_per_sector;
    int cur_sector = fat32->fat_start_sector + sectors;
    if(!fat32->dsk->read(fat32->dsk, fat32->buf, cur_sector, 1)) // read sector
    {
        #ifdef _DEBUG
        printk("read fat failed\n");
        #endif
        return CLUSTER_INVALID;
    }
        
    uint32 *table = (uint32 *)fat32->buf;
    return table[index];
}

int set_fat(struct fat *fat32, int index, int value)
{
    if(index < fat32->root_cluster || index >= (fat32->fat_sectors * fat32->bytes_per_sector / 4)) return 0;
    int items_per_sector = fat32->bytes_per_sector / 4;
    int sectors = index / items_per_sector;
    index = index % items_per_sector;
    int cur_sector = fat32->fat_start_sector + sectors;
    if(!fat32->dsk->read(fat32->dsk, fat32->buf, cur_sector, 1)) // read sector
        return 0;
    uint32 *table = (uint32 *)fat32->buf;
    table[index] = value;
    if(!fat32->dsk->write(fat32->dsk, fat32->buf, cur_sector, 1))
        return 0;
    return 1;
}

void update_file_pos(struct fat_file *file, int bytes)
{
    if(bytes > file->finfo.size - file->cur_from_file_start)
    {
        bytes = file->finfo.size - file->cur_from_file_start;
    }
    file->cur_from_file_start += bytes;

    if(file->cur_from_file_start == file->finfo.size) // 文件已读完，pos重置为起始
    {
        file->cur_cluster = file->start_cluster;
        file->cur_sector_offset = 0;
        file->cur_byte_offset = 0;
    }
    else // 还文件未读完
    {
        file->cur_byte_offset += bytes;

        if(file->cur_byte_offset >= file->fat32->bytes_per_sector) // 若当前扇区也刚好读完
        {
            int s = file->cur_byte_offset / file->fat32->bytes_per_sector;
            file->cur_byte_offset %= file->fat32->bytes_per_sector;
            file->cur_sector_offset += s;
            if(file->cur_sector_offset >= file->fat32->sectors_per_cluster)
            {
                int c = file->cur_sector_offset / file->fat32->sectors_per_cluster;
                file->cur_sector_offset %= file->fat32->sectors_per_cluster;
                while(c--)
                {
                    file->cur_cluster = get_fat(file->fat32, file->cur_cluster); // 更新cluster
                }
            }
        }
    }
}

fstatus update_file_diritem(struct fat_file *file, int isdelete)
{
    int cluster = file->dir_cluster;
    int cur_sector = get_sector_by_cluster(file->fat32, cluster);
    int sectors = file->dir_cluster_offset / file->fat32->bytes_per_sector;
    int bytes = file->dir_cluster_offset % file->fat32->bytes_per_sector;

    cur_sector += sectors;
    if(!file->fat32->dsk->read(file->fat32->dsk, file->fat32->buf, cur_sector, 1))
        return FSTATUS_ERROR;
    struct fat_diritem *item = (struct fat_diritem *)(file->fat32->buf + bytes);
}

fstatus search_file_in_dir(struct fat_file *dir, struct fat_file *file, const char *name, int len)
{
    if(dir->finfo.type != FAT_DIR) return FSTATUS_NOT_DIR;
    if(len > 12) return FSTATUS_PATH_ERR; // 这里只支持短文件名，暂时未适配长文件名目录项
    int dot, i, j;
    for(i = 0; i < len; i++)
        if(!((name[i]>='a'&&name[i]<='z') || (name[i]>='A'&&name[i]<='Z') || name[i]=='.'))
            return FSTATUS_PATH_ERR;
    for(dot = len-1; dot >= 0; dot--)
        if(name[dot] == '.') break;

    if(dot > 0)
    {
        if(dot > 8) return FSTATUS_PATH_ERR;
        if(len - dot - 1 > 3) return FSTATUS_PATH_ERR;
    } else if(len > 8)
        return FSTATUS_PATH_ERR;
    char fname[9] = {0};
    char fext[4] = {0};
    char dname[9] = {0};
    char dext[4] = {0};
    int lenname = dot > 0 ? dot : len;
    int lenext = dot > 0 ? len - lenname - 1: 0;
    
    memcpy(fname, name, lenname);
    memcpy(fext, name+lenname+1, lenext);
    to_lower_s(fname);
    to_lower_s(fext); // 文件类型不区分大小写
    
    int start_cluster = dir->start_cluster;
    do {
        int cur_sector = get_sector_by_cluster(dir->fat32, start_cluster);
        int sectors = dir->fat32->sectors_per_cluster;
        int i, j, k;
        for(i = 0; i < sectors; i++)
        {
            if(!dir->fat32->dsk->read(dir->fat32->dsk, dir->fat32->buf, cur_sector+i, 1))
                return FSTATUS_ERROR;
            struct fat_diritem *dir_items = (struct fat_diritem *)dir->fat32->buf;
            for(j = 0; j < dir->fat32->bytes_per_sector / sizeof(struct fat_diritem); j++)
            {
                struct fat_diritem item = dir_items[j];
                if(item.DIR_Name[0] != 0xE5 && item.DIR_Attr != DIRITEM_ATTR_LONG_NAME && ((item.DIR_Attr & DIRITEM_ATTR_DIRECTORY) || (item.DIR_Attr & DIRITEM_ATTR_ARCHIVE)))
                {
                    if(item.DIR_Name[0] == 0x05) item.DIR_Name[0]=0xE5;

                    memcpy(dname, item.DIR_Name, 8);
                    memcpy(dext, item.DIR_ExtName, 3);
                    for(k=0;k<8;k++) if(dname[k]==0x20) dname[k]='\0';
                    for(k=0;k<3;k++) if(dext[k]==0x20) dext[k]='\0';
                    to_lower_s(dname);
                    to_lower_s(dext);
                    #ifdef _DEBUG
                    printk("is file or dir:%s:%s:%s:%s\n", dname, dext, fname, fext);
                    #endif
                    if(strlen(fname) == strlen(dname) && strlen(fext)==strlen(dext))
                    {
                        if(strcmp(fname, dname)==0 && strcmp(fext, dext)==0)
                        {
                            file->fat32 = dir->fat32;
                            strcpy(file->finfo.file_name, fname);
                            file->finfo.size = item.DIR_FileSize;
                            file->finfo.attr = item.DIR_Attr;
                            if(item.DIR_Attr & DIRITEM_ATTR_DIRECTORY)
                                file->finfo.type = FAT_DIR;
                            else file->finfo.type = FAT_FILE;

                            file->finfo.create_time.year = (uint16)item.DIR_CrtDate.year_from_1980+1980;
                            file->finfo.create_time.month = (uchar)item.DIR_CrtDate.month;
                            file->finfo.create_time.day = (uchar)item.DIR_CrtDate.day;
                            file->finfo.create_time.hour = (uchar)item.DIR_CrtTime.hour;
                            file->finfo.create_time.minute = (uchar)item.DIR_CrtTime.minute;
                            file->finfo.create_time.second = (uchar)item.DIR_CrtTime.second_2 * 2;

                            file->finfo.last_acctime.year = (uint16)item.DIR_LastAccDate.year_from_1980+1980;
                            file->finfo.last_acctime.month = (uchar)item.DIR_LastAccDate.month;
                            file->finfo.last_acctime.day = (uchar)item.DIR_LastAccDate.day;
                            file->finfo.last_acctime.hour = 0;
                            file->finfo.last_acctime.minute = 0;
                            file->finfo.last_acctime.second = 0;

                            file->finfo.modify_time.year = (uint16)item.DIR_WrtDate.year_from_1980+1980;
                            file->finfo.modify_time.month = (uchar)item.DIR_WrtDate.month;
                            file->finfo.modify_time.day = (uchar)item.DIR_WrtDate.day;
                            file->finfo.modify_time.hour = (uchar)item.DIR_WrtTime.hour;
                            file->finfo.modify_time.minute = (uchar)item.DIR_WrtTime.minute;
                            file->finfo.modify_time.second = (uchar)item.DIR_WrtTime.second_2 * 2;


                            file->start_cluster = (((uint32)item.DIR_ClusterH) << 16) | ((uint32)item.DIR_ClusterL);
                            file->cur_cluster = file->start_cluster;
                            file->cur_sector_offset = 0;
                            file->cur_byte_offset = 0;
                            file->cur_from_file_start = 0;
                            file->dir_cluster = start_cluster; // 文件目录项所在的簇号
                            file->dir_cluster_offset = i * dir->fat32->bytes_per_sector + sizeof(struct fat_diritem) * j; // 簇内偏移
                            return FSTATUS_OK;
                        }
                    }
                }
            }
        }
    } while((start_cluster=get_fat(dir->fat32, start_cluster)) != CLUSTER_INVALID);

    return FSTATUS_NOT_FILE;
}
fstatus fat_open_file(struct fat *fat32, struct fat_file *file, const char *path, int len)
{
    return FSTATUS_OK;
}

fstatus fat_close_file(struct fat_file *file)
{
    return FSTATUS_OK;
}

fstatus fat_read_file(struct fat_file *file, uchar *buf, int cnt, int *ret)
{
    if(file->finfo.type != FAT_FILE)
    {
        *ret = 0;
        return FSTATUS_NOT_FILE;
    }
    if(file->cur_from_file_start >= file->finfo.size)
    {
        *ret = -1;
        return FSTATUS_EOF;
    }
    int oldcluster = file->cur_cluster;
    int oldsector = file->cur_sector_offset;
    int oldbyte = file->cur_byte_offset;
    int oldfile = file->cur_from_file_start;

    int remain = file->finfo.size - file->cur_from_file_start;
    if(cnt > remain) cnt = remain;

    int cur_sector = get_sector_by_cluster(file->fat32, file->cur_cluster) + file->cur_sector_offset;
    if(!file->fat32->dsk->read(file->fat32->dsk, file->fat32->buf, cur_sector, 1)) // read current sector
    {
        *ret = 0;
        return FSTATUS_ERROR;
    }
    int i, j, b;
    for(b=file->cur_byte_offset, i=0; i < cnt && b < file->fat32->bytes_per_sector; i++, b++)
        buf[i] = file->fat32->buf[b];

    update_file_pos(file, i); // 更新当前位置信息

    if(i==cnt) // 一扇区内刚好读完所需的数据直接返回
    {
        *ret = cnt;
        return FSTATUS_OK;
    }

    // 到这里说明未读完，则继续读取n个扇区
    remain = cnt - i;
    int sectors = remain / file->fat32->bytes_per_sector;
    int bytes = remain % file->fat32->bytes_per_sector;

    for(j = 0; j < sectors; j++)
    {
        cur_sector = get_sector_by_cluster(file->fat32, file->cur_cluster) + file->cur_sector_offset;
        if(!file->fat32->dsk->read(file->fat32->dsk, &buf[i], cur_sector, 1))
        {
            file->cur_cluster = oldcluster;
            file->cur_sector_offset = oldsector;
            file->cur_byte_offset = oldbyte;
            file->cur_from_file_start = oldfile;
            *ret = 0;
            return FSTATUS_ERROR;
        }
        i += file->fat32->bytes_per_sector;
        update_file_pos(file, file->fat32->bytes_per_sector);
    }
    cur_sector = get_sector_by_cluster(file->fat32, file->cur_cluster) + file->cur_sector_offset;
    if(!file->fat32->dsk->read(file->fat32->dsk, file->fat32->buf, cur_sector, 1)) // read current sector
    {
        file->cur_cluster = oldcluster;
        file->cur_sector_offset = oldsector;
        file->cur_byte_offset = oldbyte;
        file->cur_from_file_start = oldfile;
        *ret = 0;
        return FSTATUS_ERROR;
    }
    for(b=file->cur_byte_offset; i < cnt && b < bytes; i++, b++)
        buf[i] = file->fat32->buf[b];
    #ifdef _DEBUG
    if(i!=cnt || b != bytes)
        printk("sdcard read error: bytes not equal!\n");
    #endif
    update_file_pos(file, bytes);
    *ret = cnt;
    return FSTATUS_OK;

    // if(sectors <= (file->fat32->sectors_per_cluster - file->cur_sector_offset))
    // {
    //     cur_sector = get_sector_by_cluster(file->fat32, file->cur_cluster) + file->cur_sector_offset;
    //     if(!file->fat32->dsk->read(file->fat32->dsk, &buf[i], cur_sector, sectors))
    //     {
    //         file->cur_cluster = oldcluster;
    //         file->cur_sector_offset = oldsector;
    //         file->cur_byte_offset = oldbyte;
    //         file->cur_from_file_start = oldfile;
    //         *ret = 0;
    //         return FSTATUS_ERROR;
    //     }
    //     i += sectors * file->fat32->bytes_per_sector;
    //     update_file_pos(file, sectors * file->fat32->bytes_per_sector);
    // } else {
    //     cur_sector = get_sector_by_cluster(file->fat32, file->cur_cluster) + file->cur_sector_offset;
    //     if(!file->fat32->dsk->read(file->fat32->dsk, &buf[i], cur_sector, (file->fat32->sectors_per_cluster - file->cur_sector_offset)))
    //     {
    //         file->cur_cluster = oldcluster;
    //         file->cur_sector_offset = oldsector;
    //         file->cur_byte_offset = oldbyte;
    //         file->cur_from_file_start = oldfile;
    //         *ret = 0;
    //         return FSTATUS_ERROR;
    //     }
    //     i += (file->fat32->sectors_per_cluster - file->cur_sector_offset) * file->fat32->bytes_per_sector;
    //     update_file_pos(file, (file->fat32->sectors_per_cluster - file->cur_sector_offset) * file->fat32->bytes_per_sector);
    //     sectors -= (file->fat32->sectors_per_cluster - file->cur_sector_offset);

    //     int clusters = sectors / file->fat32->sectors_per_cluster;
    //     sectors = sectors % file->fat32->sectors_per_cluster;
    //     int c;
    //     int ssize = file->fat32->sectors_per_cluster * file->fat32->bytes_per_sector;
    //     for(c = 0; c < clusters; c++)
    //     {
    //         cur_sector = get_sector_by_cluster(file->fat32, file->cur_cluster) + file->cur_sector_offset;
    //         if(!file->fat32->dsk->read(file->fat32->dsk, &buf[i], cur_sector, file->fat32->sectors_per_cluster))
    //         {
    //             file->cur_cluster = oldcluster;
    //         file->cur_sector_offset = oldsector;
    //         file->cur_byte_offset = oldbyte;
    //         file->cur_from_file_start = oldfile;
    //         *ret = 0;
    //             return FSTATUS_ERROR;
    //         }
    //         update_file_pos(file, ssize);
    //         i += ssize;
    //     }
    // }
    // cur_sector = get_sector_by_cluster(file->fat32, file->cur_cluster) + file->cur_sector_offset;
    // if(!file->fat32->dsk->read(file->fat32->dsk, file->buf, cur_sector, 1)) // read current sector
    // {
    //     file->cur_cluster = oldcluster;
    //     file->cur_sector_offset = oldsector;
    //     file->cur_byte_offset = oldbyte;
    //     file->cur_from_file_start = oldfile;
    //     *ret = 0;
    //     return FSTATUS_ERROR;
    // }
    // for(b=file->cur_byte_offset; i < cnt && b < bytes; i++, b++)
    //     buf[i] = file->buf[b];
    // update_file_pos(file, bytes);
    // *ret = cnt;
    // return FSTATUS_OK;
}

fstatus fat_write_file(struct fat_file *file, uchar *buf, int cnt, int flags)
{
    // if(file->finfo.type != FAT_FILE)
    // {
    //     return FSTATUS_NOT_FILE;
    // }

    // int oldcluster = file->cur_cluster;
    // int oldsector = file->cur_sector_offset;
    // int oldbyte = file->cur_byte_offset;
    // int oldfile = file->cur_from_file_start;

    // int cur_sector = get_sector_by_cluster(file->fat32, file->cur_cluster) + file->cur_sector_offset;
    
    // if(!file->fat32->dsk->read(file->fat32->dsk, file->fat32->buf, cur_sector, 1)) // read current sector
    // {
    //     return FSTATUS_ERROR;
    // }

    // int i, j, b;
    // for(b=file->cur_byte_offset, i=0; i < cnt && b < file->fat32->bytes_per_sector; i++, b++)
    //     file->fat32->buf[b] = buf[i];
    
    // if(!file->fat32->dsk->write(file->fat32->dsk, file->fat32->buf, cur_sector, 1)) // write current sector
    // {
    //     file->cur_cluster = oldcluster;
    //     file->cur_sector_offset = oldsector;
    //     file->cur_byte_offset = oldbyte;
    //     file->cur_from_file_start = oldfile;
    //     return FSTATUS_ERROR;
    // }

    // update_file_pos(file, i); // 更新当前位置信息
    // if(file->cur_byte_offset > file->finfo.size)
    // {
    //     file->finfo.size = file->cur_byte_offset; // 若文件从指定位置开始写入后增加了源文件的数据大小，则更新finfo
    // }

    // if(i==cnt) // 一扇区内刚好读完所需的数据直接返回
    // {
    //     *ret = cnt;
    //     return FSTATUS_OK;
    // }

    // // 到这里说明未读完，则继续读取n个扇区
    // remain = cnt - i;
    // int sectors = remain / file->fat32->bytes_per_sector;
    // int bytes = remain % file->fat32->bytes_per_sector;

    // for(j = 0; j < sectors; j++)
    // {
    //     cur_sector = get_sector_by_cluster(file->fat32, file->cur_cluster) + file->cur_sector_offset;
    //     if(!file->fat32->dsk->read(file->fat32->dsk, &buf[i], cur_sector, 1))
    //     {
    //         file->cur_cluster = oldcluster;
    //         file->cur_sector_offset = oldsector;
    //         file->cur_byte_offset = oldbyte;
    //         file->cur_from_file_start = oldfile;
    //         *ret = 0;
    //         return FSTATUS_ERROR;
    //     }
    //     i += file->fat32->bytes_per_sector;
    //     update_file_pos(file, file->fat32->bytes_per_sector);
    // }
    // cur_sector = get_sector_by_cluster(file->fat32, file->cur_cluster) + file->cur_sector_offset;
    // if(!file->fat32->dsk->read(file->fat32->dsk, file->fat32->buf, cur_sector, 1)) // read current sector
    // {
    //     file->cur_cluster = oldcluster;
    //     file->cur_sector_offset = oldsector;
    //     file->cur_byte_offset = oldbyte;
    //     file->cur_from_file_start = oldfile;
    //     *ret = 0;
    //     return FSTATUS_ERROR;
    // }
    // for(b=file->cur_byte_offset; i < cnt && b < bytes; i++, b++)
    //     buf[i] = file->fat32->buf[b];
    // if(i!=cnt || b != bytes)
    //     printk("sdcard read error: bytes not equal!\n");
    // update_file_pos(file, bytes);
    // *ret = cnt;
    return FSTATUS_OK;
}