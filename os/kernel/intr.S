.macro PUSH_ALL
    csrrw a0, sscratch, a0 # scratch保存指向cpu结构体的指针，a0与scratch交换
    sd ra, 0*8(a0)
    sd sp, 1*8(a0)
    sd gp, 2*8(a0)
    sd tp, 3*8(a0)
    sd t0, 4*8(a0)
    sd t1, 5*8(a0)
    sd t2, 6*8(a0)
    sd s0, 7*8(a0)
    sd s1, 8*8(a0)
    # sd a0, 9*8(sp)
    sd a1, 10*8(a0)
    sd a2, 11*8(a0)
    sd a3, 12*8(a0)
    sd a4, 13*8(a0)
    sd a5, 14*8(a0)
    sd a6, 15*8(a0)
    sd a7, 16*8(a0)
    sd s2, 17*8(a0)
    sd s3, 18*8(a0)
    sd s4, 19*8(a0)
    sd s5, 20*8(a0)
    sd s6, 21*8(a0)
    sd s7, 22*8(a0)
    sd s8, 23*8(a0)
    sd s9, 24*8(a0)
    sd s10, 25*8(a0)
    sd s11, 26*8(a0)
    sd t3, 27*8(a0)
    sd t4, 28*8(a0)
    sd t5, 29*8(a0)
    sd t6, 30*8(a0)

    mv s0, a0
    csrrw s0, sscratch, s0 # sscratch与s0交换，即原a0的值保存至s0，而sscratch恢复a0即原值
    csrr s1, sstatus
    csrr s2, sepc
    # 以上三项异常返回时可被修改，用于实现切换
    csrr s3, sbadaddr
    csrr s4, scause
    
    sd s0, 9*8(a0) # 保存原a0

    sd s1, 31*8(a0)
    sd s2, 32*8(a0)
    sd s3, 33*8(a0)
    sd s4, 34*8(a0)
.endm
.macro POP_ALL
    # 先恢复状态寄存器，用于切换进程
    csrr a0, sscratch
    ld s1, 31*8(a0) # sstatus
    ld s2, 32*8(a0) # sepc

    csrw sstatus, s1
    csrw sepc, s2

    ld ra, 0*8(a0)
    ld sp, 1*8(a0)
    ld gp, 2*8(a0)
    ld tp, 3*8(a0)
    ld t0, 4*8(a0)
    ld t1, 5*8(a0)
    ld t2, 6*8(a0)
    ld s0, 7*8(a0)
    ld s1, 8*8(a0)
    # ld a0, 9*8(sp)
    ld a1, 10*8(a0)
    ld a2, 11*8(a0)
    ld a3, 12*8(a0)
    ld a4, 13*8(a0)
    ld a5, 14*8(a0)
    ld a6, 15*8(a0)
    ld a7, 16*8(a0)
    ld s2, 17*8(a0)
    ld s3, 18*8(a0)
    ld s4, 19*8(a0)
    ld s5, 20*8(a0)
    ld s6, 21*8(a0)
    ld s7, 22*8(a0)
    ld s8, 23*8(a0)
    ld s9, 24*8(a0)
    ld s10, 25*8(a0)
    ld s11, 26*8(a0)
    ld t3, 27*8(a0)
    ld t4, 28*8(a0)
    ld t5, 29*8(a0)
    ld t6, 30*8(a0)
    ld a0, 9*8(a0) # 最后再恢复a0
.endm

.globl trap_entry, trap_ret
# 入口地址必须低2位对齐
.align(2)
trap_entry:
    PUSH_ALL
    ld sp, 35*8(a0) # 加载内核栈
    call trap_handler
trap_ret:
    POP_ALL
    sret
