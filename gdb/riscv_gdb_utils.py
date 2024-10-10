import gdb
import psutil

from enum import IntEnum, IntFlag

uint64_t = gdb.lookup_type('unsigned long long')
uint64_t_ptr = uint64_t.pointer()


class SatpMode(IntEnum):
    Bare = 0
    Sv39 = 8
    Sv48 = 9
    Sv57 = 10


class PteFlags(IntFlag):
    Valid = 1 << 0,
    Readable = 1 << 1,
    Writeable = 1 << 2,
    Executable = 1 << 3,
    UserAllowed = 1 << 4,
    Global = 1 << 5,
    Accessed = 1 << 6,
    Dirty = 1 << 7,
    Reserved0 = 1 << 8,
    Reserved1 = 1 << 9,


PAGE_SHIFT = 12
ENTRIES_PER_PAGE_PAGE_TABLE = 512


def get_proc_owning_port(port: int) -> psutil.Process:
    for proc in psutil.process_iter():
        try:
            for conn in proc.connections():
                if conn.status == 'LISTEN' and conn.laddr.port == port:
                    return proc
        except Exception:
            continue
    assert False


def read_physical_memory(addr: int, conn_name) -> int:
    if 'qemu' in conn_name:
        val = gdb.execute(f'mon xp/x {addr:#x}', to_string=True).split(': ')[1].strip()
        try:
            return int(val, 16)
        except ValueError:
            raise gdb.GdbError(f'error while trying to read memory at {addr:#x}: {val}')
    elif 'openocd' in conn_name:
        val = gdb.execute(f'mon read_memory {addr:#x} 64 1 phys', to_string=True).strip()
        try:
            return int(val, 16)
        except ValueError:
            raise gdb.GdbError(f'error while trying to read memory at {addr:#x}: {val}')
    else:
        return int.from_bytes(bytes(gdb.selected_inferior().read_memory(addr, 8)), 'little')


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
                    raise gdb.GdbError('satp.MODE is Bare')
                else:
                    raise gdb.GdbError(f'unsupported satp.MODE: {mode}')

            print(f'{satp=:#x}: {ppn=:#x} {asid=:#x} {mode.name=}')

            root_tbl_paddr = ppn << PAGE_SHIFT

            print(f'{root_tbl_paddr=:#x}')
        else:
            root_tbl_paddr = int(args[0], 16)

        if len(args) > 0:
            args.pop(0)

        search_for_vaddr = None if len(args) == 0 else int(gdb.parse_and_eval(args[0]))

        port = int(gdb.selected_inferior().connection.details.split(':')[1])
        conn_name = get_proc_owning_port(port).name()

        def walk(tbl_addr: int, level: int = 0, vpn_split: list[int] = [], trace: list[int] = []):
            if level >= 3:
                raise gdb.GdbError('page table recursion limit exceeded!')

            if search_for_vaddr is None:
                print(f'{" " * level * 2}table @ {tbl_addr:#x}:')

            i = 0
            for addr in range(tbl_addr, tbl_addr + ENTRIES_PER_PAGE_PAGE_TABLE * 8, 8):
                entry = read_physical_memory(addr, conn_name)

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
                            print(f'{" " * level * 2}[{i:#05x}] {addr:#x}: next level: {paddr:#x}')
                        walk(paddr, level + 1, vpn_split + [i], trace + [addr])
                    else:
                        vpn = (vpn_split[0] << 18) | (vpn_split[1] << 9) | i
                        vaddr = vpn << PAGE_SHIFT
                        if search_for_vaddr == vaddr:
                            print(f'{vaddr:#x} -> {paddr:#x} ({permissions})')
                            print(f'vpn_split: {", ".join(map(hex, vpn_split + [i]))} -> vpn: {vpn:#x}')
                            print(f'trace: {", ".join(map(hex, trace + [addr]))}')
                            return
                        elif search_for_vaddr is None:
                            print(
                                f'{" " * level * 2}[{i:#05x}] {addr:#x}: leaf: {vaddr:#x} -> {paddr:#x} ({permissions})'
                            )

                i += 1

        walk(root_tbl_paddr)


INTERRUPT_MASK = 0x8000000000000000


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


class PrivilegeLevel(IntEnum):
    User = 0
    Supervisor = 1
    Reserved = 2
    Machine = 3


class PrintMcause(gdb.Command):
    def __init__(self) -> None:
        super().__init__('mcause', gdb.COMMAND_USER)

    def invoke(self, *_) -> None:
        inferior = gdb.selected_inferior()
        if inferior.architecture().name() != 'riscv:rv64':
            raise gdb.GdbError(f'unsupported architecture: {inferior.architecture().name()}')

        frame = gdb.selected_frame()
        mcause = Mcause(int(frame.read_register('mcause').cast(uint64_t)))
        mstatus = int(frame.read_register('mstatus').cast(uint64_t))
        mpp = PrivilegeLevel((mstatus >> 11) & 0b11)
        print(f'mcause: {mcause.name} ({mcause.value:#x})')
        print(f'mepc: {frame.read_register("mepc").cast(uint64_t_ptr).format_string(symbols=True, styling=True)}')
        print(f'mtval: {frame.read_register("mtval").cast(uint64_t_ptr).format_string(symbols=True, styling=True)}')
        print(f'mstatus.MPP: {mpp.name}')


