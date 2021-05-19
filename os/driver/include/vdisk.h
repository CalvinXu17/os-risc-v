#ifndef VDISK_H
#define VDISK_H

#include "type.h"

// https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.pdf

// virtio mmio control registers, mapped starting at 0x10001000.
// from qemu VIRTIO_MMIO.h

#define VIRTIO_MMIO_MAGIC_VALUE_OFFSET		      0x000 // 0x74726976
#define VIRTIO_MMIO_VERSION_OFFSET		          0x004 // legacy设备为1
#define VIRTIO_MMIO_DEVICE_ID_OFFSET		        0x008 // blk is 2, net is 1
#define VIRTIO_MMIO_VENDOR_ID_OFFSET		        0x00c // 0x554d4551
#define VIRTIO_MMIO_DEVICE_FEATURES_OFFSET	    0x010
#define VIRTIO_MMIO_DRIVER_FEATURES_OFFSET	    0x020
#define VIRTIO_MMIO_GUEST_PAGE_SIZE_OFFSET	    0x028 // page size for PFN, write-only
#define VIRTIO_MMIO_QUEUE_SEL_OFFSET		        0x030 // write, 选择virtual queue that follow options on QueueNumMax, QueueNum, QueueReady, QueueDescLow,QueueDescHigh, QueueAvailLow, QueueAvailHigh, QueueUsedLow andQueueUsedHigh apply to. The index number of the first queue is zero (0x0).
#define VIRTIO_MMIO_QUEUE_NUM_MAX_OFFSET	      0x034 // read, max size of current queue
#define VIRTIO_MMIO_QUEUE_NUM_OFFSET		        0x038 // write, size of current queue
#define VIRTIO_MMIO_QUEUE_ALIGN_OFFSET		      0x03c // write, used ring alignment
#define VIRTIO_MMIO_QUEUE_PFN_OFFSET		        0x040 // read/write, physical page number for queue
#define VIRTIO_MMIO_QUEUE_READY_OFFSET		      0x044 // ready bit, Writing one (0x1) to this register notifies the device that it can execute requests from this virtual queu
#define VIRTIO_MMIO_QUEUE_NOTIFY_OFFSET	        0x050 // write
#define VIRTIO_MMIO_INTERRUPT_STATUS_OFFSET	    0x060 // read
#define VIRTIO_MMIO_INTERRUPT_ACK_OFFSET	      0x064 // write
#define VIRTIO_MMIO_STATUS_OFFSET		            0x070 // read/write

// status register bits, from qemu virtio_config.h
#define VIRTIO_CONFIG_S_ACKNOWLEDGE	            1
#define VIRTIO_CONFIG_S_DRIVER		              2
#define VIRTIO_CONFIG_S_DRIVER_OK	              4
#define VIRTIO_CONFIG_S_FEATURES_OK	            8

// device feature bits
#define VIRTIO_BLK_F_RO                         5	  // Disk is read-only
#define VIRTIO_BLK_F_SCSI                       7	  // Supports scsi command passthru
#define VIRTIO_BLK_F_CONFIG_WCE                 11	// Writeback mode available in config
#define VIRTIO_BLK_F_MQ                         12	// support more than one vq
#define VIRTIO_F_ANY_LAYOUT                     27
#define VIRTIO_RING_F_INDIRECT_DESC             28
#define VIRTIO_RING_F_EVENT_IDX                 29

// this many virtio descriptors.
// must be a power of two.
#define NUM 8

struct VRingDesc {
  uint64 addr;
  uint32 len;
  uint16 flags;
  uint16 next;
};
#define VRING_DESC_F_NEXT  1 // chained with another descriptor
#define VRING_DESC_F_WRITE 2 // device writes (vs read)

struct VRingUsedElem {
  uint32 id;   // index of start of completed descriptor chain
  uint32 len;
};

// for disk ops
#define VIRTIO_BLK_T_IN  0 // read the disk
#define VIRTIO_BLK_T_OUT 1 // write the disk

struct UsedArea {
  uint16 flags;
  uint16 id;
  struct VRingUsedElem elems[NUM];
};

void            virtio_disk_init(void);
void            virtio_disk_rw(struct buf *b, int write);
void            virtio_disk_intr(void);

#endif