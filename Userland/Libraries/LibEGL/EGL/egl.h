/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibEGL/EGL/eglplatform.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EGL_VERSION_1_0 1
#define EGL_VERSION_1_1 1
#define EGL_VERSION_1_2 1
#define EGL_VERSION_1_3 1
#define EGL_VERSION_1_4 1
#define EGL_VERSION_1_5 1

#define EGL_TRUE 1
#define EGL_FALSE 0

#define EGL_SUCCESS 0x3000
#define EGL_BAD_ALLOC 0x3003
#define EGL_BAD_ATTRIBUTE 0x3004
#define EGL_BAD_CONTEXT 0x3006
#define EGL_BAD_MATCH 0x3009
#define EGL_BAD_PARAMETER 0x300C
#define EGL_NONE 0x3038
#define EGL_OPENGL_ES_API 0x30A0
#define EGL_OPENVG_API 0x30A1
#define EGL_OPENGL_API 0x30A2

#define EGL_NO_CONTEXT ((EGLContext)0)
#define EGL_NO_DISPLAY ((EGLDisplay)0)

// 3.1 Errors
EGLint eglGetError(void);

// 3.2 Initialization
EGLDisplay eglGetPlatformDisplay(EGLenum platform, void* native_display, EGLAttrib const* attrib_list);
EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id);
EGLBoolean eglInitialize(EGLDisplay dpy, EGLint* major, EGLint* minor);
EGLBoolean eglTerminate(EGLDisplay dpy);

// 3.3 EGL Queries
char const* eglQueryString(EGLDisplay dpy, EGLint name);

// 3.4.1 Querying Configurations
EGLBoolean eglGetConfigs(EGLDisplay dpy, EGLConfig* configs, EGLint config_size, EGLint* num_config);
EGLBoolean eglChooseConfig(EGLDisplay dpy, EGLint const* attrib_list, EGLConfig* configs, EGLint config_size, EGLint* num_config);

// 3.4.3 Querying Configuration Attributes
EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint* value);

// 3.5.1 Creating On-Screen Rendering Surfaces
EGLSurface eglCreatePlatformWindowSurface(EGLDisplay dpy, EGLConfig config, void* native_window, EGLAttrib const* attrib_list);
EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, EGLint const* attrib_list);

// 3.5.2 Creating Off-Screen Rendering Surfaces
EGLSurface eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, EGLint const* attrib_list);

// 3.5.3 Binding Off-Screen Rendering Surfaces To Client Buffers
EGLSurface eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, EGLint const* attrib_list);

// 3.5.4 Creating Native Pixmap Rendering Surfaces
EGLSurface eglCreatePlatformPixmapSurface(EGLDisplay dpy, EGLConfig config, void* native_pixmap, EGLAttrib const* attrib_list);
EGLSurface eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, EGLint const* attrib_list);

// 3.5.5 Destroying Rendering Surfaces
EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface);

// 3.5.6 Surface Attributes
EGLBoolean eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);
EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint* value);

// 3.6.1 Binding a Surface to a OpenGL ES Texture
EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);

// 3.6.2 Releasing a Surface from an OpenGL ES Texture
EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);

// 3.7 Rendering Contexts
EGLBoolean eglBindAPI(EGLenum api);
EGLenum eglQueryAPI(void);

// 3.7.1 Creating Rendering Contexts
EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, EGLint const* attrib_list);

// 3.7.2 Destroying Rendering Contexts
EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx);

// 3.7.3 Binding Contexts and Drawables
EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);

// 3.7.4 Context Queries
EGLContext eglGetCurrentContext(void);
EGLSurface eglGetCurrentSurface(EGLint readdraw);
EGLDisplay eglGetCurrentDisplay(void);
EGLBoolean eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint* value);

// 3.8 Synchronization Primitives
EGLBoolean eglWaitClient(void);
EGLBoolean eglWaitGL(void);
EGLBoolean eglWaitNative(EGLint engine);

// 3.8.1 Sync Objects
EGLSync eglCreateSync(EGLDisplay dpy, EGLenum type, EGLAttrib const* attrib_list);

// 3.8.1.3 Waiting For Sync Objects
EGLint eglClientWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout);
EGLBoolean eglWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags);

// 3.8.1.4 Querying Sync Object Attributs
EGLBoolean eglGetSyncAttrib(EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib* value);
EGLBoolean eglDestroySync(EGLDisplay dpy, EGLSync sync);

// 3.9 EGLImage Specification and Management
EGLImage eglCreateImage(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, EGLAttrib const* attrib_list);
EGLBoolean eglDestroyImage(EGLDisplay dpy, EGLImage image);

// 3.10.1 Posting to a Window
EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface);

// 3.10.2 Copying to a Native Pixmap
EGLBoolean eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target);

// 3.10.3 Posting Semantics
EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval);

// 3.11 Obtaining Function Pointers
void (*eglGetProcAddress(char const* procname))(void);

// 3.12 Releasing Thread State
EGLBoolean eglReleaseThread(void);

#ifdef __cplusplus
}
#endif
