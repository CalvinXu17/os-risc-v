#include "cpu.h"
#include "printk.h"
#include "sbi.h"
#include "kmalloc.h"
#include "page.h"
#include "plic.h"
#include "console.h"
#include "fpioa.h"
#include "sdcard_test.h"
#include "disk.h"
#include "fat32.h"

extern void intr_init(void);

static volatile init_done = 0;

extern uint64 k_pg_t[512];

extern struct disk dsk;

void kstart(uint64 hartid)
{
    cpu_init(hartid);
    printk("init hartid: %ld\n", hartid);
    if(hartid == 0) // 只允许一个cpu核执行的初始化操作
    {
        printk("os init...\n");
        page_init();
        console_init();
        // plicinit();
        fpioa_pin_init();

        if(!sdcard_init())
        {
            printk("sdcard init success\n");
            disk_init(&dsk);
            fat32_init(&dsk);
            
        } else printk("sdcard init failed\n");

        for(int i=1;i < CPU_N; i++)
        {
            uint64 mask = 1 << i;
            sbi_send_ipi(&mask); // 启动其他的核
        }
        __sync_synchronize();
        init_done = 1;
    }
    while(!init_done) {}
    __sync_synchronize();

    //plicinithart();
    intr_init();
    
    printk("os init success\n");
    while(1)
    {

    }
}