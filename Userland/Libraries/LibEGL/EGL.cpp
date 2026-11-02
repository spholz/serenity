/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/StdLibExtras.h>
#include <LibEGL/EGL/egl.h>
#include <LibGL/GLContext.h>

#include <dlfcn.h>

// #define PLATFORM_X11
// #define PLATFORM_WAYLAND

#ifdef PLATFORM_WAYLAND
#    include <wayland-client.h>
#    include <wayland-egl-backend.h>
#elifdef PLATFORM_X11
#    include <X11/Xatom.h>
#    include <X11/Xlib.h>
#    include <X11/Xutil.h>
#endif

namespace {

thread_local EGLint s_error = EGL_SUCCESS;

// "The initial value of the current rendering API is EGL_OPENGL_ES_API, unless
//  OpenGL ES is not supported by an implementation, in which case the initial value
//  is EGL_NONE."
thread_local EGLenum s_current_rendering_api = EGL_NONE;

thread_local void* s_libgl_handle = nullptr;

struct EGLContextImpl {
    RefPtr<Gfx::Bitmap> bitmap;
    OwnPtr<GL::GLContext> gl_context;
    EGLSurface draw_surface;
    EGLSurface read_surface;
    EGLDisplay display;
};

thread_local EGLContextImpl* s_current_context = nullptr;

#ifdef PLATFORM_WAYLAND
struct EGLDisplayImpl {
    struct wl_shm* wl_shm;
    struct wl_event_queue* wayland_queue;
};

struct EGLSurfaceImpl {
    struct wl_egl_window* wl_egl_window;
    struct wl_buffer* wl_buffer;
    int width;
    int height;
    int depth;
};
#elifdef PLATFORM_X11
struct EGLDisplayImpl {
    Display* x11_display;
};

struct EGLSurfaceImpl {
    Window x11_window;
    GC x11_gc;
    int width;
    int height;
    int depth;
    XImage* x11_image;
    Visual* x11_visual;
};
#else
struct EGLSurfaceImpl {
    int width { 0 };
    int height { 0 };
    int depth { 0 };
};
#endif

}

EGLint eglGetError(void)
{
    return exchange(s_error, EGL_SUCCESS);
}

#ifdef PLATFORM_WAYLAND
static void registry_handle_global(void* data, struct wl_registry* registry, uint32_t name, char const* interface, uint32_t version)
{
    auto* egl_display = reinterpret_cast<EGLDisplayImpl*>(data);
    if (strcmp(interface, wl_shm_interface.name) == 0 && version >= 1) {
        egl_display->wl_shm = reinterpret_cast<wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
        VERIFY(egl_display->wl_shm != nullptr);
    }
}

static void registry_handle_global_remove(void* data, struct wl_registry* registry, uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};
#endif

EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id)
{
#ifdef PLATFORM_WAYLAND
    if (display_id == EGL_DEFAULT_DISPLAY) {
        s_error = EGL_SUCCESS;
        return EGL_NO_DISPLAY;
    }

    auto* egl_display = new (nothrow) EGLDisplayImpl;
    if (egl_display == nullptr) {
        s_error = EGL_BAD_ALLOC;
        return EGL_NO_DISPLAY;
    }

    auto* wayland_display = reinterpret_cast<wl_display*>(display_id);
    auto* wayland_queue = wl_display_create_queue_with_name(wayland_display, "SerenityOS LibEGL Queue");

    egl_display->wayland_queue = wayland_queue;

    auto* wayland_registry = wl_display_get_registry(wayland_display);
    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(wayland_registry), wayland_queue);
    wl_registry_add_listener(wayland_registry, &registry_listener, egl_display);

    int rc = wl_display_roundtrip_queue(wayland_display, wayland_queue);
    VERIFY(rc != -1);

    s_error = EGL_SUCCESS;
    return egl_display;
#elifdef PLATFORM_X11
    if (display_id == EGL_DEFAULT_DISPLAY) {
        s_error = EGL_SUCCESS;
        return EGL_NO_DISPLAY;
    }

    auto* egl_display = new (nothrow) EGLDisplayImpl;
    if (egl_display == nullptr) {
        s_error = EGL_BAD_ALLOC;
        return EGL_NO_DISPLAY;
    }

    auto* x11_display = reinterpret_cast<Display*>(display_id);

    egl_display->x11_display = x11_display;

    s_error = EGL_SUCCESS;
    return egl_display;
