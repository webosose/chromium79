// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/window_tree_host_platform.h"

#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host_observer.h"
#include "ui/base/layout.h"
#include "ui/compositor/compositor.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/events/keyboard_hook.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/keycodes/dom/dom_keyboard_layout_map.h"
#include "ui/platform_window/platform_window_base.h"
#include "ui/platform_window/platform_window_init_properties.h"

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#endif

#if defined(OS_WIN)
#include "ui/base/cursor/cursor_loader_win.h"
#include "ui/platform_window/win/win_window.h"
#endif

///@name USE_NEVA_APPRUNTIME
///@{
#include "base/command_line.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ui_base_neva_switches.h"
///@}

#if defined(USE_NEVA_APPRUNTIME)
#include "ui/views/widget/desktop_aura/neva/native_event_delegate.h"
#endif

#if defined(USE_X11)
#include "ui/platform_window/x11/x11_window.h"  // nogncheck
#endif

namespace aura {

// static
std::unique_ptr<WindowTreeHost> WindowTreeHost::Create(
    ui::PlatformWindowInitProperties properties) {
  return std::make_unique<WindowTreeHostPlatform>(
      std::move(properties),
      std::make_unique<aura::Window>(nullptr, client::WINDOW_TYPE_UNKNOWN));
}

WindowTreeHostPlatform::WindowTreeHostPlatform(
    ui::PlatformWindowInitProperties properties,
    std::unique_ptr<Window> window,
    const char* trace_environment_name,
    bool use_external_begin_frame_control)
    : WindowTreeHost(std::move(window)) {
  bounds_in_pixels_ = properties.bounds;
  CreateCompositor(viz::FrameSinkId(),
                   /* force_software_compositor */ false,
                   use_external_begin_frame_control,
                   /* are_events_in_pixels */ true, trace_environment_name);
  CreateAndSetPlatformWindow(std::move(properties));
}

WindowTreeHostPlatform::WindowTreeHostPlatform(std::unique_ptr<Window> window)
    : WindowTreeHost(std::move(window)),
      widget_(gfx::kNullAcceleratedWidget),
      current_cursor_(ui::CursorType::kNull) {}

void WindowTreeHostPlatform::CreateAndSetPlatformWindow(
    ui::PlatformWindowInitProperties properties) {
#if defined(USE_OZONE)
  platform_window_ = ui::OzonePlatform::GetInstance()->CreatePlatformWindow(
      this, std::move(properties));
///@name USE_NEVA_APPRUNTIME
///@{
  bool ime_enabled = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableNevaIme);
  if (ime_enabled)
    GetInputMethod()->AddObserver(this);
  SetImeEnabled(ime_enabled);
///@}
#elif defined(OS_WIN)
  platform_window_.reset(new ui::WinWindow(this, properties.bounds));
#elif defined(USE_X11)
  auto x11_window = std::make_unique<ui::X11Window>(this);
  x11_window->Initialize(std::move(properties));
  SetPlatformWindow(std::move(x11_window));
#else
  NOTIMPLEMENTED();
#endif
}

void WindowTreeHostPlatform::SetPlatformWindow(
    std::unique_ptr<ui::PlatformWindowBase> window) {
  platform_window_ = std::move(window);
}

WindowTreeHostPlatform::~WindowTreeHostPlatform() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableNevaIme))
    GetInputMethod()->RemoveObserver(this);
  DestroyCompositor();
  DestroyDispatcher();

  // |platform_window_| may not exist yet.
  if (platform_window_)
    platform_window_->Close();
}

ui::EventSource* WindowTreeHostPlatform::GetEventSource() {
  return this;
}

gfx::AcceleratedWidget WindowTreeHostPlatform::GetAcceleratedWidget() {
  return widget_;
}

void WindowTreeHostPlatform::ShowImpl() {
  platform_window_->Show();
}

void WindowTreeHostPlatform::HideImpl() {
  platform_window_->Hide();
}

gfx::Rect WindowTreeHostPlatform::GetBoundsInPixels() const {
  return platform_window_ ? platform_window_->GetBounds() : gfx::Rect();
}

void WindowTreeHostPlatform::SetBoundsInPixels(const gfx::Rect& bounds) {
  pending_size_ = bounds.size();
  platform_window_->SetBounds(bounds);
}

