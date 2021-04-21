.macro PUSH_ALL
    csrrw sp, sscratch, sp # scratch保存内核栈指针，sp与scratch交换
    addi sp, sp, -37*8  # 此时sp切换为内核栈，栈开辟新空间，而scratch保存的是原sp

    sd ra, 0*8(sp)
    # user sp 后面再保存
    sd gp, 2*8(sp)
    sd tp, 3*8(sp)
    sd t0, 4*8(sp)
    sd t1, 5*8(sp)
    sd t2, 6*8(sp)
    sd s0, 7*8(sp)
    sd s1, 8*8(sp)
    sd a0, 9*8(sp)
    sd a1, 10*8(sp)
    sd a2, 11*8(sp)
    sd a3, 12*8(sp)
    sd a4, 13*8(sp)
    sd a5, 14*8(sp)
    sd a6, 15*8(sp)
    sd a7, 16*8(sp)
    sd s2, 17*8(sp)
    sd s3, 18*8(sp)
    sd s4, 19*8(sp)
    sd s5, 20*8(sp)
    sd s6, 21*8(sp)
    sd s7, 22*8(sp)
    sd s8, 23*8(sp)
    sd s9, 24*8(sp)
    sd s10, 25*8(sp)
    sd s11, 26*8(sp)
    sd t3, 27*8(sp)
    sd t4, 28*8(sp)
    sd t5, 29*8(sp)
    sd t6, 30*8(sp)

    csrrw s0, sscratch, zero # 将scratch与s0交换，并将scratch置为0用于防止中断嵌套
    csrr s1, sstatus
    csrr s2, sepc
    addi s3, sp, 37*8 # 保存内核态栈指针，即原scratch，用于恢复以及内核栈切换
    csrr s4, satp
    # 以上三项异常返回时可被修改，用于实现切换
    csrr s5, sbadaddr
    csrr s6, scause
    
    sd s0, 1*8(sp)
    sd s1, 31*8(sp)
    sd s2, 32*8(sp)
    sd s3, 33*8(sp)
    sd s4, 34*8(sp)
    sd s5, 35*8(sp)
    sd s6, 36*8(sp)
.endm
.macro POP_ALL
    # 先恢复状态寄存器，用于切换进程
    sd s1, 31*8(sp)
    sd s2, 32*8(sp)
    sd s3, 33*8(sp)
    sd s4, 34*8(sp)

    csrw sstatus, s1
    csrw sepc, s2
    csrw sscratch, s3
    csrw satp, s4

    ld ra, 0*8(sp)
    # sp 最后再恢复
    ld gp, 2*8(sp)
    ld tp, 3*8(sp)
    ld t0, 4*8(sp)
    ld t1, 5*8(sp)
    ld t2, 6*8(sp)
    ld s0, 7*8(sp)
    ld s1, 8*8(sp)
    ld a0, 9*8(sp)
    ld a1, 10*8(sp)
    ld a2, 11*8(sp)
    ld a3, 12*8(sp)
    ld a4, 13*8(sp)
    ld a5, 14*8(sp)
    ld a6, 15*8(sp)
    ld a7, 16*8(sp)
    ld s2, 17*8(sp)
    ld s3, 18*8(sp)
    ld s4, 19*8(sp)
    ld s5, 20*8(sp)
    ld s6, 21*8(sp)
    ld s7, 22*8(sp)
    ld s8, 23*8(sp)
    ld s9, 24*8(sp)
    ld s10, 25*8(sp)
    ld s11, 26*8(sp)
    ld t3, 27*8(sp)
    ld t4, 28*8(sp)
    ld t5, 29*8(sp)
    ld t6, 30*8(sp)
    ld sp, 1*8(sp) # 最后再恢复sp
.endm

.globl trap_entry, trap_ret
# 入口地址必须二字节对齐
.align(2)
trap_entry:
    PUSH_ALL
    mv a0, sp
    call trap_handler
trap_ret:
    POP_ALL
    sret
