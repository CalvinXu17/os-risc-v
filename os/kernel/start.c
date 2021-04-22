#include "cpu.h"
#include "printk.h"
#include "sbi.h"
#include "kmalloc.h"
#include "page.h"

extern void intr_init(void);

static volatile init_done = 0;

void kstart(uint64 hartid)
{
    cpu_init(hartid);
    printk("init hartid: %ld\n", hartid);
    if(hartid == 0) // 只允许一个cpu核执行的初始化操作
    {
        printk("os init...\n");
        page_init();
        for(int i=1;i < CPU_N; i++)
        {
            uint64 mask = 1 << i;
            sbi_send_ipi(&mask); // 启动其他的核
        }
        init_done = 1;
    }

    while(!init_done) {}
    Page *p1,*p2,*p3;

    p1 = alloc_pages(1);
    p2 = alloc_pages(2);
    p3 = alloc_pages(3);
    printk("%d: page1 vm: 0x%lx\n", hartid, get_vm_addr_by_page(p1));
    printk("%d: page2 vm: 0x%lx\n", hartid, get_vm_addr_by_page(p2));
    printk("%d: page3 vm: 0x%lx\n", hartid, get_vm_addr_by_page(p3));

    free_pages(p3, 3);
    free_pages(p1, 1);
    free_pages(p2, 2);
        

    p1 = alloc_pages(1);
    printk("%d: page1 vm: 0x%lx\n", hartid, get_vm_addr_by_page(p1));
    free_pages(p1, 1);


    void *p = kmalloc(4096);
    printk("%d: p: 0x%lx\n", hartid, p);
    kfree(p);
    p = kmalloc(4096);
    printk("%d: p: 0x%lx\n", hartid, p);
    kfree(p);

    p = kmalloc(4);
    printk("%d: p: 0x%lx\n", hartid, p);
    kfree(p);
    p = kmalloc(8);
    printk("%d: p: 0x%lx\n", hartid, p);
    kfree(p);

    p = kmalloc(4096);
    printk("%d: p: 0x%lx\n", hartid, p);
    kfree(p);
    
    intr_init();
    printk("os init success\n");
    while(1)
    {

    }
}