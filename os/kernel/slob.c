#include "slob.h"
#include "page.h"

small_block free_small_blocks = {.unit_n=1, .next=&free_small_blocks};
small_block *p_free_small_blocks = &free_small_blocks;
big_block *p_big_blocks = 0;

void* slob_get_pages(uint64 page_n)
{
    Page *p = alloc_pages(page_n);
    if(!p) return 0;
    else return (void*)get_vm_addr_by_page(p);
}

void slob_free_pages(uint64 va, uint64 page_n)
{
    free_pages(get_page_by_vm_addr(va), page_n);
}

void* small_alloc(uint64 size) // 分配小内存
{
    small_block *pre, *p;
    uint64 unit_n = get_unit_n(size);
    pre = p_free_small_blocks;
    p = pre->next;

    // spin_lock_irqsave(&slob_lock, 0);
    for( ; ;pre=p, p=p->next)
    {
        if(p->unit_n >= unit_n)
        {
            if (p->unit_n == unit_n) // 当前block大小刚好，直接取出
				pre->next = p->next;
			else { // 否则取出该适当大小的块并分裂
				pre->next = p + unit_n;
				pre->next->unit_n = p->unit_n - unit_n;
				pre->next->next = p->next;
				p->unit_n = unit_n;
			}

			p_free_small_blocks = pre;
			 // spin_unlock_irqrestore(&slob_lock, 0);
			return p;
        }
        if(p == p_free_small_blocks) // 未找到合适的则申请新的一页内存
        {
			p = (small_block *)slob_get_pages(1);
            // spin_unlock_irqrestore(&slob_lock, 0);
			if (!p)
				return 0;
			small_free(p, PAGE_SIZE);
			// spin_lock_irqsave(&slob_lock, 0);
			p = p_free_small_blocks;
        }
    }
}

void small_free(void *block, uint64 size)
{
    if (!block)
		return;
	small_block *p, *q;
    p = q = (small_block *)block;
	
	if (size)
		q->unit_n = get_unit_n(size);

	// spin_lock_irqsave(&slob_lock, 0);

    // 找到q在p与p->next之间
	for (p = p_free_small_blocks; !(q > p && q < p->next); p = p->next)
		if (p >= p->next && (q > p || q < p->next))
			break;
    // 此时指针的位置关系：p、q、q->next
    // q向后链接
	if (q + q->unit_n == p->next) { // 如果q、p->next恰好相邻，则合并q与q->next
		q->unit_n += p->next->unit_n;
		q->next = p->next->next;
	} else
		q->next = p->next;

    // q向前链接
	if (p + p->unit_n == q) { // 如果p、q相邻，则合并p与q
		p->unit_n += q->unit_n;
		p->next = q->next;
	} else
		p->next = q;

	p_free_small_blocks = p;

	// spin_unlock_irqrestore(&slob_lock, 0);
}
