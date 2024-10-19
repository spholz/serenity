/*
 * Copyright (c) 2021, Nico Weber <thakis@chromium.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// In a specially-named text section so that the linker script can put it first in .text.
.section ".text.first"

// Make the kernel bootable using U-Boot's booti command by adding a linux header (https://www.kernel.org/doc/html/latest/arch/arm64/booting.html#call-the-kernel-image).
// Booting via booti causes U-Boot to pass us the boot hart ID and a flattened devicetree.
linux_header:
  b start                                   // u32 code0
  b start                                   // u32 code1

  // This offset is needed, as otherwise U-Boot will try to load us at the same address where OpenSBI is loaded.
  // The value is the same that Linux uses.
  .dword 0x0                                // u64 text_offset

  .dword end_of_kernel_image - linux_header // u64 image_size
  .dword 0                                  // u64 flags
  .dword 0                                  // u32 res2
  .dword 0                                  // u64 res3
  .dword 0                                  // u64 res4
  .ascii "ARM\x64"                          // u32 magic
  .word 0                                   // u32 res5

// We expect that x0 contains a pointer to a flattened devicetree,
// which is also what the AArch64 Linux boot protocol requires.

.global start
.type start, @function
start:
  // Let only core 0 continue, put other cores to sleep.
  mrs x13, MPIDR_EL1
  and x13, x13, 0xff
  cbnz x13, halt

  // Let stack start before .text for now.
  // 512 kiB (0x80000) of stack are probably not sufficient, especially once we give the other cores some stack too,
  // but for now it's ok.
  adrp x14, start
  add x14, x14, :lo12:start
  mov sp, x14

  // Clear BSS.
  adrp x14, start_of_bss
  add x14, x14, :lo12:start_of_bss
  ldr x15, =size_of_bss_divided_by_8
Lbss_clear_loop:
  str xzr, [x14], #8
  subs x15, x15, #1
  bne Lbss_clear_loop

  b pre_init

halt:
  msr daifset, #2
  wfi
  b halt
