from argparse import ArgumentParser
from elftools.elf.elffile import ELFFile
import gdb


class AutoMap(gdb.Command):
    def __init__(self) -> None:
        super().__init__('automap', gdb.COMMAND_USER, gdb.COMPLETE_FILENAME)

    def invoke(self, argument: str, from_tty: bool) -> None:
        parser = ArgumentParser(prog='automap')
        parser.add_argument('elf_file')
        parser.add_argument('-s', '--needle-size', type=int, default=100)
        parser.add_argument('-a', '--address', type=lambda x: int(x, 16), default=gdb.selected_frame().pc())

        args = parser.parse_args(argument.strip().split())

        with open(args.elf_file, 'rb') as f:
            elf = ELFFile(f)

            inferior = gdb.selected_inferior()

            needle = inferior.read_memory(args.address, args.needle_size)

            for segment in elf.iter_segments(type='PT_LOAD'):
                offset = segment.data().find(needle)
                if offset == -1:
                    continue

                elf_address = segment.header.p_vaddr + offset
                base_address = args.address - elf_address
                print(f'found, {elf_address=:#x} => {base_address=:#x}')
                gdb.execute(f'add-symbol-file {args.elf_file} -o {base_address:#x}')
                return

            print('not found :^(')


if __name__ == '__main__':
    AutoMap()
