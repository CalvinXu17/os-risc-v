#include "vmm.h"
#include "mm.h"
#include "cpu.h"
#include "string.h"
#include "kmalloc.h"
#include "process.h"
#include "signal.h"

struct vma* new_vma(void)
{
    struct vma *vma = kmalloc(sizeof(struct vma));
    init_list(&vma->vma_list_node);
    vma->file = NULL;
    vma->type = 0;
    vma->foff = 0;
    vma->fsize = 0;
    vma->prot = 0;
    vma->va = vma->size = vma->vmoff = 0;
    return vma;
}

void free_vma_mem(uint64 *pgt, struct vma *vma)
{
    uint64 pa = va2pa(pgt, vma->va);
    if(vma->file)
    {
        if(pa)
        {
            vm_free_pages(pa + PV_OFFSET, 1);
            unmappages(pgt, vma->va, 1);
        }
        
        return;
    }
    uint64 vai;
    for(vai = vma->va; vai < vma->va + vma->size; vai += PAGE_SIZE)
    {
        pa = va2pa(pgt, vai);
        
        if(pa)
        {
            if(vma->file)
            {
                // uint64 file_begin_va = vma->va + vma->vmoff;
                // int fd = vfs_file2fd(vma->file);
                // if(vai <= file_begin_va)
                // {
                //     vfs_lseek(fd, vma->foff, VFS_SEEK_SET);
                //     uint64 page_remain = PAGE_SIZE - (file_begin_va - vai);
                //     vfs_write(fd, (uint64)pa + PV_OFFSET + (file_begin_va - vai), page_remain);
                // } else
                // {
                //     uint64 file_begin = vma->foff + vai - file_begin_va;
                //     vfs_lseek(fd, file_begin, VFS_SEEK_SET);
                //     vfs_write(fd, (uint64)pa + PV_OFFSET, PAGE_SIZE);
                // }
                
            }

            if(!isforkmem(pgt, vai))
            {
                vm_free_pages(pa + PV_OFFSET, 1);
            }

            unmappages(pgt, vai, 1);
        }
        
    }
    vma->file = NULL;
}

struct vma* find_vma(struct Process *proc, uint64 va)
{
    list *l = proc->vma_list_head.next;
    struct vma *vma = NULL;
    while(l != &proc->vma_list_head)
    {
        vma = vma_list_node2vma(l);
        if(va < vma->va)  // vma链表按va由小到大排列
        {
            break;
        }
        else if(va >= vma->va && va < (vma->va + vma->size))
        {
            return vma;
        }
        l = l->next;
    }
    return NULL;
}

int add_vma(struct Process *proc, struct vma *vma)
{
    lock(&proc->lock);
    list *l = proc->vma_list_head.next;
    struct vma *v = NULL;
    while(l != &proc->vma_list_head)
    {
        v = vma_list_node2vma(l);
        
        if(vma->va < v->va)
        {
            add_before(l, &vma->vma_list_node);
            unlock(&proc->lock);
            return 1;
        } else if(vma->va == v->va) // 已存在
        {
            unlock(&proc->lock);
            return 0;
        }

        l = l->next;
    }
    add_before(&proc->vma_list_head, &vma->vma_list_node);
    unlock(&proc->lock);
    return 1;
}

uint64 prot2pte(uint64 prot)
{
    uint64 rt = PTE_U;
    if(prot & PROT_READ)
        rt |= PTE_R;
    if(prot & PROT_WRITE)
        rt |= PTE_W;
    if(prot & PROT_EXEC)
        rt |= PTE_X;
    return rt;
}

void copy_kpgt(uint64 *pg_t)
{
    pg_t[508] = k_pg_t[508];
    pg_t[509] = k_pg_t[509];
    pg_t[510] = k_pg_t[510];
}

uint64 mcnt = 0;
void *vm_alloc_pages(uint64 pagecnt)
{
    Page *page = alloc_pages(pagecnt);
    if(!page) return NULL;
    void *mem = get_kvm_addr_by_page(page);
    memset(mem, 0, pagecnt * PAGE_SIZE);
    return mem;
}

void vm_free_pages(void *kva, uint64 pagecnt)
{
    if((uint64)kva & (PAGE_SIZE-1)) return;
    free_pages(get_page_by_kvm_addr(kva), pagecnt);
}

