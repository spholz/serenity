.section .text

.global start
.type start, @function
start:
    // Clear sstatus.SIE, which disables all interrupts in supervisor mode.
    csrci sstatus, 1 << 1

    // Also, disable all interrupts sources and mark them as non-pending.
    csrw sie, zero
    csrw sip, zero

    // TODO: maybe load the gp register here?

    // Clear the BSS.
    lla t0, start_of_bss
    lla t1, end_of_bss
    bgeu t0, t1, .Lclear_bss_done
.Lclear_bss_loop:
    sd zero, (t0)
    addi t0, t0, 8
    bltu t0, t1, .Lclear_bss_loop
.Lclear_bss_done:

    // Save the arguments passed by the firmware.
    addi sp, sp, -16
    sd a0, 0(sp)
    sd a1, 8(sp)

    mv a2, a1                        // Pass the EFI_SYSTEM_TABLE so we can print an error message if self-relocation fails.
    lla a0, start_of_prekernel_image // This symbol is defined in the linker script.
    lla a1, _DYNAMIC                 // And this symbol is automatically defined by the linker.
    call perform_relative_relocations

    ld a1, 8(sp)
    ld a0, 0(sp)
    addi sp, sp, 16

    call init
