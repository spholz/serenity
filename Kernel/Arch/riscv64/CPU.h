#include <AK/Forward.h>

namespace Kernel {

void dbgln_without_mmu(StringView message);
[[noreturn]] void panic_without_mmu(StringView message);

namespace Memory {
void init_page_tables();
}
}