#else
    (void)display_id;
    s_error = EGL_SUCCESS;
    return reinterpret_cast<EGLDisplay>(1);
#endif
}

EGLBoolean eglInitialize(EGLDisplay, EGLint* major, EGLint* minor)
{
    dbgln("eglInitialize()");
    if (major != nullptr)
        *major = 1;

    if (minor != nullptr)
        *minor = 5;

    s_libgl_handle = dlopen("liblagom-gl.so", RTLD_LAZY | RTLD_LOCAL);
    if (!s_libgl_handle) {
        dbgln("Failed to load LibGL: {}", dlerror());
        s_error = EGL_NOT_INITIALIZED;
        return EGL_FALSE;
    }

    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

EGLBoolean eglTerminate(EGLDisplay)
{
    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

char const* eglQueryString(EGLDisplay, EGLint name)
{
    switch (name) {
    case EGL_VENDOR:
        s_error = EGL_SUCCESS;
        return "SerenityOS";
    case EGL_VERSION:
        s_error = EGL_SUCCESS;
        return "1.5";
    case EGL_EXTENSIONS:
        s_error = EGL_SUCCESS;
        return "EGL_KHR_client_get_all_proc_addresses EGL_EXT_platform_base";
    case EGL_CLIENT_APIS:
        s_error = EGL_SUCCESS;
        return "OpenGL";
    default:
        dbgln("eglQueryString(): unimplemented name {:#x}", name);
        s_error = EGL_BAD_ATTRIBUTE;
        return EGL_FALSE;
    }
}

EGLBoolean eglGetConfigs(EGLDisplay, EGLConfig* configs, EGLint config_size, EGLint* num_config)
{
    static int s_config_placeholder;

    if (num_config == nullptr) {
        s_error = EGL_BAD_PARAMETER;
        return EGL_TRUE;
    }

    *num_config = 1;

    if (configs == nullptr)
        return EGL_TRUE;

    if (config_size < 1) {
        s_error = EGL_SUCCESS;
        return EGL_TRUE;
    }

    configs[0] = reinterpret_cast<EGLConfig>(&s_config_placeholder);
    *num_config = 1;

    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

EGLBoolean eglChooseConfig(EGLDisplay, EGLint const*, EGLConfig* configs, EGLint config_size, EGLint* num_config)
{
    static int s_config_placeholder;

    if (num_config == nullptr) {
        s_error = EGL_BAD_PARAMETER;
        return EGL_TRUE;
    }

    if (config_size < 1) {
        s_error = EGL_SUCCESS;
        return EGL_TRUE;
    }

    configs[0] = reinterpret_cast<EGLConfig>(&s_config_placeholder);
    *num_config = 1;

    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig, EGLint attribute, EGLint* value)
{
    switch (attribute) {
    case EGL_ALPHA_SIZE:
        *value = 0; // If we don't say 0 here, GLFW will ignore us on wayland.
        break;
    case EGL_BLUE_SIZE:
        *value = 8;
        break;
    case EGL_GREEN_SIZE:
        *value = 8;
        break;
    case EGL_RED_SIZE:
        *value = 8;
        break;
    case EGL_DEPTH_SIZE:
        *value = 24;
        break;
    case EGL_STENCIL_SIZE:
        *value = 8;
        break;
    case EGL_NATIVE_VISUAL_ID: {
#ifdef PLATFORM_X11
        if (dpy == EGL_NO_DISPLAY) {
            *value = 0;
        } else {
            auto* egl_display = reinterpret_cast<EGLDisplayImpl*>(dpy);
            auto* x11_display = egl_display->x11_display;

            int default_screen = DefaultScreen(egl_display->x11_display);

            XVisualInfo visual_info = {};
            auto rc = XMatchVisualInfo(x11_display, default_screen, 24, TrueColor, &visual_info);
            VERIFY(rc != 0);

            *value = visual_info.visualid;
        }
#else
        (void)dpy;
        *value = 0;
#endif
        break;
    }
    case EGL_SAMPLES:
        *value = 0;
        break;
    case EGL_SURFACE_TYPE:
        *value = EGL_WINDOW_BIT;
        break;
    case EGL_COLOR_BUFFER_TYPE:
        *value = EGL_RGB_BUFFER;
        break;
    case EGL_RENDERABLE_TYPE:
        *value = EGL_OPENGL_BIT;
        break;
    default:
        dbgln("eglGetConfigAttrib(): unimplemented attribute {:#x}", attribute);
        s_error = EGL_BAD_ATTRIBUTE;
        return EGL_FALSE;
    }

    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig, EGLNativeWindowType win, EGLint const*)
{
#ifdef PLATFORM_WAYLAND
    (void)dpy;
    auto* wayland_window = reinterpret_cast<wl_egl_window*>(win);

    auto* egl_surface = new (nothrow) EGLSurfaceImpl;
    if (egl_surface == nullptr) {
        s_error = EGL_BAD_ALLOC;
        return EGL_NO_CONTEXT;
    }

    egl_surface->wl_egl_window = wayland_window;
    egl_surface->width = wayland_window->width;
    egl_surface->height = wayland_window->height;
    egl_surface->depth = 32; // FIXME

    s_error = EGL_SUCCESS;
    return egl_surface;
#elifdef PLATFORM_X11
    auto* egl_display = reinterpret_cast<EGLDisplayImpl*>(dpy);
    auto* x11_display = egl_display->x11_display;

    auto x11_window = reinterpret_cast<Window>(win);
    auto* egl_surface = new (nothrow) EGLSurfaceImpl;
    if (egl_surface == nullptr) {
        s_error = EGL_BAD_ALLOC;
        return EGL_NO_CONTEXT;
    }

    egl_surface->x11_window = x11_window;

    XWindowAttributes window_attributes {};
    Status status = XGetWindowAttributes(x11_display, x11_window, &window_attributes);
    VERIFY(status != 0);

    egl_surface->width = window_attributes.width;
    egl_surface->height = window_attributes.height;
    egl_surface->depth = window_attributes.depth;
    egl_surface->x11_visual = window_attributes.visual;

    egl_surface->x11_gc = XCreateGC(x11_display, x11_window, 0, nullptr);
    VERIFY(egl_surface->x11_gc != nullptr);

    s_error = EGL_SUCCESS;
    return egl_surface;
#else
    (void)dpy;
    (void)win;
    s_error = EGL_SUCCESS;
    return reinterpret_cast<EGLSurface>(1);
#endif
}

EGLSurface eglCreatePbufferSurface(EGLDisplay, EGLConfig, EGLint const*)
{
    s_error = EGL_BAD_MATCH;
    return EGL_FALSE;
}

EGLBoolean eglDestroySurface(EGLDisplay, EGLSurface)
{
    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

EGLBoolean eglQuerySurface(EGLDisplay, EGLSurface surface, EGLint attribute, EGLint* value)
{
    auto* egl_surface = reinterpret_cast<EGLSurfaceImpl*>(surface);

    switch (attribute) {
    case EGL_WIDTH:
        *value = egl_surface->width;
        break;
    case EGL_HEIGHT:
        *value = egl_surface->height;
        break;
    default:
        dbgln("eglQuerySurface(): unimplemented attribute {:#x}", attribute);
        s_error = EGL_BAD_ATTRIBUTE;
        return EGL_FALSE;
    }

    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

EGLBoolean eglBindAPI(EGLenum api)
{
    // "If api is not one of the values specified above, or if the client API specified
    //  by api is not supported by the implementation, an EGL_BAD_PARAMETER error
    //  is generated."
    if (api != EGL_OPENGL_API) {
        s_error = EGL_BAD_PARAMETER;
        return EGL_FALSE;
    }

    s_current_rendering_api = api;
    s_error = EGL_SUCCESS;

    return EGL_TRUE;
}

EGLenum eglQueryAPI(void)
{
    s_error = EGL_SUCCESS;
    return s_current_rendering_api;
}

EGLContext eglCreateContext(EGLDisplay, EGLConfig, EGLContext share_context, EGLint const*)
{
    dbgln("eglCreateContext()");
    // "An EGL_BAD_MATCH error is generated if the current rendering api is
    //  EGL_NONE (this situation can only arise in an implementation which does not
    //  support OpenGL ES 1.x, and prior to the first call to eglBindAPI)."
    if (s_current_rendering_api == EGL_NONE) {
        s_error = EGL_BAD_MATCH;
        return EGL_NO_CONTEXT;
    }

    // "An EGL_BAD_CONTEXT error is generated if share context is neither
    //  EGL_NO_CONTEXT nor a valid context of the same client API type as the newly
    //  created context."
    // We don't support sharing contexts at the moment.
    if (share_context != EGL_NO_CONTEXT) {
        s_error = EGL_BAD_CONTEXT;
        return EGL_NO_CONTEXT;
    }

#if defined(PLATFORM_WAYLAND) || defined(PLATFORM_X11)
    auto* egl_context = new (nothrow) EGLContextImpl;
    if (egl_context == nullptr) {
        s_error = EGL_BAD_ALLOC;
        return EGL_NO_CONTEXT;
    }

    s_error = EGL_SUCCESS;
    return reinterpret_cast<EGLContext>(egl_context);
#else
    auto bitmap = Gfx::Bitmap::create(Gfx::BitmapFormat::BGRx8888, { 640, 480 }).release_value_but_fixme_should_propagate_errors();
    bitmap->fill(Color::Cyan);
    auto gl_context = GL::create_context(*bitmap).release_value_but_fixme_should_propagate_errors();

    auto* egl_context = new (nothrow) EGLContextImpl;
    egl_context->bitmap = move(bitmap);
    egl_context->gl_context = move(gl_context);

    s_error = EGL_SUCCESS;
    return reinterpret_cast<EGLContext>(egl_context);
#endif
}

EGLBoolean eglDestroyContext(EGLDisplay, EGLContext ctx)
{
    dbgln("eglDestroyContext()");
    auto* egl_context = reinterpret_cast<EGLContextImpl*>(ctx);
    delete egl_context;

    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    (void)read;
    dbgln("eglMakeCurrent({})", ctx);
    if (ctx == EGL_NO_CONTEXT) {
        // if (draw == EGL_NO_SURFACE || read == EGL_NO_SURFACE) {
        //     s_error = EGL_BAD_MATCH;
        //     return EGL_FALSE;
        // }

        GL::make_context_current(nullptr);
        s_current_context = nullptr;
        s_error = EGL_SUCCESS;
        return EGL_TRUE;
    }

    // // We don't support being bound without read and draw surfaces.
    // if (draw == EGL_NO_SURFACE || read == EGL_NO_SURFACE) {
    //     s_error = EGL_BAD_MATCH;
    //     return EGL_FALSE;
    // }

    auto* egl_context = reinterpret_cast<EGLContextImpl*>(ctx);

#ifdef PLATFORM_WAYLAND
    if (!egl_context->gl_context) {
        // HACK: egLCreateContext doesn't have access to the surface, but we need its size to create a LibGL context.
        auto* display = reinterpret_cast<EGLDisplayImpl*>(dpy);
        auto* target_surface = reinterpret_cast<EGLSurfaceImpl*>(draw);

        auto width = target_surface->wl_egl_window->width;
        auto height = target_surface->wl_egl_window->height;

        auto buffer = Core::AnonymousBuffer::create_with_size(width * height * 4).release_value_but_fixme_should_propagate_errors();

        struct wl_shm_pool* pool = wl_shm_create_pool(display->wl_shm, buffer.fd(), buffer.size());
        VERIFY(pool != NULL);
        wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(pool), display->wayland_queue);

        struct wl_buffer* wl_buffer = wl_shm_pool_create_buffer(pool, 0, width, height, width * 4, WL_SHM_FORMAT_XRGB8888);
        VERIFY(wl_buffer != NULL);
        wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(wl_buffer), display->wayland_queue);
        wl_shm_pool_destroy(pool);

        target_surface->wl_buffer = wl_buffer;

        auto bitmap = Gfx::Bitmap::create_with_anonymous_buffer(Gfx::BitmapFormat::BGRx8888, buffer, { width, height }, 1).release_value_but_fixme_should_propagate_errors();
        bitmap->fill(Color::Magenta);

        egl_context->bitmap = bitmap;

        egl_context->gl_context = GL::create_context(*bitmap).release_value_but_fixme_should_propagate_errors();
    }
#elifdef PLATFORM_X11
    if (draw != EGL_NO_SURFACE) {
        auto* egl_display = reinterpret_cast<EGLDisplayImpl*>(dpy);
        auto* target_surface = reinterpret_cast<EGLSurfaceImpl*>(draw);

        auto* x11_display = egl_display->x11_display;

        auto width = target_surface->width;
        auto height = target_surface->height;

        auto bitmap = Gfx::Bitmap::create(Gfx::BitmapFormat::BGRx8888, { width, height }).release_value_but_fixme_should_propagate_errors();
        bitmap->fill(Color::Magenta);

        target_surface->x11_image = XCreateImage(x11_display, target_surface->x11_visual, target_surface->depth, ZPixmap, 0, reinterpret_cast<char*>(bitmap->scanline_u8(0)), width, height, 32, 0);
        VERIFY(target_surface->x11_image != nullptr);

        egl_context->bitmap = bitmap;

        egl_context->gl_context = GL::create_context(*bitmap).release_value_but_fixme_should_propagate_errors();
    } else {
        auto bitmap = Gfx::Bitmap::create(Gfx::BitmapFormat::BGRx8888, { 640, 480 }).release_value_but_fixme_should_propagate_errors();
        egl_context->gl_context = GL::create_context(*bitmap).release_value_but_fixme_should_propagate_errors();
    }

#else
    (void)dpy;
    (void)draw;
#endif

    GL::make_context_current(egl_context->gl_context);
    s_current_context = egl_context;

    s_current_context->read_surface = EGL_NO_SURFACE; // FIXME
    s_current_context->draw_surface = draw;
    s_current_context->display = dpy;

    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

EGLContext eglGetCurrentContext(void)
{
    s_error = EGL_SUCCESS;
    if (s_current_context == nullptr)
        return EGL_NO_CONTEXT;

    return s_current_context;
}

EGLSurface eglGetCurrentSurface(EGLint readdraw)
{
    s_error = EGL_SUCCESS;
    if (s_current_context == nullptr)
        return EGL_NO_SURFACE;

    if (readdraw == EGL_DRAW)
        return s_current_context->draw_surface;

    return s_current_context->read_surface;
}

EGLDisplay eglGetCurrentDisplay(void)
{
    s_error = EGL_SUCCESS;
    if (s_current_context == nullptr)
        return EGL_NO_SURFACE;

    return s_current_context->display;
}

EGLBoolean eglQueryContext(EGLDisplay, EGLContext, EGLint attribute, EGLint* value)
{
    switch (attribute) {
    case EGL_CONTEXT_CLIENT_TYPE:
        // FIXME: Don't hardcode this once we support more than OpenGL.
        *value = EGL_OPENGL_API;
        break;

    default:
        dbgln("eglQueryContext(): unimplemented attribute {:#x}", attribute);
        s_error = EGL_BAD_ATTRIBUTE;
        return EGL_FALSE;
    }

    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

EGLBoolean eglWaitClient(void)
{
    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

EGLBoolean eglWaitGL(void)
{
    // "For backwards compatibility, the function
    //    EGLBoolean eglWaitGL(void);
    //  is equivalent to
    //      EGLenum api = eglQueryAPI();
    //      eglBindAPI(EGL_OPENGL_ES_API);
    //      eglWaitClient();
    //      eglBindAPI(api);"
    EGLenum api = eglQueryAPI();
    eglBindAPI(EGL_OPENGL_ES_API);
    eglWaitClient();
    eglBindAPI(api);

    // FIXME: Propagate errors.
    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

EGLBoolean eglWaitNative(EGLint)
{
    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    if (s_current_context)
        s_current_context->gl_context->present();

#ifdef PLATFORM_WAYLAND
    (void)dpy;
    auto* surface_impl = reinterpret_cast<EGLSurfaceImpl*>(surface);

    wl_surface_attach(surface_impl->wl_egl_window->surface, surface_impl->wl_buffer, 0, 0);
    wl_surface_damage(surface_impl->wl_egl_window->surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(surface_impl->wl_egl_window->surface);

    s_error = EGL_SUCCESS;
    return EGL_TRUE;
#elifdef PLATFORM_X11
    auto* egl_display = reinterpret_cast<EGLDisplayImpl*>(dpy);
    auto* egl_surface = reinterpret_cast<EGLSurfaceImpl*>(surface);

    XPutImage(egl_display->x11_display, egl_surface->x11_window, egl_surface->x11_gc, egl_surface->x11_image, 0, 0, 0, 0, egl_surface->width, egl_surface->height);

    s_error = EGL_SUCCESS;
    return EGL_TRUE;
#else
    (void)dpy;
    (void)surface;

    s_error = EGL_SUCCESS;
    return EGL_TRUE;
#endif
}

EGLBoolean eglSwapInterval(EGLDisplay, EGLint)
{
    s_error = EGL_SUCCESS;
    return EGL_TRUE;
}

void (*eglGetProcAddress(char const* procname))(void)
{
    auto* addr = dlsym(s_libgl_handle, procname);
    dbgln("eglGetProcAddress(\"{}\") => {}", procname, addr);
    return reinterpret_cast<void (*)(void)>(addr);
}
