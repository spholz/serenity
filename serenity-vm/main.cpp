/*
 * Copyright 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aidl/android/system/virtualizationcommon/DeathReason.h>
#include <aidl/android/system/virtualizationcommon/ErrorCode.h>
#include <aidl/android/system/virtualizationservice/BnVirtualMachineCallback.h>
#include <aidl/android/system/virtualizationservice/IVirtualMachine.h>
#include <aidl/android/system/virtualizationservice/IVirtualMachineCallback.h>
#include <aidl/android/system/virtualizationservice/IVirtualizationService.h>
#include <aidl/android/system/virtualizationservice/VirtualMachineConfig.h>
#include <aidl/android/system/virtualizationservice/VirtualMachineState.h>

#include <android-base/thread_annotations.h>

#include <android-base/errors.h>
#include <android-base/file.h>
#include <android-base/result.h>
#include <android-base/unique_fd.h>
#include <array>
#include <binder/IServiceManager.h>

#include <android/binder_manager.h>

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <sys/mman.h>
#include <ui/DisplayState.h>

#include <stdio.h>
#include <system/window.h>
#include <unistd.h>

#include <binder_rpc_unstable.hpp>

#include <cstddef>
#include <memory>
#include <sys/mman.h>

#include <android/os/IInputFlinger.h>
#include <gui/PidUid.h>
#include <input/Input.h>
#include <input/InputConsumer.h>
#include <input/InputTransport.h>

#include <linux/input.h>

using namespace android;
using namespace std::literals;

using android::base::ErrnoError;
using android::base::Error;
using android::base::Pipe;
using android::base::Result;
using android::base::Socketpair;
using android::base::unique_fd;

using android::os::IInputFlinger;

using ndk::ScopedAStatus;
using ndk::ScopedFileDescriptor;
using ndk::SharedRefBase;
using ndk::SpAIBinder;

using aidl::android::system::virtualizationcommon::DeathReason;
using aidl::android::system::virtualizationcommon::ErrorCode;
using aidl::android::system::virtualizationservice::BnVirtualMachineCallback;
using aidl::android::system::virtualizationservice::CustomMemoryBackingFile;
using aidl::android::system::virtualizationservice::DiskImage;
using aidl::android::system::virtualizationservice::InputDevice;
using aidl::android::system::virtualizationservice::IVirtualizationService;
using aidl::android::system::virtualizationservice::IVirtualMachine;
using aidl::android::system::virtualizationservice::Partition;
using aidl::android::system::virtualizationservice::UsbConfig;
using aidl::android::system::virtualizationservice::VirtualMachineConfig;
using aidl::android::system::virtualizationservice::VirtualMachineRawConfig;
using aidl::android::system::virtualizationservice::VirtualMachineState;

size_t page_round_up(size_t x)
{
    return (((size_t)(x)) + getpagesize() - 1) & (~(getpagesize() - 1));
}

void verification_failed(char const* message)
{
    std::cerr << "VERIFICATION FAILED: " << message << '\n';
    __builtin_trap();
}

#define __stringify_helper(x) #x
#define __stringify(x) __stringify_helper(x)
#define VERIFY(expr)                                                               \
    (__builtin_expect(!(expr), 0)                                                  \
            ? verification_failed(#expr " at " __FILE__ ":" __stringify(__LINE__)) \
            : (void)0)

// Copied from vm_demo_native. We can't use libavf, as you can't add input devices with its API.

//--------------------------------------------------------------------------------------------------
// Step 1: connect to IVirtualizationService
//--------------------------------------------------------------------------------------------------
static constexpr char const VIRTMGR_PATH[] = "/apex/com.android.virt/bin/virtmgr";
static constexpr size_t VIRTMGR_THREADS = 2;

// Start IVirtualizationService instance and get FD for the unix domain socket that is connected to
// the service. The returned FD should be kept open until the service is no longer needed.
Result<unique_fd> get_service_fd()
{
    unique_fd server_fd, client_fd;
    if (!Socketpair(SOCK_STREAM, &server_fd, &client_fd)) {
        return ErrnoError() << "Failed to create socketpair";
    }

    unique_fd wait_fd, ready_fd;
    if (!Pipe(&wait_fd, &ready_fd, 0)) {
        return ErrnoError() << "Failed to create pipe";
    }

    if (fork() == 0) {
        client_fd.reset();
        wait_fd.reset();

        auto server_fd_str = std::to_string(server_fd.get());
        auto ready_fd_str = std::to_string(ready_fd.get());

        if (execl(VIRTMGR_PATH, VIRTMGR_PATH, "--rpc-server-fd", server_fd_str.c_str(),
                "--ready-fd", ready_fd_str.c_str(), nullptr)
            == -1) {
            return ErrnoError() << "Failed to execute virtmgr";
        }
    }

    server_fd.reset();
    ready_fd.reset();

    char buf;
    if (read(wait_fd.get(), &buf, sizeof(buf)) < 0) {
        return ErrnoError() << "Failed to wait for VirtualizationService to be ready";
    }

    return client_fd;
}

// Establish a binder communication channel over the unix domain socket and returns the remote
// IVirtualizationService.
Result<SpAIBinder> connect_service(int fd)
{
    std::unique_ptr<ARpcSession, decltype(&ARpcSession_free)> session(ARpcSession_new(),
        &ARpcSession_free);
    ARpcSession_setFileDescriptorTransportMode(session.get(),
        ARpcSession_FileDescriptorTransportMode::Unix);
    ARpcSession_setMaxIncomingThreads(session.get(), VIRTMGR_THREADS);
    ARpcSession_setMaxOutgoingConnections(session.get(), VIRTMGR_THREADS);
    AIBinder* binder = ARpcSession_setupUnixDomainBootstrapClient(session.get(), fd);
    if (binder == nullptr) {
        return Error() << "Failed to connect to VirtualizationService";
    }
    return SpAIBinder { binder };
}

// Utility function for opening a file at a given path and wrap the resulting FD in
// ScopedFileDescriptor so that it can be passed to the service.
Result<ScopedFileDescriptor> open_file(std::string const& path, int flags)
{
    int fd = open(path.c_str(), flags, S_IWUSR);
    if (fd == -1) {
        return ErrnoError() << "Failed to open " << path;
    }
    return ScopedFileDescriptor(fd);
}

//--------------------------------------------------------------------------------------------------
// Step 3: create a VM and start it
//--------------------------------------------------------------------------------------------------

// Create a virtual machine with the config, but doesn't start it yet.
Result<std::shared_ptr<IVirtualMachine>> create_virtual_machine(
    IVirtualizationService& service, VirtualMachineConfig& config)
{
    std::shared_ptr<IVirtualMachine> vm;

    ScopedFileDescriptor console_out_fd(fcntl(fileno(stdout), F_DUPFD_CLOEXEC));
    ScopedFileDescriptor console_in_fd(fcntl(fileno(stdin), F_DUPFD_CLOEXEC));
    ScopedFileDescriptor log_fd(fcntl(fileno(stdout), F_DUPFD_CLOEXEC));
    ScopedFileDescriptor dump_dt_fd(-1);

    __android_log_print(ANDROID_LOG_ERROR, "serenity-vm", "-> createVm()");
    ScopedAStatus ret = service.createVm(config, console_out_fd, console_in_fd, log_fd, dump_dt_fd, &vm);
    __android_log_print(ANDROID_LOG_ERROR, "serenity-vm", "<- CreateVm()");
    if (!ret.isOk()) {
        return Error() << "Failed to create VM: " << ret;
    }
    return vm;
}

class Callback : public BnVirtualMachineCallback {
public:
    Callback()
    {
    }

    ScopedAStatus onPayloadStarted(int32_t cid)
    {
        std::cerr << "onPayloadStarted(" << cid << ")\n";
        return ScopedAStatus::ok();
    }

    ScopedAStatus onPayloadReady(int32_t cid)
    {
        std::cerr << "onPayloadReady(" << cid << ")\n";
        return ScopedAStatus::ok();
    }

    ScopedAStatus onPayloadFinished(int32_t cid, int32_t exitCode)
    {
        std::cerr << "onPayloadFinished(" << cid << ", " << exitCode << ")\n";
        return ScopedAStatus::ok();
    }

    ScopedAStatus onError(int32_t cid, ErrorCode errorCode, std::string const& message)
    {
        std::cerr << "onError(" << cid << ", " << std::size_t(errorCode) << ", " << message << ")\n";
        return ScopedAStatus::ok();
    }

    ScopedAStatus onDied(int32_t cid, DeathReason reason)
    {
        std::cerr << "onDied(" << cid << ", " << std::size_t(reason) << ")\n";
        return ScopedAStatus::ok();
    }
};

Result<std::shared_ptr<Callback>> start_virtual_machine(std::shared_ptr<IVirtualMachine> vm)
{
    std::shared_ptr<Callback> cb = SharedRefBase::make<Callback>();
    ScopedAStatus ret = vm->registerCallback(cb);
    if (!ret.isOk()) {
        return Error() << "Failed to register callback to virtual machine";
    }
    __android_log_print(ANDROID_LOG_ERROR, "serenity-vm", "-> vm.start()");
    ret = vm->start();
    __android_log_print(ANDROID_LOG_ERROR, "serenity-vm", "<- vm.start()");
    if (!ret.isOk()) {
        return Error() << "Failed to start virtual machine";
    }
    return cb;
}

struct SamsungWindowInfo {
    uint64_t pad;
    sp<IBinder> token;

    sp<IBinder> windowToken;

    int32_t id = -1;
    std::string name;
    uint32_t samsungFlags;
    std::chrono::nanoseconds dispatchingTimeout = std::chrono::seconds(5);

    Rect frame = Rect::INVALID_RECT;

    ui::Size contentSize = ui::Size(0, 0);

    int32_t surfaceInset = 0;

    float globalScaleFactor = 1.0f;

    float alpha;

    ui::Transform transform;

    std::optional<ui::Transform> cloneLayerStackTransform;

    Region touchableRegion;

    uint8_t pad5[116];
    gui::TouchOcclusionMode touchOcclusionMode = gui::TouchOcclusionMode::BLOCK_UNTRUSTED;
    gui::Pid ownerPid = gui::Pid::INVALID;
    gui::Uid ownerUid = gui::Uid::INVALID;
    std::string packageName;
    ftl::Flags<os::InputConfig> inputConfig;
    ui::LogicalDisplayId displayId = ui::LogicalDisplayId::INVALID;
    gui::InputApplicationInfo applicationInfo;
    bool replaceTouchableRegionWithCrop = false;
    wp<IBinder> touchableRegionCropHandle;

    gui::WindowInfo::Type layoutParamsType = gui::WindowInfo::Type::UNKNOWN;
    ftl::Flags<gui::WindowInfo::Flag> layoutParamsFlags;

    sp<IBinder> focusTransferTarget;

    bool canOccludePresentation = false;
};

// NOTE: This overlay expects /#address-cells == 2 and /#size-cells == 2.
// /dts-v1/;
// /plugin/;
//
// &{/} {
//     framebuffer {
//         compatible = "simple-framebuffer";
//         reg = <0xdeadbeef 0xdeadbeef 0xdeadbeef 0xdeadbeef>;
//         width = <0xdeadbeef>;
//         height = <0xdeadbeef>;
//         stride = <0xdeadbeef>;
//         format = "x8r8g8b8";
//     };
// };

auto dtbo_template = std::array<uint8_t, 334> {
    0xd0, 0x0d, 0xfe, 0xed, 0x00, 0x00, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x38,
    0x00, 0x00, 0x01, 0x18, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x11,
    0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36,
    0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x66, 0x72, 0x61, 0x67,
    0x6d, 0x65, 0x6e, 0x74, 0x40, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x5f, 0x5f, 0x6f, 0x76, 0x65, 0x72, 0x6c, 0x61,
    0x79, 0x5f, 0x5f, 0x00, 0x00, 0x00, 0x00, 0x01, 0x66, 0x72, 0x61, 0x6d,
    0x65, 0x62, 0x75, 0x66, 0x66, 0x65, 0x72, 0x00, 0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x0c, 0x73, 0x69, 0x6d, 0x70,
    0x6c, 0x65, 0x2d, 0x66, 0x72, 0x61, 0x6d, 0x65, 0x62, 0x75, 0x66, 0x66,
    0x65, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x10,
    0x00, 0x00, 0x00, 0x17, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
    0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x1b, 0xde, 0xad, 0xbe, 0xef,
    0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x21,
    0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04,
    0x00, 0x00, 0x00, 0x28, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x2f, 0x78, 0x38, 0x72, 0x38,
    0x67, 0x38, 0x62, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x09, 0x74, 0x61, 0x72, 0x67, 0x65, 0x74, 0x2d, 0x70,
    0x61, 0x74, 0x68, 0x00, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x74, 0x69, 0x62,
    0x6c, 0x65, 0x00, 0x72, 0x65, 0x67, 0x00, 0x77, 0x69, 0x64, 0x74, 0x68,
    0x00, 0x68, 0x65, 0x69, 0x67, 0x68, 0x74, 0x00, 0x73, 0x74, 0x72, 0x69,
    0x64, 0x65, 0x00, 0x66, 0x6f, 0x72, 0x6d, 0x61, 0x74, 0x00
};

static constexpr size_t dtbo_fixup_offset_framebuffer_address = 0x00ac;
static constexpr size_t dtbo_fixup_offset_framebuffer_size = 0x00ac + 2 * 4;
static constexpr size_t dtbo_fixup_offset_framebuffer_width = 0x00c8;
static constexpr size_t dtbo_fixup_offset_framebuffer_height = 0x00d8;
static constexpr size_t dtbo_fixup_offset_framebuffer_stride = 0x00e8;

struct Window {
    sp<SurfaceComposerClient> surface_composer_client;
    sp<Surface> surface;
    sp<SurfaceControl> surface_control;

    InputConsumer* input_consumer;

    size_t width;
    size_t height;
    size_t pitch;
};

Result<sp<gui::WindowInfoHandle>> create_window_info_for_input(Window& window, std::string_view window_and_app_name)
{
    // NOTE: Samsung added new fields in WindowInfo and therefore changed ABI compared to AOSP!
    void* window_info_handle_buffer = malloc(1 * 1024 * 1024);
    sp<gui::WindowInfoHandle> input_info = new (window_info_handle_buffer) gui::WindowInfoHandle;
    new (input_info->editInfo()) SamsungWindowInfo;
    // sp<gui::WindowInfoHandle> mInputInfo = sp<gui::WindowInfoHandle>::make();

    auto input_flinger_binder = defaultServiceManager()->waitForService(String16("inputflinger"));
    VERIFY(input_flinger_binder != nullptr);
    auto input_flinger = interface_cast<IInputFlinger>(input_flinger_binder);

    android::os::InputChannelCore temp_channel;
    VERIFY(input_flinger->createInputChannel(window.surface_control->getName() + " channel", &temp_channel).isOk());
    std::shared_ptr<InputChannel> client_channel = InputChannel::create(std::move(temp_channel));
    window.input_consumer = new InputConsumer(client_channel);

    reinterpret_cast<SamsungWindowInfo*>(input_info->editInfo())->token = client_channel->getConnectionToken();
    reinterpret_cast<SamsungWindowInfo*>(input_info->editInfo())->name = window_and_app_name;
    reinterpret_cast<SamsungWindowInfo*>(input_info->editInfo())->touchableRegion.set(Rect(window.width, window.height));
    reinterpret_cast<SamsungWindowInfo*>(input_info->editInfo())->contentSize.set(ui::Size(window.width, window.height));
    reinterpret_cast<SamsungWindowInfo*>(input_info->editInfo())->ownerPid = gui::Pid(getpid());
    reinterpret_cast<SamsungWindowInfo*>(input_info->editInfo())->ownerUid = gui::Uid(getuid());
    reinterpret_cast<SamsungWindowInfo*>(input_info->editInfo())->displayId = ui::LogicalDisplayId::DEFAULT;
    reinterpret_cast<SamsungWindowInfo*>(input_info->editInfo())->layoutParamsType = gui::WindowInfo::Type::APPLICATION;

    reinterpret_cast<SamsungWindowInfo*>(input_info->editInfo())->applicationInfo.name = window_and_app_name;
    reinterpret_cast<SamsungWindowInfo*>(input_info->editInfo())->applicationInfo.token = new BBinder();
    reinterpret_cast<SamsungWindowInfo*>(input_info->editInfo())->applicationInfo.dispatchingTimeoutMillis = std::chrono::duration_cast<std::chrono::milliseconds>(5s).count();

    return input_info;
}

Result<Window> create_window(std::string_view window_name)
{
    Window window;

    window.surface_composer_client = sp<SurfaceComposerClient>::make();
    VERIFY(window.surface_composer_client->initCheck() == NO_ERROR);

    auto const ids = SurfaceComposerClient::getPhysicalDisplayIds();
    VERIFY(!ids.empty());

    auto const display_token = SurfaceComposerClient::getPhysicalDisplayToken(ids.front());
    VERIFY(display_token != nullptr);

    ui::DisplayMode display_mode;
    VERIFY(SurfaceComposerClient::getActiveDisplayMode(display_token, &display_mode) == NO_ERROR);

    ui::DisplayState display_state;
    VERIFY(SurfaceComposerClient::getDisplayState(display_token, &display_state) == NO_ERROR);

    ui::Size const& resolution = display_mode.resolution;
    auto screen_width = resolution.getWidth();
    auto screen_height = resolution.getHeight();

    if (display_state.orientation == ui::ROTATION_90 || display_state.orientation == ui::ROTATION_270) {
        std::swap(screen_width, screen_height);
    }

    window.width = screen_width;
    window.height = screen_height;

    window.surface_control = window.surface_composer_client->createSurface(
        String8(window_name), window.width, window.height,
        PIXEL_FORMAT_RGBA_8888, ISurfaceComposerClient::eFXSurfaceEffect);
    VERIFY(window.surface_control != nullptr);
    VERIFY(window.surface_control->isValid());

    auto input_info = OR_RETURN(create_window_info_for_input(window, window_name));

    SurfaceComposerClient::Transaction {}
        .show(window.surface_control)
        .setLayer(window.surface_control, 1)
        .setPosition(window.surface_control, 0, 0)
        .setCrop(window.surface_control, Rect(screen_width, screen_height))
        .setAlpha(window.surface_control, 1)
        .setInputWindowInfo(window.surface_control, input_info)
        .apply();

    window.surface = window.surface_control->getSurface();

    {
        ANativeWindow_Buffer surface;
        VERIFY(window.surface->lock(&surface, nullptr) == 0);
        window.pitch = surface.stride * 4;
        window.surface->unlockAndPost();
    }

    return window;
}

struct Framebuffer {
    size_t width;
    size_t height;
    size_t pitch;
    size_t size;

    size_t guest_paddr;

    uint8_t* memory;
};

Result<Framebuffer> create_framebuffer_and_add_to_vm(Window const& window, VirtualMachineRawConfig& raw_config)
{
    Framebuffer framebuffer;

    framebuffer.width = window.width;
    framebuffer.height = window.height;
    framebuffer.pitch = window.pitch;
    framebuffer.size = page_round_up(framebuffer.width + framebuffer.height * framebuffer.pitch);

    int32_t framebuffer_fd = ashmem_create_region("framebuffer", framebuffer.size);
    VERIFY(framebuffer_fd != -1);

    framebuffer.memory = reinterpret_cast<uint8_t*>(mmap(nullptr, framebuffer.size, PROT_READ | PROT_WRITE, MAP_SHARED, framebuffer_fd, 0));
    VERIFY(framebuffer.memory != MAP_FAILED);

    // Fill the screen with blue.
    for (size_t i = 0; i < framebuffer.size / 4; i++) {
        reinterpret_cast<uint32_t*>(framebuffer.memory)[i] = 0xff'00'00'ff;
    }

    framebuffer.guest_paddr = (0x8000'0000 + (raw_config.memoryMib * 1024 * 1024) - framebuffer.size) & ~(getpagesize() - 1);

    CustomMemoryBackingFile fb_memory_backing_file;
    fb_memory_backing_file.file = ScopedFileDescriptor(framebuffer_fd);
    fb_memory_backing_file.rangeStart = framebuffer.guest_paddr;
    fb_memory_backing_file.size = framebuffer.size;

    raw_config.customMemoryBackingFiles.push_back(std::move(fb_memory_backing_file));

    return framebuffer;
}

Result<ScopedFileDescriptor> create_devicetree_overlay(Framebuffer const& framebuffer)
{
    int dtbo_fd = open("/tmp/serenity.dtbo", O_CREAT | O_RDWR | O_TRUNC, 0600);
    VERIFY(dtbo_fd != -1);
    VERIFY(ftruncate(dtbo_fd, dtbo_template.size()) == 0);

    uint8_t* dtbo_mem = static_cast<uint8_t*>(mmap(nullptr, dtbo_template.size(), PROT_READ | PROT_WRITE, MAP_SHARED, dtbo_fd, 0));
    VERIFY(dtbo_mem != MAP_FAILED);

    memcpy(dtbo_mem, dtbo_template.data(), dtbo_template.size());

    *reinterpret_cast<uint32_t*>(&dtbo_mem[dtbo_fixup_offset_framebuffer_address]) = htonl(framebuffer.guest_paddr >> 32);
    *reinterpret_cast<uint32_t*>(&dtbo_mem[dtbo_fixup_offset_framebuffer_address + 4]) = htonl(framebuffer.guest_paddr & 0xffff'ffff);

    *reinterpret_cast<uint32_t*>(&dtbo_mem[dtbo_fixup_offset_framebuffer_size]) = htonl(framebuffer.size >> 32);
    *reinterpret_cast<uint32_t*>(&dtbo_mem[dtbo_fixup_offset_framebuffer_size + 4]) = htonl(framebuffer.size & 0xffff'ffff);

    *reinterpret_cast<uint32_t*>(&dtbo_mem[dtbo_fixup_offset_framebuffer_width]) = htonl(framebuffer.width);
    *reinterpret_cast<uint32_t*>(&dtbo_mem[dtbo_fixup_offset_framebuffer_height]) = htonl(framebuffer.height);
    *reinterpret_cast<uint32_t*>(&dtbo_mem[dtbo_fixup_offset_framebuffer_stride]) = htonl(framebuffer.pitch);

    munmap(dtbo_mem, dtbo_template.size());

    return ScopedFileDescriptor(dtbo_fd);
}

Result<void> inner_main()
{
    auto window = OR_RETURN(create_window("serenity-vm framebuffer"));

    std::filesystem::path work_dir_path("/data/local/tmp/serenity-vm/");

    unique_fd fd = OR_RETURN(get_service_fd());
    auto service_binder = OR_RETURN(connect_service(fd.get()));

    auto service = IVirtualizationService::fromBinder(service_binder);
    VERIFY(service != nullptr);

    // std::shared_ptr<IVirtualizationServiceInternal> service_internal = IVirtualizationServiceInternal::fromBinder(service->asBinder());
    // auto service_internal = IVirtualizationServiceInternal::fromBinder(service_binder);
    // VERIFY(service_internal != nullptr);

    // auto service_internal_binder = ndk::SpAIBinder(AServiceManager_waitForService("android.system.virtualizationservice"));
    // VERIFY(service_internal_binder != nullptr);
    // auto service_internal = IVirtualizationServiceInternal::fromBinder(service_internal_binder);

    VirtualMachineRawConfig raw_config;
    raw_config.bootloader = OR_RETURN(open_file("/apex/com.android.virt/etc/u-boot.bin", O_RDONLY));
    // raw_config.kernel = OR_RETURN(open_file(work_dir_path / "Kernel.bin", O_RDONLY));
    raw_config.platformVersion = "~1.0";
    raw_config.memoryMib = 1024;
    raw_config.networkSupported = true;

    UsbConfig usb_config;
    usb_config.controller = true;

    unique_fd mouse_server_fd, mouse_client_fd;
    VERIFY(Socketpair(SOCK_STREAM, &mouse_server_fd, &mouse_client_fd));

    InputDevice::Mouse mouse;
    mouse.pfd = ScopedFileDescriptor(mouse_client_fd);

    raw_config.inputDevices.push_back(std::move(mouse));

    raw_config.usbConfig = usb_config;

    // raw_config.gdbPort = 1234;

    // Partition root_partition;
    // root_partition.image = OR_RETURN(open_file(work_dir_path / "_disk_image", O_RDWR));
    // root_partition.writable = true;
    // root_partition.label = "rootfs";

    // raw_config.params = "dump_fdt serial_debug root=block100:0 panic=shutdown";

    DiskImage disk;
    // disk.partitions.push_back(std::move(root_partition));
    disk.image = OR_RETURN(open_file(work_dir_path / "uefi_disk_image", O_RDWR));
    disk.writable = true;

    raw_config.disks.push_back(std::move(disk));

    auto framebuffer = OR_RETURN(create_framebuffer_and_add_to_vm(window, raw_config));

    raw_config.devices = OR_RETURN(create_devicetree_overlay(framebuffer));

    VirtualMachineConfig config = std::move(raw_config);
    std::shared_ptr<IVirtualMachine> vm = OR_RETURN(create_virtual_machine(*service, config));
    std::shared_ptr<Callback> cb = OR_RETURN(start_virtual_machine(vm));

    // SpAIBinder crosvm_display_service_binder;
    // service_internal->waitDisplayService(&crosvm_display_service_binder);
    // VERIFY(crosvm_display_service_binder != nullptr);
    //
    // std::shared_ptr<ICrosvmAndroidDisplayService> crosvm_display_service = ICrosvmAndroidDisplayService::fromBinder(crosvm_display_service_binder);
    // VERIFY(crosvm_display_service != nullptr);

    // aidl::android::view::Surface view_surface(window.get());
    // crosvm_display_service->setSurface(&view_surface, false);

    PreallocatedInputEventFactory mInputEventFactory;

    for (;;) {
        ANativeWindow_Buffer surface;
        VERIFY(window.surface->lock(&surface, nullptr) == 0);

        auto* dst_pixels = reinterpret_cast<uint8_t*>(surface.bits);
        auto* src_pixels = reinterpret_cast<uint8_t*>(framebuffer.memory);
        VERIFY(surface.bits != nullptr);
        VERIFY(surface.width == framebuffer.width);
        VERIFY(surface.height == framebuffer.height);
        VERIFY(surface.stride == framebuffer.pitch / 4);
        for (size_t y = 0; y < framebuffer.height; y++) {
            for (size_t x = 0; x < framebuffer.width; x++) {
                size_t i = (y * surface.stride + x) * 4;
                uint8_t* src_pixel = &src_pixels[i];
                uint8_t* dst_pixel = &dst_pixels[i];

                dst_pixel[0] = src_pixel[2]; // r
                dst_pixel[1] = src_pixel[1]; // g
                dst_pixel[2] = src_pixel[0]; // b
                dst_pixel[3] = 0xff;         // a
            }
        }

        window.surface->unlockAndPost();

        VirtualMachineState state;
        VERIFY(vm->getState(&state).isOk());
        if (state == VirtualMachineState::DEAD || state == VirtualMachineState::FINISHED)
            break;

        InputEvent* ev;
        uint32_t seq_id;
        status_t consume_result = window.input_consumer->consume(&mInputEventFactory, true, -1, &seq_id, &ev);
        if (consume_result == OK) {
            VERIFY(window.input_consumer->sendFinishedSignal(seq_id, true) == OK);
            switch (ev->getType()) {
            case InputEventType::MOTION: {
                MotionEvent* mev = static_cast<MotionEvent*>(ev);
                // std::cerr << "motion event: " << *mev << '\n';

                std::vector<input_event> linux_evs = {
                    {
                        .type = EV_ABS,
                        .code = ABS_X,
                        .value = static_cast<int>((mev->getX(0) / window.width) * 65535),
                    },
                    {
                        .type = EV_ABS,
                        .code = ABS_Y,
                        .value = static_cast<int>((mev->getY(0) / window.height) * 65535),
                    },
                };

                if (mev->getAction() == AMOTION_EVENT_ACTION_DOWN) {
                    linux_evs.push_back({
                        .type = EV_KEY,
                        .code = BTN_LEFT,
                        .value = 1,
                    });
                } else if (mev->getAction() == AMOTION_EVENT_ACTION_UP) {
                    linux_evs.push_back({
                        .type = EV_KEY,
                        .code = BTN_LEFT,
                        .value = 0,
                    });
                }

                linux_evs.push_back({
                    .type = EV_SYN,
                    .code = SYN_REPORT,
                    .value = 0,
                });

                VERIFY(write(mouse_server_fd, linux_evs.data(), linux_evs.size() * sizeof(input_event)) != -1);
                break;
            }
            default:
                std::cerr << "Unknown input event type: " << static_cast<std::size_t>(ev->getType()) << '\n';
                std::cerr << *ev << '\n';
                break;
            }
        } else if (consume_result != WOULD_BLOCK) {
            std::cerr << "failed to read input events: " << consume_result << '\n';
        }

        std::this_thread::sleep_for(16.666ms); // 60 FPS
    }

    return {};
}

int main()
{
    if (auto ret = inner_main(); !ret.ok()) {
        std::cerr << ret.error() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Done" << std::endl;
    return EXIT_SUCCESS;
}
