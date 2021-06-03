#include "vdisk.h"
#include "mm.h"
#include "spinlock.h"
#include "panic.h"
#include "string.h"
#include "plic.h"

#define R(r) (volatile uint32 *)(VIRTIO0_V + (r))
#define PGSIZE  4096
#define PGSHIFT 12
#define BSIZE   512
static struct disk {
    // memory for virtio descriptors &c for queue 0.
    // this is a global instead of allocated because it must
    // be multiple contiguous pages, which kalloc()
    // doesn't support, and page aligned.
    char pages[2*PGSIZE];
    // the following three pointers will point to pages[]
    vring_desc_t *desc;
    vring_avail_t *avail;
    vring_used_t *used;
    // our own book-keeping.
    char free[NUM];  // is a descriptor free?
    char status[NUM];
    spinlock vdisk_lock;
} __attribute__((aligned (PGSIZE))) disk;

int vdisk_init(void)
{
    uint32 status = 0;

    init_spinlock(&disk.vdisk_lock, "virtio_disk");

    if (*R(VIRTIO_MMIO_MAGIC_VALUE_OFFSET) != 0x74726976 ||
        *R(VIRTIO_MMIO_VERSION_OFFSET) != 1 ||
        *R(VIRTIO_MMIO_DEVICE_ID_OFFSET) != 2 ||
        *R(VIRTIO_MMIO_VENDOR_ID_OFFSET) != 0x554d4551)
    {
        #ifdef _DEBUG
        printk("virtio blk info wrong!\n");
        #endif
        return 1;
    }
  
    status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
    *R(VIRTIO_MMIO_STATUS_OFFSET) = status;

    status |= VIRTIO_CONFIG_S_DRIVER;
    *R(VIRTIO_MMIO_STATUS_OFFSET) = status;

    // negotiate features
    uint64 features = *R(VIRTIO_MMIO_DEVICE_FEATURES_OFFSET);
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    *R(VIRTIO_MMIO_DRIVER_FEATURES_OFFSET) = features;

    // tell device that feature negotiation is complete.
    status |= VIRTIO_CONFIG_S_FEATURES_OK;
    *R(VIRTIO_MMIO_STATUS_OFFSET) = status;

    // tell device we're completely ready.
    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    *R(VIRTIO_MMIO_STATUS_OFFSET) = status;

    *R(VIRTIO_MMIO_GUEST_PAGE_SIZE_OFFSET) = PGSIZE;

    // initialize queue 0.
    *R(VIRTIO_MMIO_QUEUE_SEL_OFFSET) = 0;
    uint32 max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX_OFFSET);
    if(max == 0)
        return 1;
    if(max < NUM)
        return 1;
    *R(VIRTIO_MMIO_QUEUE_NUM_OFFSET) = NUM;
    memset(disk.pages, 0, sizeof(disk.pages));
    *R(VIRTIO_MMIO_QUEUE_PFN_OFFSET) = (((uint64)disk.pages) - PV_OFFSET) >> PGSHIFT;

    // desc = pages -- num * VRingDesc
    // avail = pages + 0x40 -- 2 * uint16, then num * uint16
    // used = pages + 4096 -- 2 * uint16, then num * vRingUsedElem
    disk.desc = (vring_desc_t *) disk.pages;
    disk.avail = (uint16*)(((char*)disk.desc) + NUM*sizeof(vring_desc_t));
    disk.used = (vring_used_t *) (disk.pages + PGSIZE);
    for(int i = 0; i < NUM; i++)
        disk.free[i] = 1;
    #ifdef _DEBUG
    printk("vdisk init success\n");
    #endif
    return 0;
}

// find a free descriptor, mark it non-free, return its index.
static int alloc_desc()
{
    for(int i = 0; i < NUM; i++)
    {
        if(disk.free[i])
        {
            disk.free[i] = 0;
            return i;
        }
    }
    return -1;
}

// mark a descriptor as free.
static void free_desc(int i)
{
    if(i >= NUM)
        panic("virtio_disk_intr 1");
    if(disk.free[i])
        panic("virtio_disk_intr 2");
    disk.desc[i].addr = 0;
    disk.free[i] = 1;
    // wakeup(&disk.free[0]);
}

// free a chain of descriptors.
static void free_chain(int i)
{
    while(1)
    {
        free_desc(i);
        if(disk.desc[i].flags & VRING_DESC_F_NEXT)
            i = disk.desc[i].next;
        else
            break;
    }
}

static int alloc3_desc(int *idx)
{
    for(int i = 0; i < 3; i++)
    {
        idx[i] = alloc_desc();
        if(idx[i] < 0)
        {
            for(int j = 0; j < i; j++)
            free_desc(idx[j]);
            return -1;
        }
    }
    return 0;
}

