#include "printk.h"
#include "sbi.h"
#include "kmalloc.h"
#include "page.h"

extern void intr_init(void);

extern uint64 kernel_stack[4096];


void kstart(uint64 a0, uint64 a1)
{
    printk("os init...\n");

    page_init();
    printk("%lx %lx\n", a0, (uint64)kstart);

    Page *p1,*p2,*p3;

    p1 = alloc_pages(1);
    p2 = alloc_pages(2);
    p3 = alloc_pages(3);
    printk("page1 vm: 0x%lx\n", get_vm_addr_by_page(p1));
    printk("page2 vm: 0x%lx\n", get_vm_addr_by_page(p2));
    printk("page3 vm: 0x%lx\n", get_vm_addr_by_page(p3));

    free_pages(p3, 3);
    free_pages(p1, 1);
    free_pages(p2, 2);
    

    p1 = alloc_pages(1);
    printk("page1 vm: 0x%lx\n", get_vm_addr_by_page(p1));
    free_pages(p1, 1);


    void *p = kmalloc(4096);
    printk("p: 0x%lx\n", p);
    kfree(p);
    p = kmalloc(4096);
    printk("p: 0x%lx\n", p);
    kfree(p);

    p = kmalloc(4);
    printk("p: 0x%lx\n", p);
    kfree(p);
    p = kmalloc(8);
    printk("p: 0x%lx\n", p);
    kfree(p);

    p = kmalloc(4096);
    printk("p: 0x%lx\n", p);
    kfree(p);

    
    intr_init();
    printk("os init success\n");
    while(1)
    {

    }
}