// 这里 va 与 pa 必须按页对齐，只映射页表，内存分配由外部完成并作为pa参数输入
int mappages(uint64 *pg0_t, uint64 va, uint64 pa, uint64 page_cnt, uint64 pte_flags)
{
    if(!pg0_t || !page_cnt) return 0;
    if(PAGEDOWN(va) != va || PAGEDOWN(pa) != pa)
        return 0;
    
    uint64 *pg0, *pg1, *pg2;
    uint64 cnt;

    for(cnt = 0; cnt < page_cnt; cnt++, va += PAGE_SIZE, pa += PAGE_SIZE)
    {
        uint64 pg0_index = (va >> 30) & 0x1FF;
        uint64 pg1_index = (va >> 21) & 0x1FF;
        uint64 pg2_index = (va >> 12) & 0x1FF;

        pg0 = pg0_t;
        if(!(pg0[pg0_index] & PTE_V))
        {
            pg1 = vm_alloc_pages(1);
            if(!pg1)
                return 0;
            pg0[pg0_index] = PA2PTE((uint64)pg1 - PV_OFFSET) | PTE_V;
        } else pg1 = PTE2PA(pg0[pg0_index]) + PV_OFFSET;

        if(!(pg1[pg1_index] & PTE_V))
        {
            pg2 = vm_alloc_pages(1);
            if(!pg2)
                return 0;
            pg1[pg1_index] = PA2PTE((uint64)pg2 - PV_OFFSET) | PTE_V;
        } else pg2 = PTE2PA(pg1[pg1_index]) + PV_OFFSET;

        pg2[pg2_index] = PA2PTE(pa) | PTE_V | pte_flags;
    }
    return 1;
}

// 这里 va 与 pa 必须按页对齐，只映射页表，内存分配由外部完成并作为pa参数输入
int unmappages(uint64 *pg0_t, uint64 va, uint64 page_cnt)
{
    if(!pg0_t || !page_cnt) return 0;
    if(PAGEDOWN(va) != va)
        return 0;
    
    uint64 *pg0, *pg1, *pg2;
    uint64 cnt;

    for(cnt = 0; cnt < page_cnt; cnt++, va += PAGE_SIZE)
    {
        uint64 pg0_index = (va >> 30) & 0x1FF;
        uint64 pg1_index = (va >> 21) & 0x1FF;
        uint64 pg2_index = (va >> 12) & 0x1FF;

        pg0 = pg0_t;
        if(pg0[pg0_index] & PTE_V)
        {
            pg1 = PTE2PA(pg0[pg0_index]) + PV_OFFSET;
            if(pg1[pg1_index] & PTE_V)
            {
                pg2 = PTE2PA(pg1[pg1_index]) + PV_OFFSET;
                pg2[pg2_index] = 0; // 取消映射
            }
        }
    }
    return 1;
}

// 只释放页表所占的内存，页表所映射的内存由vma释放
void free_pagetable(uint64 *pg_t)
{
    if(!pg_t) return;

    uint64 *pg0, *pg1, *pg2, *pg3;
    int i, j;
    pg0 = pg_t;
    for(i = 0; i < 256; i++)
    {
        if(pg0[i] & PTE_V)
        {
            pg1 = PTE2PA(pg0[i]) + PV_OFFSET;
            if(!(pg0[i] & PTE_R) && !(pg0[i] & PTE_W) && !(pg0[i] & PTE_X)) // 指向下一级
            {
                for(j = 0; j < 512; j++)
                {
                    if(pg1[j] & PTE_V)
                    {
                        pg2 = PTE2PA(pg1[j]) + PV_OFFSET;
                        vm_free_pages(pg2, 1);
                    }
                }
            }
            vm_free_pages(pg1, 1);
        }
    }
    vm_free_pages(pg0, 1);
}

