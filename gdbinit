# Kernel at physical load address...
add-symbol-file Build/riscv64/Kernel/Kernel.debug 0x80200000
# ... and at the final remapped virtual address
add-symbol-file Build/riscv64/Kernel/Kernel.debug

# Kernel
break pre_init
break __panic
break trap_handler
break trap_handler_nommu

source Meta/serenity_gdb.py
source riscv-gdb-utils.py

continue
