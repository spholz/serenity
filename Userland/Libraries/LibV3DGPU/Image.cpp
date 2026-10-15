/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Image.h"

#include <LibV3DGPU/Definitions.h>
#include <cstddef>

namespace V3DGPU {

static constexpr u32 MICROTILE_WIDTH = 4;
static constexpr u32 MICROTILE_HEIGHT = 4;

static constexpr u32 UIF_BLOCK_WIDTH = 2 * MICROTILE_WIDTH;
static constexpr u32 UIF_BLOCK_HEIGHT = 2 * MICROTILE_HEIGHT;

static constexpr u32 FOUR_BY_FOUR_WIDTH = 4 * UIF_BLOCK_WIDTH;
static constexpr u32 FOUR_BY_FOUR_HEIGHT = 4 * UIF_BLOCK_HEIGHT;

Image::Image(void const* ownership_token, GPU::PixelFormat const& pixel_format, u32 width, u32 height, u32 depth, u32 max_levels)
    : GPU::Image(ownership_token, pixel_format, width, height, depth, max_levels)
{
    VERIFY(pixel_format == GPU::PixelFormat::Alpha
        || pixel_format == GPU::PixelFormat::Intensity
        || pixel_format == GPU::PixelFormat::Luminance
        || pixel_format == GPU::PixelFormat::LuminanceAlpha
        || pixel_format == GPU::PixelFormat::RGB
        || pixel_format == GPU::PixelFormat::RGBA);
}

void Image::regenerate_mipmaps()
{
    dbgln("V3DGPU::Image::regenerate_mipmaps(): unimplemented");
}

void Image::write_texels(u32 level, Vector3<i32> const& output_offset, void const* input_data, GPU::ImageDataLayout const& input_layout)
{
    // FIXME: Format conversion.
    (void)input_layout;

    // FIXME: Mipmaps.
    if (level != 0)
        return;

    // Convert to UIF.
    // m_data = ByteBuffer::copy(input_data, static_cast<size_t>(width_at_level(0) * height_at_level(0)) * sizeof(u32)).release_value_but_fixme_should_propagate_errors();

    auto width = width_at_level(0);
    auto height = height_at_level(0);

    m_data.resize(width * height * 8); // FIXME: How do you calculate this?

    u32 const* input_image = reinterpret_cast<u32 const*>(input_data);

    auto microtile = [width, height, this, input_image](u32 start_x, u32 start_y, u32 output_index) -> u32 {
        for (u32 y = start_y; y < start_y + MICROTILE_HEIGHT; y++) {
            for (u32 x = start_x; x < start_x + MICROTILE_WIDTH; x++) {
                if (x < width && y < height)
                    m_data[output_index] = input_image[y * width + x];
                output_index++;
            }
        }

        return output_index;
    };

    auto uif_block = [&microtile](u32 start_x, u32 start_y, u32 output_index) -> u32 {
        for (u32 y = start_y; y < start_y + UIF_BLOCK_HEIGHT; y += MICROTILE_HEIGHT) {
            for (u32 x = start_x; x < start_x + UIF_BLOCK_WIDTH; x += MICROTILE_WIDTH) {
                output_index = microtile(x, y, output_index);
            }
        }

        return output_index;
    };

    auto four_by_four = [height, &uif_block](u32 start_x, u32 start_y, u32 output_index) -> u32 {
        for (u32 y = start_y; y < start_y + FOUR_BY_FOUR_HEIGHT; y += UIF_BLOCK_HEIGHT) {
            if (y >= height)
                continue;

            for (u32 x = start_x; x < start_x + FOUR_BY_FOUR_WIDTH; x += UIF_BLOCK_WIDTH)
                output_index = uif_block(x, y, output_index);
        }

        return output_index;
    };

    auto start_x = output_offset.x();
    auto start_y = output_offset.y();

    u32 output_index = 0;

    if (width <= MICROTILE_WIDTH || height <= MICROTILE_HEIGHT) {
        for (u32 y = start_y; y < height; y += MICROTILE_HEIGHT) {
            for (u32 x = start_x; x < width; x += MICROTILE_WIDTH) {
                output_index = microtile(x, y, output_index);
            }
        }
    } else if (width <= 2 * UIF_BLOCK_WIDTH) {
        for (u32 y = start_y; y < height; y += UIF_BLOCK_HEIGHT) {
            for (u32 x = start_x; x < width; x += UIF_BLOCK_WIDTH) {
                output_index = uif_block(x, y, output_index);
            }
        }
    } else {
        for (u32 x = start_x; x < width; x += FOUR_BY_FOUR_WIDTH) {
            for (u32 y = start_y; y < height; y += FOUR_BY_FOUR_HEIGHT) {
                // FIXME: XOR mode
                // if texture_width % 256 == 0 and (x // 32) % 2 == 1:
                //     output_index = four_by_four(x, y ^ 1 << 7, output_index)

                output_index = four_by_four(x, y, output_index);
            }
        }
    }

    size_t buffer_size = m_data.size() * sizeof(u32);

    m_buffer = Buffer::create(align_up_to(buffer_size, V3D_PAGE_SIZE), "LibV3DGPU: Texture"sv).release_value_but_fixme_should_propagate_errors();
    u8* texture_buffer_data = m_buffer->data().data();
    memcpy(texture_buffer_data, m_data.data(), buffer_size);
}

void Image::read_texels(u32, Vector3<i32> const&, void*, GPU::ImageDataLayout const&) const
{
    dbgln("V3DGPU::Image::read_texels(): unimplemented");
}

void Image::copy_texels(GPU::Image const&, u32, Vector3<u32> const&, Vector3<u32> const&, u32, Vector3<u32> const&)
{
    dbgln("V3DGPU::Image::copy_texels(): unimplemented");
}

}
