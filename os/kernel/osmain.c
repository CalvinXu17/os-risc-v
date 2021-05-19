#include "cpu.h"
#include "panic.h"
#include "intr.h"
#include "kmalloc.h"
#include "page.h"
#include "slob.h"
#include "plic.h"
#include "console.h"
#include "fpioa.h"
#include "sdcard_test.h"
#include "xdisk.h"
#include "xfat.h"
#include "xtypes.h"
#include "string.h"

#include "process.h"
#include "sched.h"
#include "syscall.h"

static volatile int init_done = 0;


// extern struct disk dsk;
// extern struct fat fat32;
// extern struct fat_file root_dir;
extern xdisk_driver_t vdisk_driver;
uchar disk_buf[XFAT_BUF_SIZE(512, 2)] = {0};
uchar fat_buf[XFAT_BUF_SIZE(512, 2)] = {0};
xdisk_t disk;
xfat_t xfat;

void osmain(uint64 hartid)
{
    cpu_init(hartid);
    if(hartid == 0)
        printk_init();
    
    #ifdef _DEBUG
    printk("init hartid: %ld\n", hartid);
    #endif

    if(hartid == 0) // 只允许一个cpu核执行的初始化操作
    {
        #ifdef _DEBUG
        printk("os init...\n");
        #endif
        page_init();
        slob_init();
        console_init();
        plicinit();
        fpioa_pin_init();
        if(!sdcard_init())
        {
            printk("sdcard init success\n");
            xfat_err_t err;
            xdisk_part_t disk_part;
            memset(disk_buf, 0, sizeof(disk_buf));
            memset(disk_buf, 0, sizeof(fat_buf));
            err = xdisk_open(&disk, "sdcard", &vdisk_driver, "sdcard", disk_buf, sizeof(disk_buf));
            if (err < 0) {
                panic("xdisk_open failed!\n");
            }

            err = xdisk_get_part(&disk, &disk_part, 0);
            if (err < 0) {
                panic("xdisk_get_part failed!\n");
            }

            err = xfat_init();
            if (err < 0) {
                panic("xfat_init failed!\n");
            }

            err = xfat_mount(&xfat, &disk_part, "root");
            if (err < 0) {
                panic("xfat_mount failed!\n");
            }

            err = xfat_set_buf(&xfat, fat_buf, sizeof(fat_buf));

            if (err < 0) {
                panic("xfat_set_buf failed!\n");
            }
        } else panic("sdcard init failed\n");
        sched_init();
        syscall_init();
        proc_init();
        
        intr_init();
        plicinithart();
        user_init(hartid);
        for(int i=1;i < CPU_N; i++)
        {
            uint64 mask = 1 << i;
            sbi_send_ipi(&mask); // 启动其他的核
        }
        __sync_synchronize();
        init_done = 1;
    } else
    {
        while(!init_done) {}
        __sync_synchronize();
        intr_init();
        plicinithart();
    }
    #ifdef _DEBUG
    if(hartid)
        printk("os init success\n");
    #endif
    scheduler();
    while(1)
    {

    }
}