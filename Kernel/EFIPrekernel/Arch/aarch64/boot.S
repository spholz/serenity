.section .text

.global start
.type start, @function
start:
    // Clear the BSS.
    adrp x14, start_of_bss
    add x14, x14, :lo12:start_of_bss
    ldr x15, =size_of_bss_divided_by_8
.Lbss_clear_loop:
    str xzr, [x14], #8
    subs x15, x15, #1
    bne .Lbss_clear_loop

    // Save the arguments passed by the firmware.
    sub sp, sp, #16
    stp x0, x1, [sp, #0]

    mov x2, x1                                 // Pass the EFI_SYSTEM_TABLE so we can print an error message if self-relocation fails.

    adrp x0, start_of_prekernel_image          // This symbol is defined in the linker script.
    add x0, x0, :lo12:start_of_prekernel_image

    adrp x1, _DYNAMIC                          // And this symbol is automatically defined by the linker.
    add x1, x2, :lo12:_DYNAMIC

    bl perform_relative_relocations

    ldp x0, x1, [sp, #0]
    add sp, sp, #16

    b init
