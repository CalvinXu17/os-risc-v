#ifndef VDISK_H
#define VDISK_H

#include "type.h"
#include "hal_sd.h"

// https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.pdf

// virtio mmio control registers, mapped starting at 0x10001000.
// from qemu VIRTIO_MMIO.h

#define VIRTIO_MMIO_MAGIC_VALUE_OFFSET		        0x000 // 0x74726976
#define VIRTIO_MMIO_VERSION_OFFSET		            0x004 // legacy设备为1
#define VIRTIO_MMIO_DEVICE_ID_OFFSET		        0x008 // blk is 2, net is 1
#define VIRTIO_MMIO_VENDOR_ID_OFFSET		        0x00c // 0x554d4551
#define VIRTIO_MMIO_DEVICE_FEATURES_OFFSET	        0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL_OFFSET	    0x014
#define VIRTIO_MMIO_DRIVER_FEATURES_OFFSET	        0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL_OFFSET      0x024
#define VIRTIO_MMIO_GUEST_PAGE_SIZE_OFFSET	        0x028 // page size for PFN, write-only
#define VIRTIO_MMIO_QUEUE_SEL_OFFSET		        0x030 // write, 选择virtual queue that follow options on QueueNumMax, QueueNum, QueueReady, QueueDescLow,QueueDescHigh, QueueAvailLow, QueueAvailHigh, QueueUsedLow andQueueUsedHigh apply to. The index number of the first queue is zero (0x0).
#define VIRTIO_MMIO_QUEUE_NUM_MAX_OFFSET	        0x034 // read, max size of current queue
#define VIRTIO_MMIO_QUEUE_NUM_OFFSET		        0x038 // write, size of current queue
#define VIRTIO_MMIO_QUEUE_ALIGN_OFFSET		        0x03c // write, used ring alignment
#define VIRTIO_MMIO_QUEUE_PFN_OFFSET		        0x040 // read/write, physical page number for queue
#define VIRTIO_MMIO_QUEUE_READY_OFFSET		        0x044 // ready bit, Writing one (0x1) to this register notifies the device that it can execute requests from this virtual queu
#define VIRTIO_MMIO_QUEUE_NOTIFY_OFFSET	            0x050 // write
#define VIRTIO_MMIO_INTERRUPT_STATUS_OFFSET	        0x060 // read
#define VIRTIO_MMIO_INTERRUPT_ACK_OFFSET	        0x064 // write
#define VIRTIO_MMIO_STATUS_OFFSET		            0x070 // read/write
#define VIRTIO_MMIO_CONFIG_GENERATION_OFFSET		0x0fc // read
#define VIRTIO_MMIO_CONFIG_OFFSET		            0x100 // read/write

// status register bits, from qemu virtio_config.h
#define VIRTIO_CONFIG_S_ACKNOWLEDGE	                1
#define VIRTIO_CONFIG_S_DRIVER		                2
#define VIRTIO_CONFIG_S_DRIVER_OK	                4
#define VIRTIO_CONFIG_S_FEATURES_OK	                8

// device feature bits
#define VIRTIO_BLK_F_RO                             5	    // Disk is read-only
#define VIRTIO_BLK_F_SCSI                           7	    // Supports scsi command passthru
#define VIRTIO_BLK_F_CONFIG_WCE                     11	    // Writeback mode available in config
#define VIRTIO_BLK_F_MQ                             12      // support more than one vq
#define VIRTIO_F_ANY_LAYOUT                         27
#define VIRTIO_RING_F_INDIRECT_DESC                 28
#define VIRTIO_RING_F_EVENT_IDX                     29

// this many virtio descriptors.
// must be a power of two.
#define NUM   8

typedef struct vring_desc
{
    uint64 addr;    // 数据的物理地址
    uint32 len;     // 数据的长度
    uint16 flags;   // 标记数据对于Host是可读还是可写
    uint16 next;    // 存放下一个buffer在Descriptor Table的位置
} vring_desc_t;

#define VRING_DESC_F_NEXT       1 // 表示该buffer之后还有buffer，所有buffer可以通过next连成一个Descriptor chain
#define VRING_DESC_F_WRITE      2 // 表示该buffer只能写，当buffer用于接收数据时，需要向Host提供buffer，这个时候就标记buffer为写。反之是发送数据，标记为读
#define VRING_DESC_F_INDIRECT   4 // This means the buffer contains a list of buffer descriptors.

typedef struct vring_avail // Guest通过Avail Ring向Host提供buffer，指示Guest增加的buffer位置和当前工作的位置
{
    uint16 flags;   // 用于指示Host当它处理完buffer，将Descriptor index写入Used Ring之后，是否通过注入中断通知Guest
    uint16 idx;     // 指示Guest下一次添加buffer时的在Avail Ring所处的位置，换句话说，idx存放的ring[]数组索引，ring[idx]存放才是下一次添加的buffer头在Descriptor Table的位置
    uint16 elems[];
} vring_avail_t;


typedef struct vring_used_elem
{
    uint32 id;   // index of start of completed descriptor chain
    uint32 len;
} vring_used_elem_t;

// for disk ops
#define VIRTIO_BLK_T_IN  0 // read the disk
#define VIRTIO_BLK_T_OUT 1 // write the disk

typedef struct vring_used
{
  uint16 flags;
  uint16 idx;
  vring_used_elem_t elems[NUM];
} vring_used_t;

typedef struct virtio_blk_outhdr {
        uint32 type;
        uint32 reserved;
        uint64 sector;
} virtio_blk_outhdr_t;

typedef struct vring {
    /* VRing的队列深度，表示一个VRing有多少个buffer */
    unsigned int num;
    /* 指向Descriptor Table */
    vring_desc_t *desc;
    /* 指向Avail Ring */
    vring_avail_t *avail;
    /* 指向Used Ring */
    vring_used_t *used;
} vring_t;

typedef struct virtio_blk_config {
    uint64 capacity;
    uint32 size_max;
    uint32 seg_max;
    struct virtio_blk_geometry {
        uint16 cylinders;
        uchar heads;
        uchar sectors;
    } geometry;
    uint32 blk_size;
    struct virtio_blk_topology {
        // # of logical blocks per physical block (log2)
        uchar physical_block_exp;
        // offset of first aligned logical block
        uchar alignment_offset;
        // suggested minimum I/O size in blocks
        uint16 min_io_size;
        // optimal (suggested maximum) I/O size in blocks
        uint32 opt_io_size;
    } topology;
    uchar writeback;
    uchar unused0[3];
    uint32 max_discard_sectors;
    uint32 max_discard_seg;
    uint32 discard_sector_alignment;
    uint32 max_write_zeroes_sectors;
    uint32 max_write_zeroes_seg;
    uchar write_zeroes_may_unmap;
    uchar unused1[3];
}__attribute__((packed)) virtio_blk_config_t;

int vdisk_init(void);
int vdisk_read(uchar *buf, uint64 start_sector, uint64 count);
int vdisk_write(uchar *buf, uint64 start_sector, uint64 count);
void vdisk_intr(void);
void vdisk_getinfo(hal_sd_info_t *info);
#endif