class PrintScause(gdb.Command):
    def __init__(self) -> None:
        super().__init__('scause', gdb.COMMAND_USER)

    def invoke(self, *_) -> None:
        inferior = gdb.selected_inferior()
        if inferior.architecture().name() != 'riscv:rv64':
            raise gdb.GdbError(f'unsupported architecture: {inferior.architecture().name()}')

        frame = gdb.selected_frame()
        scause = Scause(int(frame.read_register('scause').cast(uint64_t)))
        sstatus = int(frame.read_register('sstatus').cast(uint64_t))
        spp = PrivilegeLevel((sstatus >> 8) & 0b1)
        print(f'scause: {scause.name} ({scause.value:#x})')
        print(f'sepc: {frame.read_register("sepc").cast(uint64_t_ptr).format_string(symbols=True, styling=True)}')
        print(f'stval: {frame.read_register("stval").cast(uint64_t_ptr).format_string(symbols=True, styling=True)}')
        print(f'sstatus.SPP: {spp.name}')


class TranslateVAddr(gdb.Command):
    def __init__(self) -> None:
        super().__init__('translate-vaddr', gdb.COMMAND_USER)

    def invoke(self, args: str, _: bool) -> None:
        inferior = gdb.selected_inferior()
        if inferior.architecture().name() != 'riscv:rv64':
            raise gdb.GdbError(f'unsupported architecture: {inferior.architecture().name()}')

        args = args.strip().split()
        root_tbl_paddr = None

        if len(args) < 1 or len(args) > 2:
            raise gdb.GdbError('invalid arguments')

        vaddr = int(gdb.parse_and_eval(args[0]))
        args.pop(0)

        if len(args) == 1:
            root_tbl_paddr = int(args[0], 16)
        else:
            frame = gdb.selected_frame()
            satp = int(frame.read_register('satp').cast(uint64_t))
            ppn = satp & 0xfff_ffff_ffff
            asid = (satp >> 44) & 0xff
            mode = SatpMode((satp >> 60) & 0xf)
            if mode != SatpMode.Sv39:
                if mode == SatpMode.Bare:
                    raise gdb.GdbError('satp.MODE is Bare')
                else:
                    raise gdb.GdbError(f'unsupported satp.MODE: {mode}')

            print(f'{satp=:#x}: {ppn=:#x} {asid=:#x} {mode.name=}')

            root_tbl_paddr = ppn << PAGE_SHIFT

            print(f'{root_tbl_paddr=:#x}')

        LEVELS = 3

        vpn = vaddr >> 12
        page_offset = vaddr & 0xfff
        vpn_split = ((vaddr >> 12) & 0x1ff, (vaddr >> 21) & 0x1ff, (vaddr >> 30) & 0x1ff)

        print(f'VPN: {vpn:#x} -> VPN[]: {", ".join(map(hex, vpn_split))}')

        port = int(gdb.selected_inferior().connection.details.split(':')[1])
        conn_name = get_proc_owning_port(port).name()

        level = LEVELS - 1
        current_table = root_tbl_paddr
        while True:
            entry_addr = current_table + (vpn_split[level] * 8)
            entry = read_physical_memory(entry_addr, conn_name)

            print(f'PTE @ {entry_addr:#x}:')

            pte_flags = PteFlags(entry & 0x3ff)
            print(f'flags: {pte_flags.name}')

            if PteFlags.Valid not in pte_flags:
                raise gdb.GdbError('not mapped')

            if PteFlags.Reserved0 in pte_flags or PteFlags.Reserved1 in pte_flags:
                raise gdb.GdbError('reserved PTE field used')

            permission_bits = pte_flags & (PteFlags.Readable | PteFlags.Writeable | PteFlags.Executable)
            if PteFlags.Writeable in pte_flags and PteFlags.Readable not in pte_flags:
                raise gdb.GdbError(f'reserved PTE permission bit combination used: {permission_bits.name}')

            ppn = ((entry >> 10) & 0xfff_ffff_ffff)

            if int(permission_bits) != 0:
                # leaf PTE
                print(f'{vaddr:#x} -> {(ppn << 12) + page_offset:#x}')
                break
            else:
                # non-leaf PTE
                reserved_flags = pte_flags & (PteFlags.Dirty | PteFlags.Accessed | PteFlags.UserAllowed)
                if int(reserved_flags) != 0:
                    raise gdb.GdbError(f'reserved flag(s) for non-leaf PTEs used: {reserved_flags.name}')

                level -= 1

                if level < 0:
                    raise gdb.GdbError('page table recursion limit exceeded')

                current_table = ppn << 12


if __name__ == '__main__':
    DumpPageTable()
    PrintMcause()
    PrintScause()
    TranslateVAddr()
