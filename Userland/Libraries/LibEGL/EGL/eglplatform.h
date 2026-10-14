/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>

// 2.1.1 EGL Types

typedef unsigned char EGLBoolean;
typedef int EGLint;
typedef intptr_t EGLAttrib;
typedef void* EGLContext;
typedef void* EGLImage;
typedef void* EGLSurface;
typedef void* EGLSync;
typedef uint64_t EGLTime;

typedef unsigned int EGLenum;
typedef void* EGLDisplay;
typedef void* EGLNativeDisplayType;
typedef void* EGLConfig;
typedef void* EGLNativeWindowType;
typedef void* EGLClientBuffer;
typedef void* EGLNativePixmapType;
