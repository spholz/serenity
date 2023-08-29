/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Endian.h>
#include <Kernel/Firmware/QEMUFwCfg.h>
#include <Kernel/Memory/MemoryManager.h>

namespace Kernel {

static QEMUFWCfg* s_the;

struct FWCfgFiles {
    BigEndian<u32> count;
    FWCfgFile f[];
};

struct FWCfgDmaAccess {
    enum class ControlFlags : u32 {
        Error = 1 << 0,
        Read = 1 << 1,
        Skip = 1 << 2,
        Select = 1 << 3,
        Write = 1 << 4,
    };

    BigEndian<u32> control;
    BigEndian<u32> length;
    BigEndian<u64> address;
};
AK_ENUM_BITWISE_OPERATORS(FWCfgDmaAccess::ControlFlags)

QEMUFWCfg::QEMUFWCfg(PhysicalAddress fw_cfg_addr, FlatPtr ctl_offset, FlatPtr data_offset, FlatPtr dma_offset)
{
    auto fw_cfg_end = Memory::page_round_up(fw_cfg_addr.offset(max(max(ctl_offset, data_offset), dma_offset) + 8).get()).release_value_but_fixme_should_propagate_errors();
    m_fw_cfg_region = MM.allocate_kernel_region(fw_cfg_addr.page_base(), fw_cfg_end - fw_cfg_addr.page_base().get(), "QEMU fw_cfg"sv, Memory::Region::Access::ReadWrite).release_value_but_fixme_should_propagate_errors();

    u8* fw_cfg_base = m_fw_cfg_region->vaddr().offset(fw_cfg_addr.offset_in_page()).as_ptr();

    m_fw_cfg_ctl = reinterpret_cast<u16 volatile*>(fw_cfg_base + ctl_offset);
    m_fw_cfg_data = reinterpret_cast<u64 volatile*>(fw_cfg_base + data_offset);
    m_fw_cfg_dma = reinterpret_cast<u64 volatile*>(fw_cfg_base + dma_offset);
}

QEMUFWCfg& QEMUFWCfg::the()
{
    VERIFY(s_the);
    return *s_the;
}

void QEMUFWCfg::must_initialize(PhysicalAddress fw_cfg_addr, FlatPtr ctl_offset, FlatPtr data_offset, FlatPtr dma_offset)
{
    VERIFY(!s_the);
    s_the = new (nothrow) QEMUFWCfg(fw_cfg_addr, ctl_offset, data_offset, dma_offset);
    VERIFY(s_the);

    auto& fw_cfg = the();

    fw_cfg.select_configuration_item(FW_CFG_SIGNATURE);
    VERIFY(fw_cfg.read_data_reg<BigEndian<u32>>() == 0x51454d55); // "QEMU"

    fw_cfg.select_configuration_item(FW_CFG_ID);
    VERIFY((fw_cfg.read_data_reg<LittleEndian<u32>>() & 0b11) == 0b11); // Traditional interface + DMA interface

    VERIFY(AK::convert_between_host_and_big_endian(*fw_cfg.m_fw_cfg_dma) == 0x51454d5520434647); // "QEMU CFG"

    dbgln("QEMU fw_cfg directory:");
    fw_cfg.for_each_cfg_file([](FWCfgFile const& file) {
        dbgln("  {} ({} bytes) @ {:#04x}", file.name, file.size, file.select);
        return IterationDecision::Continue;
    });

    size_t width = 640;
    size_t height = 480;
    size_t bpp = 32;
    size_t stride = width * (bpp / 8);

#define fourcc_code(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

    // auto framebuffer_region_size = Memory::page_round_up(height * stride).release_value_but_fixme_should_propagate_errors();
    // auto framebuffer_region = MM.allocate_kernel_region(framebuffer_region_size, "QEMU ramfb Framebuffer"sv, Memory::Region::Access::ReadWrite, AllocationStrategy::Reserve, Memory::Region::Cacheable::No).release_value();
    // framebuffer_data = framebuffer_region->vaddr().as_ptr();
    // auto framebuffer_paddr = framebuffer_region->physical_page(0)->paddr();

    auto framebuffer_paddr = PhysicalAddress { 0x8200'0000 };

    MUST(fw_cfg.initialize_ramfb({
        .addr = framebuffer_paddr.get(),
        .fourcc = fourcc_code('X', 'R', '2', '4'),
        .flags = 0,
        .width = width,
        .height = height,
        .stride = stride,
    }));

    multiboot_framebuffer_addr = framebuffer_paddr;
    multiboot_framebuffer_width = width;
    multiboot_framebuffer_height = height;
    multiboot_framebuffer_pitch = stride;

    multiboot_framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_RGB;
}

ErrorOr<void> QEMUFWCfg::initialize_ramfb(RAMFBCfg const& cfg)
{
    RefPtr<Memory::PhysicalPage> ramfb_cfg_dma_page;
    auto ramfb_cfg_region = TRY(MM.allocate_dma_buffer_page("QEMU ramfb cfg"sv, Memory::Region::Access::Write, ramfb_cfg_dma_page));
    auto* ramfb_cfg = bit_cast<RAMFBCfg*>(ramfb_cfg_region->vaddr().as_ptr());
    auto ramfb_cfg_paddr = ramfb_cfg_dma_page->paddr();

    dbgln("ramfb_cfg: {}, ramfb_cfg_paddr: {}", ramfb_cfg, ramfb_cfg_paddr);

    auto dma_access_region = TRY(MM.allocate_kernel_region(TRY(Memory::page_round_up(sizeof(RAMFBCfg))), "QEMU fw_cfg DMA Access"sv, Memory::Region::Access::Write, AllocationStrategy::AllocateNow, Memory::Region::Cacheable::No));
    auto* dma_access = bit_cast<FWCfgDmaAccess*>(dma_access_region->vaddr().as_ptr());
    auto dma_access_paddr = dma_access_region->physical_page(0)->paddr();

    dbgln("dma_access: {}, dma_access_paddr: {}", dma_access, dma_access_paddr);

    Optional<u16> select;
    for_each_cfg_file([&select](FWCfgFile const& file) {
        if (file.name == "etc/ramfb"sv) {
            select = file.select;
            return IterationDecision::Break;
        }
        return IterationDecision::Continue;
    });

    if (!select.has_value())
        return ENOENT;

    dbgln("Found QEMU ramfb device @ {:#04x}", select.value());
    memcpy(ramfb_cfg, &cfg, sizeof(RAMFBCfg));

    *dma_access = FWCfgDmaAccess {
        .control = to_underlying(FWCfgDmaAccess::ControlFlags::Write | FWCfgDmaAccess::ControlFlags::Select) | (select.value() << 16),
        .length = sizeof(RAMFBCfg),
        .address = ramfb_cfg_paddr.get(),
    };

    *m_fw_cfg_dma = AK::convert_between_host_and_big_endian(dma_access_paddr.get());

    // Wait until transfer is done
    while ((dma_access->control & ~to_underlying(FWCfgDmaAccess::ControlFlags::Error)) != 0)
        ;

    VERIFY((dma_access->control & to_underlying(FWCfgDmaAccess::ControlFlags::Error)) == 0);

    dbgln("QEMU ramfb device @ {:#04x} initialized", select.value());

    return {};
}

void QEMUFWCfg::select_configuration_item(u16 item) const
{
    *m_fw_cfg_ctl = AK::convert_between_host_and_big_endian(item);
}

}
