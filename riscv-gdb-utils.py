from typing import Optional
import gdb
from enum import IntEnum

uint64_t = gdb.lookup_type('unsigned long long')
uint64_t_ptr = uint64_t.pointer()

class SatpMode(IntEnum):
    Bare = 0
    Sv39 = 8
    Sv48 = 9
    Sv57 = 10

PAGE_SHIFT = 12
ENTRIES_PER_PAGE_PAGE_TABLE = 512

class DumpPageTable(gdb.Command):
    def __init__(self) -> None:
        super().__init__('dump-pgtbl', gdb.COMMAND_USER)

    def invoke(self, args: str, _: bool) -> None:
        inferior = gdb.selected_inferior()
        if inferior.architecture().name() != 'riscv:rv64':
            raise gdb.GdbError(f'unsupported architecture: {inferior.architecture().name()}')

        args = args.strip().split()
        root_tbl_paddr = None

        if len(args) == 0 or args[0] == '-':
            frame = gdb.selected_frame()
            satp = int(frame.read_register('satp').cast(uint64_t))
            ppn = satp & 0xfff_ffff_ffff
            asid = (satp >> 44) & 0xff
            mode = SatpMode((satp >> 60) & 0xf)
            if mode != SatpMode.Sv39:
                if mode == SatpMode.Bare:
                    raise gdb.GdbError(f'satp.MODE is Bare')
                else:
                    raise gdb.GdbError(f'unsupported satp.MODE: {mode}')

            print(f'{satp=:#x}: {ppn=:#x} {asid=:#x} {mode.name=}')

            root_tbl_paddr = ppn << PAGE_SHIFT

            print(f'{root_tbl_paddr=:#x}')
        else:
            root_tbl_paddr = int(args[0], 16)

        if len(args) > 0:
            args.pop(0)

        search_for_vaddr = None if len(args) == 0 else int(args[0], 16)

        def walk(tbl_addr: int, level: int = 0, vpn_split: list[int] = []):
            assert level < 3

            if search_for_vaddr is None:
                print(f'{" " * level * 2}{tbl_addr:#x}:')

            i = 0
            for addr in range(tbl_addr, tbl_addr + ENTRIES_PER_PAGE_PAGE_TABLE * 8, 8):
                # entry = int.from_bytes(bytes(inferior.read_memory(addr, 8)), 'little')
                entry = gdb.execute(f'mon xp/x {addr:#x}', to_string=True).split(': ')[1].strip()
                try:
                    entry = int(entry, 16)
                except ValueError:
                    raise gdb.GdbError(entry)

                    
                if (entry & 1) != 0:
                    # valid bit set
                    ppn = (entry >> 10) & 0xfff_ffff_ffff
                    paddr = ppn << PAGE_SHIFT

                    permissions = ''
                    if (entry & 0b10) != 0:
                        permissions += 'r'
                    if (entry & 0b100) != 0:
                        permissions += 'w'
                    if (entry & 0b1000) != 0:
                        permissions += 'x'
                    if (entry & 0b10000) != 0:
                        permissions += 'u'

                    is_leaf = permissions != ''

                    if not is_leaf:
                        if search_for_vaddr is None:
                            print(f'{" " * level * 2}[{i:#05x}] {addr:#x}: {paddr:#x}')
                        walk(paddr, level + 1, vpn_split + [i])
                    else:
                        vpn = (vpn_split[0] << 18) | (vpn_split[1] << 9) | i
                        vaddr = vpn << PAGE_SHIFT
                        if search_for_vaddr == vaddr:
                            print(f'{vaddr:#x} -> {paddr:#x} ({permissions})')
                        elif search_for_vaddr is None:
                            print(f'{" " * level * 2}[{i:#05x}] {vaddr:#x} -> {paddr:#x} ({permissions})')

                i += 1

        walk(root_tbl_paddr)

INTERRUPT_MASK = 0x8000000000000001

class Mcause(IntEnum):
    # Interrupts
    SupervisorSoftwareInterrupt = INTERRUPT_MASK | 1
    MachineSoftwareInterrupt = INTERRUPT_MASK | 3
    SupervisorTimerInterrupt = INTERRUPT_MASK | 5
    MachineTimerInterrupt = INTERRUPT_MASK | 7
    SupervisorExternalInterrupt = INTERRUPT_MASK | 9
    MachineExternalInterrupt = INTERRUPT_MASK | 11

    # Exceptions
    InstructionAddressMisaligned = 0
    InstructionAccessFault = 1
    IllegalInstruction = 2
    Breakpoint = 3
    LoadAddressMisaligned = 4
    LoadAccessFault = 5
    StoreAddressMisaligned = 6
    StoreAccessFault = 7
    EnvironmentCallFromUMode = 8
    EnvironmentCallFromSMode = 9
    EnvironmentCallFromMMode = 11
    InstructionPageFault = 12
    LoadPageFault = 13
    StorePageFault = 15

class Scause(IntEnum):
    # Interrupts
    SupervisorSoftwareInterrupt = INTERRUPT_MASK | 1
    SupervisorTimerInterrupt = INTERRUPT_MASK | 5
    SupervisorExternalInterrupt = INTERRUPT_MASK | 9

    # Exceptions
    InstructionAddressMisaligned = 0
    InstructionAccessFault = 1
    IllegalInstruction = 2
    Breakpoint = 3
    LoadAddressMisaligned = 4
    LoadAccessFault = 5
    StoreAddressMisaligned = 6
    StoreAccessFault = 7
    EnvironmentCallFromUMode = 8
    EnvironmentCallFromSMode = 9
    InstructionPageFault = 12
    LoadPageFault = 13
    StorePageFault = 15


class PrintMcause(gdb.Command):
    def __init__(self) -> None:
        super().__init__('mcause', gdb.COMMAND_USER)

    def invoke(self, *_) -> None:
        inferior = gdb.selected_inferior()
        if inferior.architecture().name() != 'riscv:rv64':
            raise gdb.GdbError(f'unsupported architecture: {inferior.architecture().name()}')

        frame = gdb.selected_frame()
        mcause = Mcause(int(frame.read_register('mcause').cast(uint64_t)))
        print(f'mcause: {mcause.name} ({mcause.value:#x})')
        print(f'mepc: {frame.read_register("mepc").cast(uint64_t_ptr).format_string(symbols=True, styling=True)}')
        print(f'mtval: {frame.read_register("mtval").cast(uint64_t_ptr).format_string(symbols=True, styling=True)}')


class PrintScause(gdb.Command):
    def __init__(self) -> None:
        super().__init__('scause', gdb.COMMAND_USER)

    def invoke(self, *_) -> None:
        inferior = gdb.selected_inferior()
        if inferior.architecture().name() != 'riscv:rv64':
            raise gdb.GdbError(f'unsupported architecture: {inferior.architecture().name()}')

        frame = gdb.selected_frame()
        scause = Scause(int(frame.read_register('scause').cast(uint64_t)))
        print(f'scause: {scause.name} ({scause.value:#x})')
        print(f'sepc: {frame.read_register("sepc").cast(uint64_t_ptr).format_string(symbols=True, styling=True)}')
        print(f'stval: {frame.read_register("stval").cast(uint64_t_ptr).format_string(symbols=True, styling=True)}')


DumpPageTable()
PrintMcause()
PrintScause()
