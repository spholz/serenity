.section .text

.global start
.type start, @function
start:
    // Save the arguments passed by the firmware.
    pushq %rcx
    pushq %rdx

    // Leave EFI_SYSTEM_TABLE in rdx, as that is the third argument in the System V amd64 ABI.
    // So we can print an error message if self-relocation fails.
    lea start_of_prekernel_image(%rip), %rdi  // This symbol is defined in the linker script.
    lea _DYNAMIC(%rip), %rsi                  // And this symbol is automatically defined by the linker.
    call perform_relative_relocations

    popq %rdx
    popq %rcx

    call init
