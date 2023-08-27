// In a specially-named text section so that the linker script can put it first in .text.
.section ".text.first"

// Make the kernel bootable using U-Boot's booti command
linux_header:
.option push
.option norvc
  j start                          // u32 code0
  j start                          // u32 code1
  .dword 1 * 1024 * 1024           // u64 text_offset

  // u64 image_size
  .dword end_of_kernel_image - start_of_kernel_image

  .dword 0                         // u64 flags
  .word 2                          // u32 version
  .word 0                          // u32 res1
  .dword 0                         // u64 res2
  .dword 0                         // u64 res3
  .ascii "RSC\x5"                  // u32 magic
  .word 0                          // u32 res4
.option pop

.section ".text.first"

.global start
.type start, @function
start:
  // Clear SIE (disables all interrupts in supervisor mode)
  csrci sstatus, 1 << 1
  // Also, disable all interrupts sources and mark them as non-pending.
  csrw sie, zero
  csrw sip, zero

  // Disable the FPU, vector extension and additional stateful user-mode extensions.
  li t0, (0b11 << 15) | (0b11 << 13) | (0b11 << 9)
  csrc sstatus, t0

  // Load the value of gp (Global Pointer) which is defined in the linker script and has to be loaded during startup
  // for gp-based linker relaxation and has to always point to the same location in memory.
.option push
.option norelax
  // lla gp, __global_pointer$
.option pop

  // Clear BSS.
  lla t0, start_of_bss
  lla t1, end_of_bss
  bgeu t0, t1, Lclear_bss_done
Lclear_bss_loop:
  sd zero, (t0)
  addi t0, t0, 8
  bltu t0, t1, Lclear_bss_loop
Lclear_bss_done:

  // Let stack start before .text for now.
  lla sp, start

  csrw sscratch, zero

  // lla t0, end_of_kernel_image
  // li t1, 512 * 1024
  // add sp, t0, t1

  mv fp, zero
  mv ra, zero

  tail pre_init
