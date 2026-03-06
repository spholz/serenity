/*
 * Copyright (c) 2024, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Firmware/DeviceTree/Device.h>
#include <Kernel/Firmware/DeviceTree/DeviceTree.h>
#include <Kernel/Firmware/DeviceTree/Management.h>

namespace Kernel::DeviceTree {

ErrorOr<Device::Resource> Device::get_resource(size_t index) const
{
    auto reg_entry = TRY(TRY(node().reg()).entry(index));
    return Resource {
        .paddr = PhysicalAddress { TRY(TRY(reg_entry.resolve_root_address()).as_flatptr()) },
        .size = TRY(reg_entry.length().as_size_t()),
    };
}

bool Device::is_dma_cache_coherent() const
{
    // Depending on the architecture DMA is by default cache coherent or not.
    // If the architecture has coherent DMA by default, the "dma-noncoherent" property indicates that a device isn't cache coherent.
    // If the architecture has non-coherent DMA by default, the "dma-coherent" property indicates that a device is cache coherent.
    // (see DTSpec 0.4, "2.3.10 dma-coherent" and "2.3.11 dma-noncoherent")

    // Whether an architecture is by default considered cache coherent, isn't specified by DTSpec.
    // So these defaults were taken from Linux, as that is what most devicetrees were written for.
#if ARCH(AARCH64)
    static constexpr bool IS_COHERENT_BY_DEFAULT = false;
#elif ARCH(RISCV64)
    static constexpr bool IS_COHERENT_BY_DEFAULT = true;
#elif ARCH(X86_64)
    static constexpr bool IS_COHERENT_BY_DEFAULT = true;
    VERIFY_NOT_REACHED(); // XXX: Don't compile this on x86.
#else
#    error Unknown architecture
#endif

    // Or is this code nicer?
    // bool is_coherent = IS_COHERENT_BY_DEFAULT;
    //
    // if (IS_COHERENT_BY_DEFAULT && node().has_property("dma-noncoherent"sv))
    //     is_coherent = false;
    // else if (!IS_COHERENT_BY_DEFAULT && node().has_property("dma-coherent"sv))
    //     is_coherent = true;
    //
    // return is_coherent;

    if constexpr (IS_COHERENT_BY_DEFAULT)
        return !node().has_property("dma-noncoherent"sv);

    return node().has_property("dma-coherent"sv);
}

ErrorOr<size_t> Device::get_interrupt_number(size_t index) const
{
    auto interrupts = TRY(node().interrupts(DeviceTree::get()));
    auto maybe_interrupt = interrupts.get(index);
    if (!maybe_interrupt.has_value())
        return EINVAL;

    return Management::the().resolve_interrupt_number(*maybe_interrupt);
}

}
