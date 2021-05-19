.globl direct_switch, _move_switch
direct_switch: # void direct_switch(proc_context *oldp, pro_context *newp);
    sd ra, 0*8(a0)
    sd sp, 1*8(a0)
    sd s0, 2*8(a0)
    sd s1, 3*8(a0)
    sd s2, 4*8(a0)
    sd s3, 5*8(a0)
    sd s4, 6*8(a0)
    sd s5, 7*8(a0)
    sd s6, 8*8(a0)
    sd s7, 9*8(a0)
    sd s8, 10*8(a0)
    sd s9, 11*8(a0)
    sd s10, 12*8(a0)
    sd s11, 13*8(a0)

    ld ra, 0*8(a1)
    ld sp, 1*8(a1)
    ld s0, 2*8(a1)
    ld s1, 3*8(a1)
    ld s2, 4*8(a1)
    ld s3, 5*8(a1)
    ld s4, 6*8(a1)
    ld s5, 7*8(a1)
    ld s6, 8*8(a1)
    ld s7, 9*8(a1)
    ld s8, 10*8(a1)
    ld s9, 11*8(a1)
    ld s10, 12*8(a1)
    ld s11, 13*8(a1)
    ret # return to newp>ra

#a0  void *pcontext,
#a1  void *proc_lock
#a2  void *list_lock
#a3  void *ready_list
#a4  void *status_list
#a5  void *schedcontext
_move_switch: # 先保存，再入队，再解锁
    sd ra, 0*8(a0)    # save process
    sd sp, 1*8(a0)
    sd s0, 2*8(a0)
    sd s1, 3*8(a0)
    sd s2, 4*8(a0)
    sd s3, 5*8(a0)
    sd s4, 6*8(a0)
    sd s5, 7*8(a0)
    sd s6, 8*8(a0)
    sd s7, 9*8(a0)
    sd s8, 10*8(a0)
    sd s9, 11*8(a0)
    sd s10, 12*8(a0)
    sd s11, 13*8(a0)

    addi sp, sp, -48
    sd a0, 0*8(sp)
    sd a1, 1*8(sp)
    sd a2, 2*8(sp)
    sd a3, 3*8(sp)
    sd a4, 4*8(sp)
    sd a5, 5*8(sp)

    ld a0, 2*8(sp)
    call lock        # lock list_lock

    ld a0, 3*8(sp)
    ld a1, 4*8(sp)
    call _add_before # add current process to ready list

    ld a0, 1*8(sp)
    call unlock      # unlock process

    ld a0, 2*8(sp)
    call unlock      # unlock list_lock
    
    
    ld a0, 5*8(sp)   # switch to scheduler
    addi sp, sp, 48  # balance stack

    ld ra, 0*8(a0)
    ld sp, 1*8(a0)
    ld s0, 2*8(a0)
    ld s1, 3*8(a0)
    ld s2, 4*8(a0)
    ld s3, 5*8(a0)
    ld s4, 6*8(a0)
    ld s5, 7*8(a0)
    ld s6, 8*8(a0)
    ld s7, 9*8(a0)
    ld s8, 10*8(a0)
    ld s9, 11*8(a0)
    ld s10, 12*8(a0)
    ld s11, 13*8(a0)
    ret