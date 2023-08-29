/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Endian.h>
#include <AK/Types.h>
#include <Kernel/Memory/PhysicalAddress.h>
#include <Kernel/Memory/Region.h>

namespace Kernel {

struct FWCfgFile {
    BigEndian<u32> size;
    BigEndian<u16> select;
    u16 reserved;
    char name[56];
};
static_assert(AssertSize<FWCfgFile, 64>());

template<class T>
struct __RemoveEndianness {
    using Type = T;
};

template<class T>
struct __RemoveEndianness<BigEndian<T>> {
    using Type = T;
};

template<class T>
struct __RemoveEndianness<LittleEndian<T>> {
    using Type = T;
};

template<typename T>
using RemoveEndianness = typename __RemoveEndianness<T>::Type;

struct [[gnu::packed]] RAMFBCfg {
    BigEndian<u64> addr;
    BigEndian<u32> fourcc;
    BigEndian<u32> flags;
    BigEndian<u32> width;
    BigEndian<u32> height;
    BigEndian<u32> stride;
};
static_assert(AssertSize<RAMFBCfg, 28>());
static_assert(sizeof(RAMFBCfg) <= PAGE_SIZE);

class QEMUFWCfg {
public:
    static void must_initialize(PhysicalAddress, FlatPtr ctl_offset, FlatPtr data_offset, FlatPtr dma_offset);
    static QEMUFWCfg& the();
    ErrorOr<void> initialize_ramfb(RAMFBCfg const&);

    template<IteratorFunction<FWCfgFile> Callback>
    IterationDecision for_each_cfg_file(Callback callback) const
    {
        select_configuration_item(FW_CFG_FILE_DIR);
        u32 file_count = read_data_reg<BigEndian<u32>>();
        for (u32 i = 0; i < file_count; i++) {
            FWCfgFile file;
            file.size = read_data_reg<BigEndian<u32>>();
            file.select = read_data_reg<BigEndian<u16>>();
            file.reserved = read_data_reg<BigEndian<u16>>();
            for (size_t i = 0; i < sizeof(FWCfgFile::name); i++)
                file.name[i] = read_data_reg<char>();

            if (callback(file) == IterationDecision::Break)
                return IterationDecision::Break;
        }
        return IterationDecision::Continue;
    }

private:
    constexpr static u16 FW_CFG_SIGNATURE = 0x0000;
    constexpr static u16 FW_CFG_ID = 0x0001;
    constexpr static u16 FW_CFG_FILE_DIR = 0x0019;
    constexpr static u16 FW_CFG_FILE_FIRST = 0x0020;

    OwnPtr<Memory::Region> m_fw_cfg_region;
    u16 volatile* m_fw_cfg_ctl;
    void volatile* m_fw_cfg_data;
    u64 volatile* m_fw_cfg_dma;

    void select_configuration_item(u16) const;

    template<typename T>
    T read_data_reg() const
    {
        return bit_cast<T>(*bit_cast<RemoveEndianness<T> volatile*>(m_fw_cfg_data));
    }

    QEMUFWCfg(PhysicalAddress, FlatPtr ctl_offset, FlatPtr data_offset, FlatPtr dma_offset);
};

}