int vdisk_read(uchar *buf, uint64 start_sector, uint64 count)
{
    lock(&disk.vdisk_lock);

    // the spec says that legacy block operations use three
    // descriptors: one for type/reserved/sector, one for
    // the data, one for a 1-byte status result.

    // allocate the three descriptors.
    int idx[3];
    while(1)
    {
        if(alloc3_desc(idx) == 0)
            break;
    }
  
    // format the three descriptors.
    // qemu's virtio-blk.c reads them.
    virtio_blk_outhdr_t buf0;
    buf0.type = VIRTIO_BLK_T_IN; // read the disk
    buf0.reserved = 0;
    buf0.sector = start_sector;

    // buf0 is on a kernel stack, which is not direct mapped,
    // thus the call to kvmpa().
    disk.desc[idx[0]].addr = (uint64)&buf0 - PV_OFFSET;
    disk.desc[idx[0]].len = sizeof(struct virtio_blk_outhdr);
    disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
    disk.desc[idx[0]].next = idx[1];

    disk.desc[idx[1]].addr = (uint64)buf - PV_OFFSET;
    disk.desc[idx[1]].len = BSIZE;
    disk.desc[idx[1]].flags = VRING_DESC_F_WRITE; // device writes b->data
    disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
    disk.desc[idx[1]].next = idx[2];

    disk.desc[idx[2]].addr = (uint64)&disk.status[idx[0]] - PV_OFFSET;
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = VRING_DESC_F_WRITE; // device writes the status
    disk.desc[idx[2]].next = 0;

    // record struct buf for virtio_disk_intr().
    // b->disk = 1;

    // avail[0] is flags
    // avail[1] tells the device how far to look in avail[2...].
    // avail[2...] are desc[] indices the device should process.
    // we only tell device the first index in our chain of descriptors.
    disk.avail->elems[disk.avail->idx % NUM] = idx[0];
    __sync_synchronize();
    disk.avail->idx = disk.avail->idx + 1;

    disk.avail->flags = 1;
    disk.status[idx[0]] = 1; // 成功时该位会被virtio device置为0
    *R(VIRTIO_MMIO_QUEUE_NOTIFY_OFFSET) = 0; // value is queue number
    // Wait for virtio_disk_intr() to say request has finished.
    int rt = 0;
    while(1)
    {
        if(*R(VIRTIO_MMIO_INTERRUPT_STATUS_OFFSET))
        {
            *R(VIRTIO_MMIO_INTERRUPT_ACK_OFFSET) = *R(VIRTIO_MMIO_INTERRUPT_STATUS_OFFSET) & 0x3;
            if(disk.status[idx[0]] != 0) rt = 1;
            break;
        }
    }
    free_chain(idx[0]);
    unlock(&disk.vdisk_lock);
    return rt;
}

int vdisk_write(uchar *buf, uint64 start_sector, uint64 count)
{
    lock(&disk.vdisk_lock);

    // the spec says that legacy block operations use three
    // descriptors: one for type/reserved/sector, one for
    // the data, one for a 1-byte status result.

    // allocate the three descriptors.
    int idx[3];
    while(1)
    {
        if(alloc3_desc(idx) == 0)
            break;
        // sleep(&disk.free[0], &disk.vdisk_lock);
    }
  
    // format the three descriptors.
    // qemu's virtio-blk.c reads them.
    virtio_blk_outhdr_t buf0;
    buf0.type = VIRTIO_BLK_T_OUT; // write the disk
    buf0.reserved = 0;
    buf0.sector = start_sector;

    // buf0 is on a kernel stack, which is not direct mapped,
    // thus the call to kvmpa().
    disk.desc[idx[0]].addr = (uint64)&buf0 - PV_OFFSET;
    disk.desc[idx[0]].len = sizeof(struct virtio_blk_outhdr);
    disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
    disk.desc[idx[0]].next = idx[1];

    disk.desc[idx[1]].addr = (uint64)buf - PV_OFFSET;
    disk.desc[idx[1]].len = BSIZE;
    disk.desc[idx[1]].flags = 0; // device reads b->data
    disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
    disk.desc[idx[1]].next = idx[2];

    disk.desc[idx[2]].addr = (uint64)&disk.status[idx[0]] - PV_OFFSET;
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = VRING_DESC_F_WRITE; // device writes the status
    disk.desc[idx[2]].next = 0;

    // avail[0] is flags
    // avail[1] tells the device how far to look in avail[2...].
    // avail[2...] are desc[] indices the device should process.
    // we only tell device the first index in our chain of descriptors.
    disk.avail->elems[disk.avail->idx % NUM] = idx[0];
    __sync_synchronize();
    disk.avail->idx = disk.avail->idx + 1;
    
    disk.avail->flags = 1;
    disk.status[idx[0]] = 1;
    *R(VIRTIO_MMIO_QUEUE_NOTIFY_OFFSET) = 0; // value is queue number
    
    // Wait for virtio_disk_intr() to say request has finished.
    int rt = 0;
    while(1)
    {
        if(*R(VIRTIO_MMIO_INTERRUPT_STATUS_OFFSET))
        {
            *R(VIRTIO_MMIO_INTERRUPT_ACK_OFFSET) = *R(VIRTIO_MMIO_INTERRUPT_STATUS_OFFSET) & 0x3;
            if(disk.status[idx[0]] != 0) rt = 1;
            break;
        }
    }
    free_chain(idx[0]);
    unlock(&disk.vdisk_lock);
    return rt;
}

void vdisk_intr()
{
    lock(&disk.vdisk_lock);

    // while((disk.used_idx % NUM) != (disk.used->idx % NUM))
    // {
    //     int id = disk.used->elems[disk.used_idx].id;

    //     if(disk.info[id].status != 0)
    //         panic("virtio_disk_intr status");
        
    //     // disk.info[id].b->disk = 0;   // disk is done with buf
    //     // wakeup(disk.info[id].b);

    //     disk.used_idx = (disk.used_idx + 1) % NUM;
    // }
    *R(VIRTIO_MMIO_INTERRUPT_ACK_OFFSET) = *R(VIRTIO_MMIO_INTERRUPT_STATUS_OFFSET) & 0x3;
    unlock(&disk.vdisk_lock);
}

void vdisk_getinfo(hal_sd_info_t *info)
{
    virtio_blk_config_t *config = VIRTIO0_V + VIRTIO_MMIO_CONFIG_OFFSET;
    info->blk_size = config->blk_size;
    info->blk_num = config->capacity;
    info->logical_blk_size = info->blk_size;
    info->logical_blk_num = info->blk_num;
    info->relative_card_addr = 0;
}