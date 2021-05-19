#include "page.h"
#include "printk.h"
#include "panic.h"

Page pages_map[PAGE_NUM];
Page_Pool free_page_pool;

uint64 align4k(uint64 s)
{
    if((s & 0xfff)==0)
        return s;
    else return ((s>>12)+1) << 12;
}

void page_init(void)
{
    #ifdef _DEBUG
    printk("page init...\n");
    #endif
    extern char _start[1]; // _start 与 _end 在link.ld中导出
    extern char _end[1];
    uint64 k_size = (uint64)_end - (uint64)_start;
    #ifdef _DEBUG
    printk("kernel size: 0x%lx\n", k_size);
    #endif
    k_size = align4k(k_size);
    assert(k_size <= (PAGE_NUM << 12));

    #ifdef _DEBUG
    printk("aligned kernel size: 0x%lx\n", k_size);
    #endif

    int i;
    for(i=0;i<(k_size >> 12)+(K_OFFSET>>12);i++)  // 初始化sbi以及内核所占的页面
    {
        pages_map[i].flag = PAGE_RESERVE;
        pages_map[i].ref_cnt = 1;
        pages_map[i].free_page_num = 0;
        init_list(&(pages_map[i].page_list));
    }

    Page *p = &pages_map[i];
    p->flag = PAGE_FREE;
    p->free_page_num = PAGE_NUM - i;
    init_list(&(pages_map[i].page_list));

    free_page_pool.page_num = p->free_page_num;
    init_list(&(free_page_pool.pages_list));
    add_list(&(free_page_pool.pages_list), &(p->page_list));
    
    for(i=i+1; i < PAGE_NUM; i++)
    {
        pages_map[i].flag = PAGE_FREE;
        pages_map[i].free_page_num = 0;
        pages_map[i].ref_cnt = 0;
        init_list(&(pages_map[i].page_list));
    }
    #ifdef _DEBUG
    printk("page init success!\n");
    #endif
}

Page* alloc_pages(uint64 n) // 首次适应法
{
    if(n>free_page_pool.page_num) // 空闲页面不足
        return 0;
    
    list *l=free_page_pool.pages_list.next;
    Page *page = 0;
    while(l != &(free_page_pool.pages_list))
    {
        if(list2page(l)->free_page_num >= n)
        {
            page = list2page(l);
            break;
        }
        l=l->next;
    }

    if(page)
    {
        del_list(&(page->page_list));
        if(page->free_page_num > n)
        {
            Page *page_next = page + n;
            page_next->free_page_num = page->free_page_num - n;
            add_list(&(free_page_pool.pages_list), &(page_next->page_list));
        }

        Page *t;
        for(t=page; t < page + n; t++)
        {
            t->flag = PAGE_USED;
            t->ref_cnt++;
            t->free_page_num = 0;
        }
        // page->free_page_num = n;
        free_page_pool.page_num -= n;
    }
    return page;
}

void free_pages(Page *page, uint64 n)
{
    Page *p;
    list *l;
    for(p=page; p != page+n; p++)
        if(p->flag != PAGE_USED)
            return;
        else
        {
            p->flag = PAGE_FREE;
            p->ref_cnt--;
        }
    
    page->free_page_num = n;
    free_page_pool.page_num += n;
    
    if(is_empty_list(&free_page_pool.pages_list))
    {
        add_list(&free_page_pool.pages_list, &(page->page_list));
    }
    else
    {
        l = free_page_pool.pages_list.next;
        int flag = 0;
        while(l != &(free_page_pool.pages_list))
        {
            p = list2page(l);
            if(page < p)
            {
                add_before(l, &(page->page_list));
                flag = 1;
                break;
            }
            l = l->next;
        }
        if(!flag)
        {
            add_before(&(free_page_pool.pages_list), &(page->page_list));
        }
    }
    // 开始合并
    l = page->page_list.prev;
    if(l!=&(free_page_pool.pages_list)) // 向前合并
    {
        p = list2page(l);
        if(p + p->free_page_num == page)
        {
            p->free_page_num += page->free_page_num;
            del_list(&(page->page_list));
            page = p;
        }
        l = l->prev;
    }
    l = page->page_list.next;
    if(l!=&(free_page_pool.pages_list)) // 向后合并
    {
        p = list2page(l);
        if(page + page->free_page_num == p)
        {
            page->free_page_num += p->free_page_num;
            del_list(&(p->page_list));
        }
    }
}
