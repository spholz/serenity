/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Applications/3DFileViewer/WavefrontOBJLoader.h>
#include <LibCore/ElapsedTimer.h>
#include <LibGL/GLContext.h>
#include <LibGUI/Application.h>
#include <LibGUI/Painter.h>
#include <LibGUI/Window.h>
#include <LibMain/Main.h>

static constexpr u32 UPDATE_FRAMERATE_EVERY_FRAMES = 30;

class Canvas final : public GUI::Widget {
    C_OBJECT(Canvas)
public:
    virtual ~Canvas() override = default;

    Canvas(GL::GLContext& context, NonnullRefPtr<Gfx::Bitmap> const& bitmap)
        : m_bitmap(bitmap)
        , m_context(context)
    {
        m_bitmap->fill(Color::Cyan);

        warnln("Vendor:     {}", reinterpret_cast<char const*>(glGetString(GL_VENDOR)));
        warnln("Renderer:   {}", reinterpret_cast<char const*>(glGetString(GL_RENDERER)));
        warnln("Version:    {}", reinterpret_cast<char const*>(glGetString(GL_VERSION)));
        warnln("Entensions: {}", reinterpret_cast<char const*>(glGetString(GL_EXTENSIONS)));

        start_timer(0);

        m_framerate_timer = Core::ElapsedTimer::start_new();

        ByteString path = "/home/anon/Documents/3D Models/teapot.obj";
        auto file = MUST(Core::File::open(path, Core::File::OpenMode::Read));
        m_mesh = MUST(m_mesh_loader.load(path, move(file)));
    }

private:
    NonnullRefPtr<Gfx::Bitmap> m_bitmap;

    virtual void paint_event(GUI::PaintEvent& event) override
    {
        GUI::Painter painter(*this);
        painter.add_clip_rect(event.rect());
        painter.draw_scaled_bitmap(rect(), *m_bitmap, m_bitmap->rect());
    }

    virtual void timer_event(Core::TimerEvent&) override
    {
        auto x_offset = sin(MonotonicTime::now().milliseconds() / 1000.f);

        glClearColor(0.2f, 0.0f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0, 0, -8.5);
        glRotatef(x_offset * 50, 0, 1, 0);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        auto zoom = 0.5;
        auto const half_aspect_ratio = static_cast<double>(m_bitmap->width()) / m_bitmap->height() * zoom;
        glFrustum(-half_aspect_ratio, half_aspect_ratio, -zoom, zoom, .5, 10.);

        m_mesh->draw(1.0f);

        // glBegin(GL_QUADS);
        // glColor3f(0, 0, 1);
        // glVertex3f(-1, 1, 1);
        // glVertex3f(-1, -1, 1);
        // glVertex3f(1, -1, 1);
        // glVertex3f(1, 1, 1);
        // glEnd();
        //
        // glBegin(GL_QUADS);
        // glColor3f(0, 1, 0);
        // glVertex3f(-1, 1, -1);
        // glVertex3f(-1, -1, -1);
        // glVertex3f(1, -1, -1);
        // glVertex3f(1, 1, -1);
        // glEnd();

        // glBegin(GL_TRIANGLES);
        //
        // // Front
        // glColor3f(1, 0, 0);
        // glVertex3f(-1,  1,  1);
        // glVertex3f(-1, -1,  1);
        // glVertex3f( 1, -1,  1);
        //
        // glVertex3f(-1,  1,  1);
        // glVertex3f( 1, -1,  1);
        // glVertex3f( 1,  1,  1);
        //
        // // Back
        // glColor3f(0, 1, 0);
        // glVertex3f(-1,  1, -1);
        // glVertex3f(-1, -1, -1);
        // glVertex3f( 1, -1, -1);
        //
        // glVertex3f(-1,  1, -1);
        // glVertex3f( 1, -1, -1);
        // glVertex3f( 1,  1, -1);
        //
        // glEnd();

        // glColor3f(0, 0, 1);
        // glVertex2f(-0.5f + x_offset, 0.5f);
        //
        // glColor3f(1, 0, 0);
        // glVertex2f(-0.5f, -0.5f);
        //
        // glColor3f(0, 1, 0);
        // glVertex2f(0.5f, -0.5f);
        //
        // glColor3f(0, 0, 1);
        // glVertex2f(-0.4f, 0.5f);
        //
        // glColor3f(0, 1, 0);
        // glVertex2f(0.6f, -0.5f);
        //
        // glColor3f(1, 0, 0);
        // glVertex2f(0.6f, 0.5f);

        VERIFY(glGetError() == 0);

        m_context.present();

        if ((m_frame_count % UPDATE_FRAMERATE_EVERY_FRAMES) == 0) {
            auto render_time = static_cast<double>(m_framerate_timer.elapsed_milliseconds()) / UPDATE_FRAMERATE_EVERY_FRAMES;
            auto frame_rate = render_time > 0 ? 1000 / render_time : 0;
            dbgln("{:.0f} fps, {:.1f} ms", frame_rate, render_time);
            m_framerate_timer = Core::ElapsedTimer::start_new();
        }

        update();

        m_frame_count++;
    }

    GL::GLContext& m_context;

    WavefrontOBJLoader m_mesh_loader;
    RefPtr<Mesh> m_mesh;

    size_t m_frame_count { 0 };
    Core::ElapsedTimer m_framerate_timer;
};

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    auto app = TRY(GUI::Application::create(arguments));
    auto window = TRY(GUI::Window::try_create());

    auto bitmap = TRY(Gfx::Bitmap::create(Gfx::BitmapFormat::BGRx8888, { 640, 480 }));

    auto context = TRY(GL::create_context(*bitmap));
    GL::make_context_current(context);

    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    window->set_title("LibGL Test");
    window->set_resizable(false);
    window->resize(640, 480);
    (void)window->set_main_widget<Canvas>(*context, bitmap);
    window->show();

    return app->exec();
}