gfx::Point WindowTreeHostPlatform::GetLocationOnScreenInPixels() const {
  return platform_window_->GetBounds().origin();
}

void WindowTreeHostPlatform::SetCapture() {
  platform_window_->SetCapture();
}

void WindowTreeHostPlatform::ReleaseCapture() {
  platform_window_->ReleaseCapture();
}

bool WindowTreeHostPlatform::CaptureSystemKeyEventsImpl(
    base::Optional<base::flat_set<ui::DomCode>> dom_codes) {
  // Only one KeyboardHook should be active at a time, otherwise there will be
  // problems with event routing (i.e. which Hook takes precedence) and
  // destruction ordering.
  DCHECK(!keyboard_hook_);
  keyboard_hook_ = ui::KeyboardHook::CreateModifierKeyboardHook(
      std::move(dom_codes), GetAcceleratedWidget(),
      base::BindRepeating(
          [](ui::PlatformWindowDelegate* delegate, ui::KeyEvent* event) {
            delegate->DispatchEvent(event);
          },
          base::Unretained(this)));

  return keyboard_hook_ != nullptr;
}

void WindowTreeHostPlatform::ReleaseSystemKeyEventCapture() {
  keyboard_hook_.reset();
}

bool WindowTreeHostPlatform::IsKeyLocked(ui::DomCode dom_code) {
  return keyboard_hook_ && keyboard_hook_->IsKeyLocked(dom_code);
}

base::flat_map<std::string, std::string>
WindowTreeHostPlatform::GetKeyboardLayoutMap() {
#if !defined(X11)
  return ui::GenerateDomKeyboardLayoutMap();
#else
  NOTIMPLEMENTED();
  return {};
#endif
}

void WindowTreeHostPlatform::SetCursorNative(gfx::NativeCursor cursor) {
  if (cursor == current_cursor_)
    return;
  current_cursor_ = cursor;

#if defined(OS_WIN)
  ui::CursorLoaderWin cursor_loader;
  cursor_loader.SetPlatformCursor(&cursor);
#endif

// Pointer cursor is considered as default system cursor.
// So, for pointer cursor, method SetCustomCursor with kNotUse argument
// is called instead of SetCursor to substitute default pointer cursor
// (black arrow) to default wayland cursor (pink plectrum).
#if defined(OS_WEBOS)
  ui::CursorType native_type = cursor.native_type();
  if (native_type == ui::CursorType::kPointer) {
    platform_window_->SetCustomCursor(neva_app_runtime::CustomCursorType::kNotUse,
                                      "", 0, 0, false);
    return;
  } else if (native_type == ui::CursorType::kNone) {
    // Hiding of the cursor after some time is handled by LSM, but some sites
    // for video playback are also have such functionality in JavaScript.
    // And in case when cursor was hidden firstly by LSM and then by
    // JavaScript, it no longer could be restored.
    // To fix such situations hiding cursor by JavaScript is ignored.
    return;
  }
#endif

  platform_window_->SetCursor(cursor.platform());
}

void WindowTreeHostPlatform::MoveCursorToScreenLocationInPixels(
    const gfx::Point& location_in_pixels) {
  platform_window_->MoveCursorTo(location_in_pixels);
}

void WindowTreeHostPlatform::OnCursorVisibilityChangedNative(bool show) {
  NOTIMPLEMENTED();
}

///@name USE_NEVA_APPRUNTIME
///@{
void WindowTreeHostPlatform::SetWindowProperty(const std::string& name,
                                               const std::string& value) {
#if defined(USE_OZONE)
  platform_window_->SetWindowProperty(name, value);
#endif
}

void WindowTreeHostPlatform::ToggleFullscreen() {
#if defined(USE_OZONE)
  platform_window_->ToggleFullscreen();
#endif
}
///@}

