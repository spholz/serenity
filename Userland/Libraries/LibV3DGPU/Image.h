/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibGPU/Image.h>
#include <LibV3DGPU/Buffer.h>

namespace V3DGPU {

class Image final : public GPU::Image {
public:
    Image(void const* ownership_token, GPU::PixelFormat const&, u32 width, u32 height, u32 depth, u32 max_levels);

    virtual void regenerate_mipmaps() override;

    virtual void write_texels(u32 level, Vector3<i32> const& output_offset, void const* input_data, GPU::ImageDataLayout const&) override;
    virtual void read_texels(u32 level, Vector3<i32> const& input_offset, void* output_data, GPU::ImageDataLayout const&) const override;
    virtual void copy_texels(GPU::Image const& source, u32 source_level, Vector3<u32> const& source_offset, Vector3<u32> const& size, u32 destination_level, Vector3<u32> const& destination_offset) override;

    ReadonlyBytes data() { return ReadonlyBytes { m_data.data(), m_data.size() * sizeof(u32) }; }
    Buffer const& buffer() const { return *m_buffer; }

private:
    Vector<u32> m_data;
    Optional<Buffer> m_buffer;
};

}
