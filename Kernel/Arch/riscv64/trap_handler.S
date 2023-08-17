.section .text.asm_trap_handler

#define XLEN 8

#define REGISTER_STATE_SIZE (34 * XLEN)
#if REGISTER_STATE_SIZE % 16 != 0
#    error "REGISTER_STATE_SIZE is not a multiple of 16 bytes!"
#endif

#define SSTATUS_SLOT        (31 * XLEN)
#define PC_SLOT             (32 * XLEN)
#define USERSPACE_SP_SLOT   (33 * XLEN)

.extern trap_handler

.align 2
.global asm_trap_handler
asm_trap_handler:
    .cfi_startproc
    // Save all register states to the current stack
    // and enter the C++ trap handler

    // Allocate stack space for Trap Frame
    addi sp, sp, -REGISTER_STATE_SIZE
    .cfi_adjust_cfa_offset REGISTER_STATE_SIZE

    sd x1, 0*XLEN(sp)
    .cfi_offset ra, -REGISTER_STATE_SIZE
    sd x2, 1*XLEN(sp)
    sd x3, 2*XLEN(sp)
    sd x4, 3*XLEN(sp)
    sd x5, 4*XLEN(sp)
    sd x6, 5*XLEN(sp)
    sd x7, 6*XLEN(sp)
    sd x8, 7*XLEN(sp)
    sd x9, 8*XLEN(sp)
    sd x10, 9*XLEN(sp)
    sd x11, 10*XLEN(sp)
    sd x12, 11*XLEN(sp)
    sd x13, 12*XLEN(sp)
    sd x14, 13*XLEN(sp)
    sd x15, 14*XLEN(sp)
    sd x16, 15*XLEN(sp)
    sd x17, 16*XLEN(sp)
    sd x18, 17*XLEN(sp)
    sd x19, 18*XLEN(sp)
    sd x20, 19*XLEN(sp)
    sd x21, 20*XLEN(sp)
    sd x22, 21*XLEN(sp)
    sd x23, 22*XLEN(sp)
    sd x24, 23*XLEN(sp)
    sd x25, 24*XLEN(sp)
    sd x26, 25*XLEN(sp)
    sd x27, 26*XLEN(sp)
    sd x28, 27*XLEN(sp)
    sd x29, 28*XLEN(sp)
    sd x30, 29*XLEN(sp)
    sd x31, 30*XLEN(sp)

    // Let's save some special registers
    csrr t0, sstatus
    sd t0, SSTATUS_SLOT(sp)
    csrr t0, sepc
    sd t0, PC_SLOT(sp)

    // Set up TrapFrame struct on the stack
    mv t0, sp
    addi sp, sp, -16
    .cfi_adjust_cfa_offset 16
    sd t0, 1*XLEN(sp)
    sd zero, 0*XLEN(sp)

    // Move stack pointer into first argument register
    // and jump to the C++ trap handler
    mv a0, sp

    call trap_handler

    // Remove TrapFrame from the stack
    addi sp, sp, 16
    .cfi_adjust_cfa_offset -16

    // Restore special registers first
    ld t0, SSTATUS_SLOT(sp)
    csrw sstatus, t0
    ld t0, PC_SLOT(sp)
    csrw sepc, t0

    ld x1, 0*XLEN(sp)
    .cfi_restore ra
    // sp
    ld x3, 2*XLEN(sp)
    ld x4, 3*XLEN(sp)
    ld x5, 4*XLEN(sp)
    ld x6, 5*XLEN(sp)
    ld x7, 6*XLEN(sp)
    ld x8, 7*XLEN(sp)
    ld x9, 8*XLEN(sp)
    ld x10, 9*XLEN(sp)
    ld x11, 10*XLEN(sp)
    ld x12, 11*XLEN(sp)
    ld x13, 12*XLEN(sp)
    ld x14, 13*XLEN(sp)
    ld x15, 14*XLEN(sp)
    ld x16, 15*XLEN(sp)
    ld x17, 16*XLEN(sp)
    ld x18, 17*XLEN(sp)
    ld x19, 18*XLEN(sp)
    ld x20, 19*XLEN(sp)
    ld x21, 20*XLEN(sp)
    ld x22, 21*XLEN(sp)
    ld x23, 22*XLEN(sp)
    ld x24, 23*XLEN(sp)
    ld x25, 24*XLEN(sp)
    ld x26, 25*XLEN(sp)
    ld x27, 26*XLEN(sp)
    ld x28, 27*XLEN(sp)
    ld x29, 28*XLEN(sp)
    ld x30, 29*XLEN(sp)
    ld x31, 30*XLEN(sp)

    // Restore sp last
    ld x2, 1*XLEN(sp)

    addi sp, sp, REGISTER_STATE_SIZE
    .cfi_adjust_cfa_offset -REGISTER_STATE_SIZE

    sret
    .cfi_endproc

.global restore_context
restore_context:
    mv a0, sp
    call exit_trap

    // Remove TrapFrame from the stack
    addi sp, sp, 16

    // Restore special registers first
    ld t0, SSTATUS_SLOT(sp)
    csrw sstatus, t0
    ld t0, PC_SLOT(sp)
    csrw sepc, t0

    ld x1, 0*XLEN(sp)
    // sp
    ld x3, 2*XLEN(sp)
    ld x4, 3*XLEN(sp)
    ld x5, 4*XLEN(sp)
    ld x6, 5*XLEN(sp)
    ld x7, 6*XLEN(sp)
    ld x8, 7*XLEN(sp)
    ld x9, 8*XLEN(sp)
    ld x10, 9*XLEN(sp)
    ld x11, 10*XLEN(sp)
    ld x12, 11*XLEN(sp)
    ld x13, 12*XLEN(sp)
    ld x14, 13*XLEN(sp)
    ld x15, 14*XLEN(sp)
    ld x16, 15*XLEN(sp)
    ld x17, 16*XLEN(sp)
    ld x18, 17*XLEN(sp)
    ld x19, 18*XLEN(sp)
    ld x20, 19*XLEN(sp)
    ld x21, 20*XLEN(sp)
    ld x22, 21*XLEN(sp)
    ld x23, 22*XLEN(sp)
    ld x24, 23*XLEN(sp)
    ld x25, 24*XLEN(sp)
    ld x26, 25*XLEN(sp)
    ld x27, 26*XLEN(sp)
    ld x28, 27*XLEN(sp)
    ld x29, 28*XLEN(sp)
    ld x30, 29*XLEN(sp)
    ld x31, 30*XLEN(sp)

    // FIXME: don't use t0 and t1
    ld t0, PC_SLOT(sp)
    ld t1, USERSPACE_SP_SLOT(sp)

    addi sp, sp, REGISTER_STATE_SIZE

    // Restore sp last
    mv sp, t1

    jr t0
