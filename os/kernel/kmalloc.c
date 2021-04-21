#include "kmalloc.h"
#include "slob.h"
#include "page.h"

static uint64 align16(uint64 size)
{
    if((size & 0xf)==0)
        return size;
    else return ((size>>4)+1) << 4;
}

static uint64 get_page_n(uint64 size)
{
    if((size & (PAGE_SIZE-1))==0)
        return size >> 12;
    else return ((size>>12)+1);
}

void *kmalloc(uint64 size)
{
	small_block *m;
	big_block *bigblock;
    size = align16(size);
	if (size < PAGE_SIZE - UNIT_SIZE) {
		m = small_alloc(size + UNIT_SIZE);
        if(!m) return 0;
        else return (void*)(m+1);
	}

	bigblock = small_alloc(sizeof(big_block));
	if (!bigblock)
		return 0;
    uint64 page_n = get_page_n(size); // 大小超过一页则分配页面大小按页对齐
	bigblock->page_n = page_n;
	bigblock->pages = (void *)slob_get_pages(page_n);

	if (bigblock->pages) { // 分配n页成功
		// spin_lock_irqsave(&block_lock, flags);
		bigblock->next = p_big_blocks;
		p_big_blocks = bigblock;
		// spin_unlock_irqrestore(&block_lock, flags);
		return bigblock->pages;
	}

	small_free(bigblock, sizeof(big_block));
	return 0;
}

void kfree(void *addr)
{
	big_block *bigblock;
    big_block **last = &p_big_blocks; // 用于修改p_big_blocks

	if (!addr)
		return;

	if (!((uint64)addr & (PAGE_SIZE-1))) { // 如果block的地址按页对齐，则说明这个block可能是big_block，但也可能不是，因此需要查找p_big_blocks
		// spin_lock_irqsave(&block_lock, flags);
		for (bigblock = p_big_blocks; bigblock != 0; last = &bigblock->next, bigblock = bigblock->next) {
			if (bigblock->pages == addr) {
				*last = bigblock->next;
				// spin_unlock_irqrestore(&block_lock, flags);
				slob_free_pages((uint64)addr, bigblock->page_n);
				small_free(bigblock, sizeof(big_block));
				return;
			}
		}
		// spin_unlock_irqrestore(&block_lock, flags);
	}

	small_free(((small_block *)addr) - 1, 0);
	return;
}

uint64 ksize(const void *addr)
{
	big_block *bigblock;
	if (!addr)
		return 0;

	if (!((uint64)addr & (PAGE_SIZE-1))) {
		//spin_lock_irqsave(&block_lock, flags);
		for (bigblock = p_big_blocks; bigblock != 0; bigblock = bigblock->next)
			if (bigblock->pages == addr) {
				// spin_unlock_irqrestore(&slob_lock, flags);
				return bigblock->page_n * PAGE_SIZE;
			}
		//spin_unlock_irqrestore(&block_lock, flags);
	}

	return (((small_block *)addr) - 1)->unit_n * UNIT_SIZE;
}