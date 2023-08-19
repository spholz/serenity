// In a specially-named text section so that the linker script can put it first in .text.
.section ".text.first"

.global start
.type start, @function
start:
  // Clear BSS.
  lla t0, start_of_bss
  lla t1, end_of_bss
  bgeu t0, t1, .Lclear_bss.done
.Lclear_bss.loop:
  sd zero, (t0)
  addi t0, t0, 8
  bltu t0, t1, .Lclear_bss.loop
.Lclear_bss.done:

  lla sp, start
  mv fp, zero
  mv ra, zero

  tail pre_init