uint64* copy_pagetable(uint64 *pg_to, uint64 *pg_from)
{
    if(!pg_to) pg_to = kmalloc(PAGE_SIZE);
    memset(pg_to, 0, PAGE_SIZE);
    pg_to[508] = pg_from[508];
    pg_to[509] = pg_from[509];
    pg_to[510] = pg_from[510]; // 拷贝内核目录项
    int i, j, k;
    uint64 *pg_from1, *pg_from2, *pg_from3, *pg_to1, *pg_to2, *pg_to3;

    for(i = 0; i < 256; i++)
    {
        if(pg_from[i] & PTE_V)
        {
            pg_from1 = PTE2PA(pg_from[i]) + PV_OFFSET;
            
            if(!(pg_from[i] & PTE_R) && !(pg_from[i] & PTE_W) && !(pg_from[i] & PTE_X)) // 指向下一级
            {
                pg_to1 = kmalloc(PAGE_SIZE);
                pg_to[i] = PA2PTE((uint64)pg_to1 - PV_OFFSET) | PTE_V;
                for(j = 0; j < 512; j++)
                {
                    if(pg_from1[j] & PTE_V)
                    {
                        pg_from2 = PTE2PA(pg_from1[j]) + PV_OFFSET;
                        pg_to1 = kmalloc(PAGE_SIZE);
                        if(!(pg_from1[j] & PTE_R) && !(pg_from1[j] & PTE_W) && !(pg_from1[j] & PTE_X)) // 指向下一级
                        {
                            pg_to1[j] = PA2PTE((uint64)pg_to2 - PV_OFFSET) | PTE_V;
                            for(k = 0; k < 512; k++)
                            {
                                if(pg_from2[k] & PTE_V)
                                {
                                    pg_from3 = PTE2PA(pg_from2[k]) + PV_OFFSET;
                                    pg_to3 = kmalloc(PAGE_SIZE);
                                    memcpy(pg_to3, pg_from3, PAGE_SIZE);
                                    pg_to2[k] = PA2PTE((uint64)pg_to3 - PV_OFFSET) | PTE_V | PTE_R | PTE_W  | PTE_X | PTE_U;
                                } else pg_to2[k] = 0;
                            }
                        }
                    } else pg_from1[j] = 0;
                }
            }
        } else pg_to[i] = 0;
    }
    return pg_to;
}

void switch2kpgt(void)
{
    uint64 sapt = ((((uint64)k_pg_t) - PV_OFFSET) >> 12) | (8L << 60);
    set_satp(sapt); // 先切换回内核页表再释放当前进程页表
    sfence_vma();
}

uint64 va2pa(uint64 *pgt_kva, uint64 va)
{
    uint64 pg0_index = (va >> 30) & 0x1FF;
    uint64 pg1_index = (va >> 21) & 0x1FF;
    uint64 pg2_index = (va >> 12) & 0x1FF;
    uint64 offset = va & 0xfff;
    uint64 *pg=pgt_kva;
    uint64 pte = 0;
    pte = pg[pg0_index];
    if(!(pte & PTE_V)) return 0;
    pg = PTE2PA(pte) + PV_OFFSET;

    pte = pg[pg1_index];
    if(!(pte & PTE_V)) return 0;
    pg = PTE2PA(pte) + PV_OFFSET;

    pte = pg[pg2_index];
    if(!(pte & PTE_V)) return 0;
    
    return (uint64)(PTE2PA(pte)) + offset;
}

uint64 va2pte(uint64 *pgt_kva, uint64 va)
{
    uint64 pg0_index = (va >> 30) & 0x1FF;
    uint64 pg1_index = (va >> 21) & 0x1FF;
    uint64 pg2_index = (va >> 12) & 0x1FF;
    uint64 offset = va & 0xfff;
    uint64 *pg=pgt_kva;
    uint64 pte = 0;
    pte = pg[pg0_index];
    if(!(pte & PTE_V)) return 0;
    pg = PTE2PA(pte) + PV_OFFSET;

    pte = pg[pg1_index];
    if(!(pte & PTE_V)) return 0;
    pg = PTE2PA(pte) + PV_OFFSET;

    pte = pg[pg2_index];
    if(!(pte & PTE_V)) return 0;
    
    return pte;
}

int isforkmem(uint64 *pgt_kva, uint64 va)
{
    uint64 pg0_index = (va >> 30) & 0x1FF;
    uint64 pg1_index = (va >> 21) & 0x1FF;
    uint64 pg2_index = (va >> 12) & 0x1FF;
    uint64 offset = va & 0xfff;
    uint64 *pg=pgt_kva;
    uint64 pte = 0;
    pte = pg[pg0_index];
    if(!(pte & PTE_V)) return 0;
    pg = PTE2PA(pte) + PV_OFFSET;

    pte = pg[pg1_index];
    if(!(pte & PTE_V)) return 0;
    pg = PTE2PA(pte) + PV_OFFSET;

    pte = pg[pg2_index];
    if(!(pte & PTE_V)) return 0;
    
    if(pte & PTE_F) return 1;
    return 0;
}

void* userva2kernelva(uint64 *pgt_kva, uint64 userva)
{
    uint64 pa = va2pa(pgt_kva, userva);
    if(!pa) return NULL;
    return (void*)(pa + PV_OFFSET);
}

