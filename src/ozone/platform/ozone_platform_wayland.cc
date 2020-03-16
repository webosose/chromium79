// Copyright 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/platform/ozone_platform_wayland.h"

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "ozone/platform/ozone_gpu_platform_support_host.h"
#include "ozone/platform/ozone_wayland_window.h"
#include "ozone/platform/window_manager_wayland.h"
#include "ozone/wayland/display.h"
#include "ozone/wayland/ozone_wayland_screen.h"
#include "ui/base/cursor/ozone/bitmap_cursor_factory_ozone.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/input_method_minimal.h"
#include "ui/display/types/native_display_delegate.h"
#include "ui/events/ozone/layout/keyboard_layout_engine_manager.h"
#include "ui/events/ozone/layout/xkb/xkb_evdev_codes.h"
#include "ui/events/ozone/layout/xkb/xkb_keyboard_layout_engine.h"
#include "ui/ozone/common/stub_overlay_manager.h"
#include "ui/ozone/public/system_input_injector.h"
#include "ui/platform_window/platform_window_delegate.h"
#include "ui/platform_window/platform_window_init_properties.h"

#if defined(USE_NEVA_MEDIA)
#include "ozone/media/video_window_controller_host_impl.h"
#include "ui/platform_window/neva/mojo/video_window_controller.mojom.h"
#endif  // defined(USE_NEVA_MEDIA)

namespace ui {

namespace {

// OzonePlatform for Wayland
//
// This platform is Linux with the Wayland display server.
class OzonePlatformWayland : public OzonePlatform {
 public:
  OzonePlatformWayland() {
    base::AtExitManager::RegisterTask(
        base::Bind(&base::DeletePointer<OzonePlatformWayland>, this));
  }

  ~OzonePlatformWayland() override {
  }

  // OzonePlatform:
  ui::SurfaceFactoryOzone* GetSurfaceFactoryOzone() override {
    return wayland_display_.get();
  }

  CursorFactoryOzone* GetCursorFactoryOzone() override {
    return cursor_factory_ozone_.get();
  }

  ui::OverlayManagerOzone* GetOverlayManager() override {
    return overlay_manager_.get();
  }

  InputController* GetInputController() override {
    return NULL;
  }

  std::unique_ptr<SystemInputInjector> CreateSystemInputInjector() override {
    return std::unique_ptr<SystemInputInjector>();
  }

  GpuPlatformSupportHost* GetGpuPlatformSupportHost() override {
    return gpu_platform_host_.get();
  }

  GpuPlatformSupport* GetGpuPlatformSupport() override {
    return wayland_display_.get();
  }

#if defined(USE_NEVA_MEDIA)
  void AddInterfaces(service_manager::BinderRegistry* registry) {
    registry->AddInterface(base::BindRepeating(
        &OzonePlatformWayland::GetVideoWindowControlleConnection,
        base::Unretained(this)));
  }

  void GetVideoWindowControlleConnection(
      mojo::PendingReceiver<ui::mojom::VideoWindowController> receiver) {
    wayland_display_->BindVideoWindowController(std::move(receiver));
  }

  ui::VideoWindowControllerHost* GetVideoWindowControllerHost() override {
    return video_window_controller_host_impl_.get();
  }
  ui::VideoWindowController* GetVideoWindowController() override {
    return wayland_display_->GetVideoWindowControllerImpl();
  }
#endif

  std::unique_ptr<PlatformWindow> CreatePlatformWindow(
      PlatformWindowDelegate* delegate,
      PlatformWindowInitProperties properties) override {
    return std::unique_ptr<PlatformWindow>(
        new OzoneWaylandWindow(delegate, gpu_platform_host_.get(),
                               window_manager_.get(), properties.bounds));
  }

  std::unique_ptr<InputMethod> CreateInputMethod(
      internal::InputMethodDelegate* delegate) override {
    return std::make_unique<InputMethodMinimal>(delegate);
  }

  std::unique_ptr<DesktopPlatformScreen> CreatePlatformScreen(
      DesktopPlatformScreenDelegate* delegate) {
    return std::unique_ptr<DesktopPlatformScreen>(
         new ozonewayland::OzoneWaylandScreen(delegate,
                                              window_manager_.get()));
  }

  std::unique_ptr<display::NativeDisplayDelegate> CreateNativeDisplayDelegate() override {
    return nullptr;
  }

  void InitializeUI(const InitParams& args) override {
    // For tests.
    if (wayland_display_.get())
      return;

    gpu_platform_host_.reset(new ui::OzoneGpuPlatformSupportHost());
    // Needed as Browser creates accelerated widgets through SFO.
    wayland_display_.reset(new ozonewayland::WaylandDisplay());
    cursor_factory_ozone_.reset(new ui::BitmapCursorFactoryOzone());
    overlay_manager_.reset(new StubOverlayManager());
    KeyboardLayoutEngineManager::SetKeyboardLayoutEngine(base::WrapUnique(
        new XkbKeyboardLayoutEngine(xkb_evdev_code_converter_)));
    window_manager_.reset(
        new ui::WindowManagerWayland(gpu_platform_host_.get()));
#if defined(USE_NEVA_MEDIA)
    video_window_controller_host_impl_.reset(
        new ui::VideoWindowControllerHostImpl(gpu_platform_host_.get()));
#endif
  }

  void InitializeGPU(const InitParams& args) override {
    if (!wayland_display_)
      wayland_display_.reset(new ozonewayland::WaylandDisplay());

    if (!wayland_display_->InitializeHardware())
      LOG(FATAL) << "failed to initialize display hardware";
  }

 private:
  std::unique_ptr<ui::BitmapCursorFactoryOzone> cursor_factory_ozone_;
  std::unique_ptr<ozonewayland::WaylandDisplay> wayland_display_;
  std::unique_ptr<StubOverlayManager> overlay_manager_;
  std::unique_ptr<ui::WindowManagerWayland> window_manager_;
  XkbEvdevCodes xkb_evdev_code_converter_;
  std::unique_ptr<ui::OzoneGpuPlatformSupportHost> gpu_platform_host_;
#if defined(USE_NEVA_MEDIA)
  std::unique_ptr<ui::VideoWindowControllerHostImpl>
      video_window_controller_host_impl_;
#endif
  DISALLOW_COPY_AND_ASSIGN(OzonePlatformWayland);
};

}  // namespace

OzonePlatform* CreateOzonePlatformWayland() { return new OzonePlatformWayland; }

std::unique_ptr<DesktopPlatformScreen> CreatePlatformScreen(
    DesktopPlatformScreenDelegate* delegate) {
  OzonePlatformWayland* platform =
      static_cast<OzonePlatformWayland*>(ui::OzonePlatform::GetInstance());
  return platform->CreatePlatformScreen(delegate);
}

}  // namespace ui
