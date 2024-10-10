import gdb
import gdb.unwinder

from gdb.unwinder import Unwinder
from gdb.FrameDecorator import FrameDecorator

from types import SimpleNamespace

flatptr_type = gdb.lookup_type('FlatPtr')
void_ptr_type = gdb.lookup_type('void').pointer()
trap_frame_ptr_type = gdb.lookup_type('Kernel::TrapFrame').pointer()


class SerenityRISCVTrapUnwinder(Unwinder):
    def __init__(self):
        super().__init__('Serenity RISC-V Trap Unwinder')

    def __call__(self, pending_frame: gdb.PendingFrame):
        pc = int(pending_frame.read_register('pc'))

        # why can't you get symbol names and only functions????
        # sym = gdb.current_progspace().block_for_pc(pc).function

        # if not gdb.execute(f'info symbol {pc:#x}', to_string=True).startswith('asm_trap_handler'):
        #     return None

        if pending_frame.name() != 'asm_trap_handler':
            return None

        sp = pending_frame.read_register('sp').cast(flatptr_type)
        register_state = sp.cast(trap_frame_ptr_type)['regs']

        unwind_info = pending_frame.create_unwind_info(SimpleNamespace(sp=sp, pc=pc))
        unwind_info.add_saved_register('sstatus', register_state['sstatus'])
        unwind_info.add_saved_register('pc', register_state['sepc'])

        unwind_info.add_saved_register('ra', register_state['x'][0])
        unwind_info.add_saved_register('sp', register_state['x'][1])
        unwind_info.add_saved_register('gp', register_state['x'][2])
        unwind_info.add_saved_register('tp', register_state['x'][3])
        unwind_info.add_saved_register('t0', register_state['x'][4])
        unwind_info.add_saved_register('t1', register_state['x'][5])
        unwind_info.add_saved_register('t2', register_state['x'][6])
        unwind_info.add_saved_register('fp', register_state['x'][7])
        unwind_info.add_saved_register('s1', register_state['x'][8])
        unwind_info.add_saved_register('a0', register_state['x'][9])
        unwind_info.add_saved_register('a1', register_state['x'][10])
        unwind_info.add_saved_register('a2', register_state['x'][11])
        unwind_info.add_saved_register('a3', register_state['x'][12])
        unwind_info.add_saved_register('a4', register_state['x'][13])
        unwind_info.add_saved_register('a5', register_state['x'][14])
        unwind_info.add_saved_register('a6', register_state['x'][15])
        unwind_info.add_saved_register('a7', register_state['x'][16])
        unwind_info.add_saved_register('s2', register_state['x'][17])
        unwind_info.add_saved_register('s3', register_state['x'][18])
        unwind_info.add_saved_register('s4', register_state['x'][19])
        unwind_info.add_saved_register('s5', register_state['x'][20])
        unwind_info.add_saved_register('s6', register_state['x'][21])
        unwind_info.add_saved_register('s7', register_state['x'][22])
        unwind_info.add_saved_register('s8', register_state['x'][23])
        unwind_info.add_saved_register('s9', register_state['x'][24])
        unwind_info.add_saved_register('s10', register_state['x'][25])
        unwind_info.add_saved_register('s11', register_state['x'][26])
        unwind_info.add_saved_register('t3', register_state['x'][27])
        unwind_info.add_saved_register('t4', register_state['x'][28])
        unwind_info.add_saved_register('t5', register_state['x'][29])
        unwind_info.add_saved_register('t6', register_state['x'][30])

        return unwind_info


# Does this have to be this ugly??
class TrapDecorator(FrameDecorator):
    def __init__(self, fobj):
        super(TrapDecorator, self).__init__(fobj)

    def function(self):
        frame = self.inferior_frame()
        name = str(frame.name())

        if name == 'asm_trap_handler':
            return f'--- TRAP --- {name}'

        return super().function()

    def frame_args(self):
        frame = self.inferior_frame()
        name = str(frame.name())

        if name != 'asm_trap_handler':
            return super().frame_args()

        class Arg:
            def __init__(self, symbol, value):
                self._symbol = symbol
                self._value = value

            def symbol(self):
                return self._symbol

            def value(self):
                return self._value

        sp = frame.read_register('sp').cast(flatptr_type)
        register_state = sp.cast(trap_frame_ptr_type)['regs']

        return [
            # cast to void* so gdb prints them as hex
            Arg('scause', register_state['scause'].cast(void_ptr_type)),
            Arg('stval', register_state['stval'].cast(void_ptr_type)),
            Arg('sstatus', register_state['sstatus'].cast(void_ptr_type)),
        ]


class TrapFilter:
    def __init__(self):
        self.name = 'TrapFilter'
        self.priority = 100
        self.enabled = True
        gdb.frame_filters[self.name] = self

    def filter(self, frame_iter):
        return map(TrapDecorator, frame_iter)


if __name__ == '__main__':
    gdb.unwinder.register_unwinder(None, SerenityRISCVTrapUnwinder(), replace=True)
    TrapFilter()
