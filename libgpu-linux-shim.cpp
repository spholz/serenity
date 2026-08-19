/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Vector.h>
#include <Kernel/API/V3D.h>

#include <dlfcn.h>
#include <drm/v3d_drm.h>
#include <xf86drm.h>

static Vector<u32> s_buffer_object_handles;

static int (*original_ioctl)(int fd, unsigned long request, ...) = nullptr;

static Optional<u32> s_out_sync;

static int ioctl_v3d_submit_job(int fd, V3DJob const* job)
{
    if (!s_out_sync.has_value()) {
        u32 out_sync = 0;
        int ret = drmSyncobjCreate(fd, DRM_SYNCOBJ_CREATE_SIGNALED, &out_sync);
        VERIFY(ret == 0);

        s_out_sync = out_sync;
    }

    u32 out_sync = s_out_sync.value();

    struct drm_v3d_submit_cl submit_cl = {
        // Binner command list (optional)
        .bcl_start = job->binning_control_list_address,
        .bcl_end = job->binning_control_list_address + job->binning_control_list_size,

        // Render command list
        .rcl_start = job->rendering_control_list_address,
        .rcl_end = job->rendering_control_list_address + job->rendering_control_list_size,

        // Optional sync objects
        .in_sync_bcl = 0,
        .in_sync_rcl = 0,
        .out_sync = out_sync,

        // Tile alloc memory offset
        .qma = job->tile_allocation_memory_base_address,
        // Tile alloc memory size
        .qms = job->tile_allocation_memory_size,

        // Offset of the tile state data array
        .qts = job->tile_state_data_array_base_address,

        // BOs referenced by this job
        .bo_handles = bit_cast<u64>(s_buffer_object_handles.data()),
        .bo_handle_count = static_cast<u32>(s_buffer_object_handles.size()),

        .flags = 0,

        .perfmon_id = 0,

        .pad = 0,

        .extensions = 0,
    };

    if (drmIoctl(fd, DRM_IOCTL_V3D_SUBMIT_CL, &submit_cl) != 0)
        return -1;

    int ret = drmSyncobjWait(fd, &out_sync, 1, INT64_MAX, DRM_SYNCOBJ_WAIT_FLAGS_WAIT_ALL, nullptr);
    VERIFY(ret == 0);

    return 0;
}

static int ioctl_v3d_allocate_buffer(int fd, V3DBuffer* buffer)
{
    struct drm_v3d_create_bo create_bo = {
        .size = buffer->size,
        .flags = 0,

        // Set by the kernel
        .handle = 0,
        .offset = 0,
    };

    if (drmIoctl(fd, DRM_IOCTL_V3D_CREATE_BO, &create_bo) != 0)
        return -1;

    s_buffer_object_handles.append(create_bo.handle);

    buffer->id = create_bo.handle;
    buffer->address = create_bo.offset;

    struct drm_v3d_mmap_bo mmap_bo = {
        .handle = create_bo.handle,
        .flags = 0,

        // Set by the kernel
        .offset = 0,
    };

    if (drmIoctl(fd, DRM_IOCTL_V3D_MMAP_BO, &mmap_bo) != 0)
        return -1;

    buffer->mmap_offset = mmap_bo.offset;

    return 0;
}

static int ioctl_v3d_free_buffer(int fd, u64 buffer_id)
{
    struct drm_gem_close close = {
        .handle = static_cast<u32>(buffer_id),
        .pad = 0,
    };

    if (drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close) != 0)
        return -1;

    s_buffer_object_handles.remove_first_matching([buffer_id](u32 handle) { return handle == buffer_id; });

    return 0;
}

extern "C" int ioctl(int fd, unsigned long request, ...)
{
    if (original_ioctl == nullptr) {
        original_ioctl = reinterpret_cast<decltype(original_ioctl)>(dlsym(RTLD_NEXT, "ioctl"));
        VERIFY(original_ioctl != nullptr);
    }

    va_list args;
    va_start(args, request);

    void* arg = va_arg(args, void*);

    int ret = -1;
    if (request == V3D_ALLOCATE_BUFFER)
        ret = ioctl_v3d_allocate_buffer(fd, reinterpret_cast<V3DBuffer*>(arg));
    else if (request == V3D_FREE_BUFFER)
        ret = ioctl_v3d_free_buffer(fd, bit_cast<u64>(arg));
    else if (request == V3D_SUBMIT_JOB)
        ret = ioctl_v3d_submit_job(fd, reinterpret_cast<V3DJob const*>(arg));
    else
        ret = original_ioctl(fd, request, arg);

    va_end(args);

    return ret;
}
