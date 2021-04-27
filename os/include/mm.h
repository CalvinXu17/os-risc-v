#ifndef MM_H
#define MM_H

#define PM_SIZE  0x600000 // 6M
#define PM_START 0x80000000
#define PM_END   (PM_START+PM_SIZE)

#define PV_OFFSET  0xFFFFFFFF00000000

#define VM_START (PM_START+PV_OFFSET)
#define VM_END   (VM_START+PM_SIZE)

#ifdef _QEMU
#define K_OFFSET 0x200000
#endif
#ifdef _K210
#define K_OFFSET 0x20000
#endif

#define KERNEL_PM_START (PM_START+K_OFFSET)
#define KERNEL_VM_START (KERNEL_PM_START+PV_OFFSET)

#endif