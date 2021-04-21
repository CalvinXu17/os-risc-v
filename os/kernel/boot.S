    .section .text.entry
    .globl k_entry, kernel_stack, kernel_stack_top
mhartnum = 1
pv_offset = 0xFFFFFFFF00000000

k_entry: # 0x80200000
    la t4, kstart
    lui t0, %hi(k_pg_t)
    li t1, pv_offset
    sub t0, t0, t1
    srli t0, t0, 12
    li t2, 8<<60
    or t0, t0, t2
    csrw satp, t0
    sfence.vma

    lui t0, %hi(go)
    addi t0, t0, %lo(go)
    jr t0
go:
    la sp, kernel_stack_top
    csrw sscratch, sp
    la t0, kstart
    mv a0, t4
    jr t0
loop:
    j loop

.section .data
.align 12
kernel_stack:
    .zero 4096 * 8 * mhartnum
kernel_stack_top:

.globl k_pg_t
.section .data
.align 12
k_pg_t: # 0xffff ffff 8000 0000 - 0x8000 0000
    .zero 8 * 510
    # 设置倒数第二个个页表项，PPN=0x80000，标志位 VRWXAD 均为 1
    # 此时该页表为叶页表为1G大的超级页面
    .quad (0x80000 << 10) | 0xcf # VRWXAD
    .zero 8
