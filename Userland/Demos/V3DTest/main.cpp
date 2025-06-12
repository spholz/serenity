/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/API/V3D.h>
#include <LibCore/System.h>
#include <LibGUI/Application.h>
#include <LibGUI/Painter.h>
#include <LibGUI/Window.h>
#include <LibMain/Main.h>

#include "Buffer.h"
#include "Job.h"
#include "Triangle.h"

class Canvas final : public GUI::Widget {
    C_OBJECT(Canvas)
public:
    virtual ~Canvas() override = default;

    Canvas(NonnullRefPtr<Gfx::Bitmap> const& bitmap)
        : m_bitmap(bitmap)
    {
    }

private:
    NonnullRefPtr<Gfx::Bitmap> m_bitmap;

    virtual void paint_event(GUI::PaintEvent& event) override
    {
        GUI::Painter painter(*this);
        painter.add_clip_rect(event.rect());
        painter.draw_scaled_bitmap(rect(), *m_bitmap, m_bitmap->rect());
    }
};

int g_v3d_fd = -1;

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    auto app = TRY(GUI::Application::create(arguments));
    auto window = TRY(GUI::Window::try_create());

    g_v3d_fd = TRY(Core::System::open("/dev/gpu/render0"sv, O_RDWR));

    auto bitmap = TRY(Gfx::Bitmap::create(Gfx::BitmapFormat::BGRx8888, { 640, 480 }));

    auto framebuffer_bo = create_buffer_object(align_up_to(bitmap->size_in_bytes(), PAGE_SIZE));
    void* framebuffer = map_buffer_object(framebuffer_bo);

    auto job = run_triangle(framebuffer_bo.offset, 640, 480, bitmap->pitch());

    V3DJob kernel_job = {
        .tile_state_data_array_base_address = job.tile_state_data_array_bo.offset,
        .tile_allocation_memory_base_address = job.tile_alloc_memory_bo.offset,
        .tile_allocation_memory_size = job.tile_alloc_memory_bo.size,

        .binning_control_list_address = job.binner_control_list.bo().offset,
        .binning_control_list_size = job.binner_control_list.bo().size,

        .rendering_control_list_address = job.render_control_list.bo().offset,
        .rendering_control_list_size = job.render_control_list.bo().size,
    };

    TRY(Core::System::ioctl(g_v3d_fd, V3D_SUBMIT_JOB, &kernel_job));

    memcpy(bitmap->scanline_u8(0), framebuffer, bitmap->size_in_bytes());

    window->set_title("V3D Test");
    window->set_resizable(false);
    window->resize(640, 480);
    (void)window->set_main_widget<Canvas>(bitmap);
    window->show();

    return app->exec();
}
