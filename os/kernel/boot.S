    .section .text.entry
    .globl k_entry, boot_stack, boot_stack_top
mhartnum = 2
pv_offset = 0xFFFFFFFF00000000

k_entry: # 0x80020000
    la a0, boot_hartid
    li a1, 1
    amoadd.w a0, a1, (a0) # t=(a0),(a0)=t+a1,a0=t 通过原子操作软获取hartid存至a0

    la t0, k_pg_t
    srli t0, t0, 12
    li t1, 8<<60
    or t0, t0, t1
    csrw satp, t0
    sfence.vma
    la t0, go
    li t1, pv_offset
    add t0, t0, t1
    jr t0
go:
    la sp, boot_stack
    li t0, 4096
    mv t1, a0 # t1=hartid
    addi t1, t1, 1
    mul t0, t0, t1
    add sp, sp, t0
    la t0, osmain
    jr t0
loop:
    j loop

.section .data
.align 12
boot_stack:
    .zero 4096 * mhartnum
boot_stack_top:

.globl k_pg_t
.section .data
.align 12
k_pg_t: # 0xffff ffff 8000 0000 - 0x8000 0000
    .quad (0x00000 << 10) | 0xcf
    .quad (0x40000 << 10) | 0xcf
    .quad (0x80000 << 10) | 0xcf
    .zero 8 * 505
    # 设置倒数第二个个页表项，PPN=0x80000，标志位 VRWXAD 均为 1
    # 此时该页表为叶页表为1G大的超级页面
    .quad (0x00000 << 10) | 0xcf
    .quad (0x40000 << 10) | 0xcf
    .quad (0x80000 << 10) | 0xcf # VRWXAD
    .quad 0
boot_hartid:
    .word 0
mask:
    .quad 2