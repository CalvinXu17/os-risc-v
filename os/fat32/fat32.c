#include "fat32.h"
#include "disk.h"
#include "sdcard_test.h"
#include "printk.h"

struct disk dsk;

#define print_p(param) \
{ \
    printk(#param ": %ld\n", fat32->param); \
}
void print_fat32_params(struct fat *fat32)
{
    printk("=======================================\n");
    printk("FAT32 PARAMS:\n");
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
}

uchar buf[512];
struct fat_dpt dpt;
struct fat_dbr dbr;
struct fat_fsinfo fsi;
struct fat fat32;

int fat32_init(struct disk *dsk)
{
    dsk->read(dsk, buf, 0, 1);
    
    if(!(buf[510]==0x55 && buf[511] == 0xaa))
        return 0;
    struct fat_dpt *dpt_p = &(buf[DPT_OFFSET]);
    if(!(dpt_p->system_id==FAT_NOT_VALID || dpt_p->system_id==FAT32 || dpt_p->system_id==WIN95_FAT32_0 || dpt_p->system_id==WIN95_FAT32_1))
        return 0;
    dpt = *dpt_p;
    dsk->read(dsk, buf, dpt.start_lba, 1);
    struct fat_dbr *dbr_p = (struct fat_dbr *)buf;
    dbr = *dbr_p;

    int fsi_sec = dbr.hdr.FsInfo + dpt.start_lba;
    dsk->read(dsk, buf, fsi_sec, 1);
    struct fat_fsinfo *fsi_p = (struct fat_fsinfo *)buf;
    fsi = *fsi_p;

    init_fat32_params(&fat32, &dpt, &dbr, &fsi);
    print_fat32_params(&fat32);
    return 1;
}
