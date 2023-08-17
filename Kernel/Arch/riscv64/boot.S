// In a specially-named text section so that the linker script can put it first in .text.
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

  // lla t0, end_of_kernel_image
  // li t1, 512 * 1024
  // add sp, t0, t1

  mv fp, zero
  mv ra, zero

  tail pre_init
