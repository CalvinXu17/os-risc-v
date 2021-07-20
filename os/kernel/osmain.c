#include "cpu.h"
#include "panic.h"
#include "intr.h"
#include "kmalloc.h"
#include "page.h"
#include "slob.h"
#include "plic.h"
#include "console.h"
#include "process.h"
#include "sched.h"
#include "syscall.h"
#include "fs.h"

static volatile int init_done = 0;

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
        init_done = 0;
        #ifdef _DEBUG
        printk("os init...\n");
        #endif
        page_init();
        slob_init();
        console_init();

        if(!fs_init())
        {
            panic("fs init failed\n");
        }
        #ifdef _DEBUG 
        else {
            printk("fs init success\n");
        }
        #endif

        sched_init();
        syscall_init();
        proc_init();

        plicinit();
        plicinithart();
        intr_init();
        
        user_init(hartid);
        // for(int i=1;i < CPU_N; i++)
        // {
        //     uint64 mask = 1 << i;
        //     sbi_send_ipi(&mask); // 启动其他的核
        // }
        // __sync_synchronize();
        // init_done = 1;
    } else
    {
        while(!init_done) {}
        __sync_synchronize();
        plicinithart();
        intr_init();
        user_init(hartid);
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