int copy2user(uint64 *pgt_user, uchar *va_to_user, uchar *va_from, uint64 len)
{
    uint64 va, kva;

    while(len > 0)
    {
        va = PAGEDOWN((uint64)va_to_user);
        kva = userva2kernelva(pgt_user, va_to_user);
        if(!kva) return 0;
        int n = PAGE_SIZE - (uint64)va_to_user + va;
        if(n > len)
            n = len;
        memmove(kva, va_from, n);
        len -= n;
        va_to_user += n;
        va_from += n;
    }
    return 1;
}

void do_exec_fault(struct Process *proc, uint64 badaddr)
{
    uint64 badpage = PAGEDOWN(badaddr);
    struct vma *vma = find_vma(proc, badaddr);
    void *mem = vm_alloc_pages(1);
    if(vma && (vma->type & VMA_PROG)) // 从elf文件中拷贝
    {
        int fd = proc->kfd;
        uint64 file_begin_va = vma->va + vma->vmoff; // 该段的起始va
        if(badpage <= file_begin_va) // 此页为该段的首页且起始地址不是按页对齐
        {
            vfs_lseek(fd, vma->foff, VFS_SEEK_SET);
            uint64 page_remain = PAGE_SIZE - (file_begin_va - badpage);
            uint64 read_size = vma->fsize < page_remain ? vma->fsize : page_remain;
            uint64 size = vfs_read(fd, (uint64)mem + (file_begin_va - badpage), read_size);
            // printk("first seek %lx pageoff %lx readsize %lx:%lx\n", vma->foff, (file_begin_va - badpage), read_size, size);
        } else
        {
            uint64 file_begin = vma->foff + badpage - file_begin_va;
            if(vma->fsize > (file_begin - vma->foff))
            {
                uint64 file_remain = vma->fsize - (file_begin - vma->foff);
                vfs_lseek(fd, file_begin, VFS_SEEK_SET);
                uint64 read_size = file_remain < PAGE_SIZE ? file_remain : PAGE_SIZE;
                uint64 size = vfs_read(fd, mem, read_size);
                // printk("seek %lx pageoff %lx readsize %lx:%lx\n", file_begin, badpage - file_begin_va, read_size, size);
            }
        }
    } else { // 执行到了非法内存中
        vm_free_pages(mem, 1);
        printk("illegal exec\n");
        while(1) ;
    }
    mappages(proc->pg_t, badpage, (uint64)mem - PV_OFFSET, 1, prot2pte(vma->prot));
    sfence_vma();
}

