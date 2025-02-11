/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Devices/Device.h>
#include <Kernel/Devices/GPU/Console/ContiguousFramebufferConsole.h>
#include <Kernel/Devices/GPU/Generic/DisplayConnector.h>
#include <Kernel/Devices/GPU/Management.h>

namespace Kernel {

ErrorOr<NonnullRefPtr<GenericDisplayConnector>> GenericDisplayConnector::create_with_preset_resolution(PhysicalAddress framebuffer_address, size_t width, size_t height, size_t pitch)
{
    auto connector = TRY(Device::try_create_device<GenericDisplayConnector>(framebuffer_address, width, height, pitch));
    TRY(connector->create_attached_framebuffer_console());
    TRY(connector->initialize_edid_for_generic_monitor({}));
    return connector;
}

GenericDisplayConnector::GenericDisplayConnector(PhysicalAddress framebuffer_address, size_t width, size_t height, size_t pitch)
    : DisplayConnector(framebuffer_address, height * pitch, Memory::MemoryType::NonCacheable)
{
    m_current_mode_setting.horizontal_active = width;
    m_current_mode_setting.vertical_active = height;
    m_current_mode_setting.horizontal_stride = pitch;

#if ARCH(RISCV64)
    if (is_vf2()) {
        // TODO: Get addresses and size from devicetree
        m_l2_cache_mmio_region = MM.allocate_mmio_kernel_region(PhysicalAddress { 0x0201'0000 }, 0x4000, "SiFive L2 Cache Controller"sv, Memory::Region::Access::ReadWrite, Memory::Region::Cacheable::No).release_value_but_fixme_should_propagate_errors();
        m_l2_zero_device_region = MM.allocate_mmio_kernel_region(PhysicalAddress { 0x0a00'0000 }, 0x20'0000, "SiFive L2 Zero Device"sv, Memory::Region::Access::ReadWrite, Memory::Region::Cacheable::No).release_value_but_fixme_should_propagate_errors();

        m_l2_cache_size = 2 * MiB;

        memcpy(&m_l2_cache_config, reinterpret_cast<void const*>(m_l2_cache_mmio_region->vaddr().as_ptr()), sizeof(CacheConfig));
        dbgln("SiFive L2 Cache Controller Config register: bank count: {}, ways per bank: {}, sets per bank: {}, block size: {}",
            static_cast<u8>(m_l2_cache_config.bank_count), static_cast<u8>(m_l2_cache_config.ways_per_bank), 1ull << static_cast<u8>(m_l2_cache_config.lg_sets_per_bank), 1ull << static_cast<u8>(m_l2_cache_config.lg_block_size_in_bytes));
    }
#endif
}

ErrorOr<void> GenericDisplayConnector::create_attached_framebuffer_console()
{
    auto width = m_current_mode_setting.horizontal_active;
    auto height = m_current_mode_setting.vertical_active;
    auto pitch = m_current_mode_setting.horizontal_stride;

    m_framebuffer_console = Graphics::ContiguousFramebufferConsole::initialize(m_framebuffer_address.value(), width, height, pitch);
    GraphicsManagement::the().set_console(*m_framebuffer_console);
    return {};
}

void GenericDisplayConnector::enable_console()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_framebuffer_console);
    m_framebuffer_console->enable();
}

void GenericDisplayConnector::disable_console()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_framebuffer_console);
    m_framebuffer_console->disable();
}

ErrorOr<void> GenericDisplayConnector::flush_first_surface()
{
#if ARCH(RISCV64)
    if (!is_vf2())
        return ENOTSUP;

    // https://starfivetech.com/uploads/u74mc_core_complex_manual_21G1.pdf

    auto* way_mask_regs = reinterpret_cast<u64 volatile*>(m_l2_cache_mmio_region->vaddr().offset(0x0800).as_ptr());

    auto block_size_in_bytes = 1ul << m_l2_cache_config.lg_block_size_in_bytes;
    size_t way_size_in_bytes = ((1ul << m_l2_cache_config.lg_sets_per_bank) * m_l2_cache_config.bank_count) * block_size_in_bytes;

    // Flush the entire cache as described in 13.5
    // To flush the entire L2 cache:
    u8 way_count = m_l2_cache_config.ways_per_bank; // TODO: WayEnable instead?
    for (u8 way_index = 0; way_index < way_count; ++way_index) {
        // 1. Write WayMaskN to allow evictions from only way `way_index`
        for (size_t master = 0; master <= 26; master++)
            way_mask_regs[master] = 1 << way_index;

        // 2. Issue a series of loads (or stores) to addresses in the L2 Zero Device region that corre-
        // spond to each L2 index (i.e., one load/store per 64 B, total of (way-size-in-bytes/64) loads
        // or stores).
        for (size_t block_index = 0; block_index < way_size_in_bytes / block_size_in_bytes; ++block_index)
            *bit_cast<u64 volatile*>(m_l2_zero_device_region->vaddr().offset(way_index * way_size_in_bytes + block_index * block_size_in_bytes).as_ptr()) = 0;
    }

    for (size_t master = 0; master < 16; master++)
        way_mask_regs[master] = 0xffff;

    asm volatile("fence" ::: "memory");

    return {};
#else
    return ENOTSUP;
#endif
}

ErrorOr<void> GenericDisplayConnector::flush_rectangle(size_t buffer_index, FBRect const& rect)
{
    (void)buffer_index;
    (void)rect;

#if ARCH(RISCV64)
    if (!is_vf2())
        return ENOTSUP;

    TRY(flush_first_surface());

    // auto* flush64 = reinterpret_cast<u64 volatile*>(m_l2_cache_mmio_region->vaddr().offset(0x0200).as_ptr());

    // auto cache_aligned_rect_x = rect.x / 64 * 64;

    // auto rect_x_end = rect.x + rect.width;
    // auto cache_aligned_rect_x_end = ((rect_x_end + 64 - 1) / 64 * 64) - cache_aligned_rect_x;

    // FBRect cache_aligned_rect = {
    //     .head_index = rect.head_index,
    //     .x = cache_aligned_rect_x,
    //     .y = rect.y,
    //     .width = cache_aligned_rect_x_end - cache_aligned_rect_x,
    //     .height = rect.height,
    // };

    // for (size_t y = cache_aligned_rect.y; y < cache_aligned_rect.y + cache_aligned_rect.height; ++y) {
    //     for (size_t x = cache_aligned_rect.x; x < cache_aligned_rect.x + cache_aligned_rect.width; x += 64) {
    //         PhysicalPtr line = m_framebuffer_address.value().offset(y * m_current_mode_setting.horizontal_stride + x).get();
    //         *flush64 = line;
    //     }
    // }
    // asm volatile("fence" ::: "memory");

    return {};
#else
    (void)rect;
    return ENOTSUP;
#endif
}

}
