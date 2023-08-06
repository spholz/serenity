# # Kernel at physical load address...
# add-symbol-file Build/riscv64/Kernel/Kernel.debug 0x80200000
# # ... and at the final remapped virtual address
# add-symbol-file Build/riscv64/Kernel/Kernel.debug

# Kernel at physical load address...
add-symbol-file Build/riscv64/Kernel/Kernel 0x80200000
# ... and at the final remapped virtual address
add-symbol-file Build/riscv64/Kernel/Kernel

# Kernel
# break pre_init
break __panic
# break trap_handler
break Kernel::dump_registers
break trap_handler_nommu

source Meta/serenity_gdb.py
source riscv-gdb-utils.py

set confirm off
directory Build/riscv64/
set confirm on
set print frame-arguments none
set print asm-demangle on

continue