void WindowTreeHostPlatform::OnBoundsChanged(const gfx::Rect& new_bounds) {
  // It's possible this function may be called recursively. Only notify
  // observers on initial entry. This way observers can safely assume that
  // OnHostDidProcessBoundsChange() is called when all bounds changes have
  // completed.
  if (++on_bounds_changed_recursion_depth_ == 1) {
    for (WindowTreeHostObserver& observer : observers())
      observer.OnHostWillProcessBoundsChange(this);
  }
  float current_scale = compositor()->device_scale_factor();
  float new_scale = ui::GetScaleFactorForNativeView(window());
  gfx::Rect old_bounds = bounds_in_pixels_;
  bounds_in_pixels_ = new_bounds;
  if (bounds_in_pixels_.origin() != old_bounds.origin())
    OnHostMovedInPixels(bounds_in_pixels_.origin());
  if (bounds_in_pixels_.size() != old_bounds.size() ||
      current_scale != new_scale) {
    pending_size_ = gfx::Size();
    OnHostResizedInPixels(bounds_in_pixels_.size());
  }
  DCHECK_GT(on_bounds_changed_recursion_depth_, 0);
  if (--on_bounds_changed_recursion_depth_ == 0) {
    for (WindowTreeHostObserver& observer : observers())
      observer.OnHostDidProcessBoundsChange(this);
  }
}

void WindowTreeHostPlatform::OnDamageRect(const gfx::Rect& damage_rect) {
  compositor()->ScheduleRedrawRect(damage_rect);
}

void WindowTreeHostPlatform::DispatchEvent(ui::Event* event) {
  TRACE_EVENT0("input", "WindowTreeHostPlatform::DispatchEvent");
  ui::EventDispatchDetails details = SendEventToSink(event);
  if (details.dispatcher_destroyed)
    event->SetHandled();
}

void WindowTreeHostPlatform::OnCloseRequest() {
  OnHostCloseRequested();
}

void WindowTreeHostPlatform::OnClosed() {}

void WindowTreeHostPlatform::OnWindowStateChanged(
    ui::PlatformWindowState new_state) {}

#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
void WindowTreeHostPlatform::OnWindowHostStateChanged(
    ui::WidgetState new_state) {
  WindowTreeHost::OnWindowHostStateChanged(new_state);
}

void WindowTreeHostPlatform::OnWindowHostClose() {
#if defined(USE_NEVA_APPRUNTIME)
  if (native_event_delegate_)
    native_event_delegate_->WindowHostClose();
#endif
}
#endif

void WindowTreeHostPlatform::OnLostCapture() {
  OnHostLostWindowCapture();
}

void WindowTreeHostPlatform::OnAcceleratedWidgetAvailable(
    gfx::AcceleratedWidget widget) {
  widget_ = widget;
  // This may be called before the Compositor has been created.
  if (compositor())
    WindowTreeHost::OnAcceleratedWidgetAvailable();
}

void WindowTreeHostPlatform::OnAcceleratedWidgetDestroyed() {
  gfx::AcceleratedWidget widget = compositor()->ReleaseAcceleratedWidget();
  DCHECK_EQ(widget, widget_);
  widget_ = gfx::kNullAcceleratedWidget;
}

void WindowTreeHostPlatform::OnActivationChanged(bool active) {}

void WindowTreeHostPlatform::OnMouseEnter() {
  client::CursorClient* cursor_client = client::GetCursorClient(window());
  if (cursor_client) {
    auto display =
        display::Screen::GetScreen()->GetDisplayNearestWindow(window());
    DCHECK(display.is_valid());
    cursor_client->SetDisplay(display);
  }
}

void WindowTreeHostPlatform::OnShowIme() {
#if defined(USE_OZONE)
  platform_window_->ShowInputPanel();
#endif
}

void WindowTreeHostPlatform::OnHideIme(ui::ImeHiddenType hidden_type) {
#if defined(USE_OZONE)
  platform_window_->HideInputPanel(hidden_type);
#endif
}

void WindowTreeHostPlatform::OnTextInputInfoChanged(
      const ui::TextInputInfo& text_input_info) {
#if defined(USE_OZONE)
  if (text_input_info.type != ui::InputContentType::INPUT_CONTENT_TYPE_NONE)
    platform_window_->SetTextInputInfo(text_input_info);
#endif
}

///@name USE_NEVA_APPRUNTIME
///@{
void WindowTreeHostPlatform::SetSurroundingText(const std::string& text,
                                                size_t cursor_position,
                                                size_t anchor_position) {
#if defined(USE_OZONE)
  platform_window_->SetSurroundingText(text, cursor_position, anchor_position);
#endif
}
///@}

}  // namespace aura