void do_page_fault(struct Process *proc, uint64 badaddr)
{
    uint64 badpage = PAGEDOWN(badaddr);
    uint64 pa = va2pa(proc->pg_t, badpage);
    struct vma *vma = find_vma(proc, badaddr);
    void *mem = NULL;

    if(pa == 0) // 未分配内存的情况
        mem = vm_alloc_pages(1);
    else mem = pa + PV_OFFSET; // 已分配，但触发了缺页异常则一定是读写属性的问题
    
    if(pa)
        send_signal(proc, SIGBUS);
    else
        send_signal(proc, SIGSEGV);

    if(vma)
    {
        // 先映射内存
        if((vma->type & VMA_CLONE) && isforkmem(proc->pg_t, badpage)) // 写时复制
        {
            memcpy(mem, pa + PV_OFFSET, PAGE_SIZE);
            vma->prot |= PROT_READ | PROT_WRITE;
        } else
        {
            if(vma->type & VMA_PROG) // 位于可执行文件中需要拷贝至指定内存
            {
                int fd = proc->kfd;
                uint64 file_begin_va = vma->va + vma->vmoff; // 该段的起始va
                if(badpage <= file_begin_va) // 此页为该段的首页且起始地址不是按页对齐
                {
                    vfs_lseek(fd, vma->foff, VFS_SEEK_SET);
                    uint64 page_remain = PAGE_SIZE - (file_begin_va - badpage);
                    uint64 read_size = vma->fsize < page_remain ? vma->fsize : page_remain;
                    vfs_read(fd, (uint64)mem + (file_begin_va - badpage), read_size);
                } else
                {
                    uint64 file_begin = vma->foff + badpage - file_begin_va;
                    if(vma->fsize > (file_begin - vma->foff))
                    {
                        uint64 file_remain = vma->fsize - (file_begin - vma->foff);
                        vfs_lseek(fd, file_begin, VFS_SEEK_SET);
                        uint64 read_size = file_remain < PAGE_SIZE ? file_remain : PAGE_SIZE;
                        vfs_read(fd, mem, read_size);
                    }
                }
                if(pa) vma->prot |= PROT_READ | PROT_WRITE;
            } else if(vma->file) // 此为map映射的文件
            {
                // printk("mmap\n");
                uint64 first_pa = va2pa(proc->pg_t, vma->va);
                if(first_pa)
                {
                    if(mem)
                        vm_free_pages(mem, 1);
                    mem = first_pa + PV_OFFSET;
                } else
                {
                    mappages(proc->pg_t, vma->va, (uint64)mem-PV_OFFSET, 1, prot2pte(vma->prot));
                }
                // vfs_fstat_t st;
                // if(vfs_stat(vma->file->relative_path, &st) < 0)
                // {
                //     // kill
                //     send_signal(proc, SIGTERM);
                //     printk("do page fault fail stat\n");
                //     return;
                // }
                // int fd = vfs_file2fd(vma->file);
                // uint64 file_begin_va = vma->va + vma->vmoff;
                // if(badpage <= file_begin_va)
                // {
                //     vfs_lseek(fd, vma->foff, VFS_SEEK_SET);
                //     uint64 page_remain = PAGE_SIZE - (badaddr - badpage);
                //     uint64 read_size = vma->fsize < page_remain ? vma->fsize : page_remain;
                //     vfs_read(fd, (uint64)mem + (badaddr - badpage), read_size);
                // } else
                // {
                //     uint64 file_begin = vma->foff + badpage - file_begin_va;
                //     if(vma->fsize > (file_begin - vma->foff))
                //     {
                //         uint64 file_remain = vma->fsize - (file_begin - vma->foff);
                //         vfs_lseek(fd, file_begin, VFS_SEEK_SET);
                //         uint64 read_size = file_remain < PAGE_SIZE ? file_remain : PAGE_SIZE;
                //         vfs_read(fd, mem, read_size);
                //     }
                // }
            } else // else 则为mmap或brk开辟但未映射的空间
            {

            }
        }
    } else // 此处为用户栈因为vma不存在则为栈，或者也可能是程序读写了非法地址，直接分配一页内存即可
    {
        vma = new_vma();
        vma->va = badpage;
        vma->size = PAGE_SIZE;
        vma->prot = PROT_READ | PROT_WRITE;
        vma->flag = PTE_R | PTE_W;
        vma->type = VMA_DATA;
        vma->vmoff = 0;
        vma->fsize = 0;
        vma->foff = 0;
        vma->file = NULL;
        add_vma(proc, vma);
    }
    mappages(proc->pg_t, badpage, (uint64)mem-PV_OFFSET, 1, prot2pte(vma->prot)); // 映射一页内存
    sfence_vma();
}

// 从MAP_LOW至MAP_TOP区间内自动选取合适的空间
static struct vma* _auto_find_map_area(struct Process *proc, uint64 len)
{
    uint64 addr;
    struct vma *vma, *vv, *tmp;
    vma = NULL;
    for(addr = MAP_LOW; addr < MAP_TOP; )
    {
        vv = find_vma(proc, addr);
        if(!vv) // 起始的一页未被使用，检查剩余是否被使用
        {
            int f = 1;
            uint64 addri;
            for(addri = addr + PAGE_SIZE; addri < addr + len; addri += PAGE_SIZE)
            {
                tmp = find_vma(proc, addri);
                if(tmp) // 剩余页有被使用，则起始地址无效
                {
                    f = 0;
                    break;
                }
            }
            if(f) // 找到
            {
                vma = new_vma();
                vma->va = addr;
                vma->size = len;
                break;
            }
            else {
                addr = tmp->va + tmp->size;
            }
        } else // 起始地址已被使用，检测下一块内存
        {
            addr = vv->va + vv->size;
        }
    }
    return vma;
}

struct vma* find_map_area(struct Process *proc, uint64 start, uint64 len)
{
    struct vma *vma = NULL;
    if(!(start >= MAP_LOW && start < MAP_TOP))
        start = 0;
    
    if(!start) // 自动搜索
    {
        vma = _auto_find_map_area(proc, len);
    } else
    {
        start = PAGEDOWN(start);
        uint64 addr;
        int f = 1;
        for(addr = start; addr < start + len; addr += PAGE_SIZE)
        {
            if(find_vma(proc, addr))
            {
                f = 0;
                break;
            }
        }
        if(f)
        {
            vma = new_vma();
            vma->va = start;
            vma->size = len;
        } else {
            vma = _auto_find_map_area(proc, len);
        }
    }
    return vma;
}
