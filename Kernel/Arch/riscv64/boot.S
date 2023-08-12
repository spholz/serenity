// In a specially-named text section so that the linker script can put it first in .text.
.section ".text.first"

.global start
.type start, @function
start:
  // Clear SIE (Supervisor Interrupt Enable)
  csrci sstatus, 1 << 1
  // Also, disable all interrupts and mark them as non-pending
  csrw sie, zero
  csrw sip, zero

  // Load the value of gp (Global Pointer) which is defined in the linker script and has to be loaded during startup
  // for gp-based linker relaxation and has to always point to the same location in memory.
.option push
.option norelax
  la gp, __global_pointer$
.option pop

  // Clear BSS.
  lla t0, start_of_bss
  lla t1, end_of_bss
  bgeu t0, t1, .Lclear_bss.done
.Lclear_bss.loop:
  sd zero, (t0)
  addi t0, t0, 8
  bltu t0, t1, .Lclear_bss.loop
.Lclear_bss.done:

  // Let stack start before .text for now.
  lla sp, start

  mv fp, zero
  mv ra, zero

  tail pre_init
