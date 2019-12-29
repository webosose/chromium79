// Copyright 2019 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "ui/ozone/platform/wayland/extensions/webos/host/webos_shell_surface_wrapper.h"

#include <wayland-webos-shell-client-protocol.h>

#include "base/strings/utf_string_conversions.h"
#include "ui/base/hit_test.h"
#include "ui/ozone/platform/wayland/common/wayland_util.h"
#include "ui/ozone/platform/wayland/extensions/webos/host/wayland_webos_extension.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"
#include "ui/ozone/platform/wayland/host/wayland_window.h"

namespace ui {

PlatformWindowState ToPlatformWindowState(uint32_t state) {
  switch (state) {
    case WL_WEBOS_SHELL_SURFACE_STATE_DEFAULT:
      return PlatformWindowState::kNormal;
    case WL_WEBOS_SHELL_SURFACE_STATE_MINIMIZED:
      return PlatformWindowState::kMinimized;
    case WL_WEBOS_SHELL_SURFACE_STATE_MAXIMIZED:
      return PlatformWindowState::kMaximized;
    case WL_WEBOS_SHELL_SURFACE_STATE_FULLSCREEN:
      return PlatformWindowState::kFullScreen;
    default:
      return PlatformWindowState::kUnknown;
  }
}

bool ToActivationState(uint32_t state) {
  switch (state) {
    case WL_WEBOS_SHELL_SURFACE_STATE_DEFAULT:
    case WL_WEBOS_SHELL_SURFACE_STATE_MAXIMIZED:
    case WL_WEBOS_SHELL_SURFACE_STATE_FULLSCREEN:
      return true;
    case WL_WEBOS_SHELL_SURFACE_STATE_MINIMIZED:
      return false;
    default:
      return true;
  }
}

WebOSShellSurfaceWrapper::WebOSShellSurfaceWrapper(
    WaylandWindow* wayland_window)
    : WaylandShellSurfaceWrapper(wayland_window),
      wayland_window_(wayland_window) {}

WebOSShellSurfaceWrapper::~WebOSShellSurfaceWrapper() {}

bool WebOSShellSurfaceWrapper::Initialize(WaylandConnection* connection,
                                          wl_surface* surface,
                                          bool with_toplevel) {
  DCHECK(connection && connection->extension());

  WaylandWebosExtension* webos_extension =
      static_cast<WaylandWebosExtension*>(connection->extension());

  WaylandShellSurfaceWrapper::Initialize(connection, surface, with_toplevel);

  webos_shell_surface_.reset(wl_webos_shell_get_shell_surface(
      webos_extension->webos_shell(), surface));
  if (!webos_shell_surface_) {
    LOG(ERROR) << "Failed to create wl_webos_shell_surface";
    return false;
  }

  static const wl_webos_shell_surface_listener webos_shell_surface_listener = {
      WebOSShellSurfaceWrapper::StateChanged,
      WebOSShellSurfaceWrapper::PositionChanged,
      WebOSShellSurfaceWrapper::Close, WebOSShellSurfaceWrapper::Exposed,
      WebOSShellSurfaceWrapper::StateAboutToChange};

  wl_webos_shell_surface_add_listener(webos_shell_surface_.get(),
                                      &webos_shell_surface_listener, this);

  return true;
}

void WebOSShellSurfaceWrapper::SetMaximized() {
  wl_webos_shell_surface_set_state(webos_shell_surface_.get(),
                                   WL_WEBOS_SHELL_SURFACE_STATE_MAXIMIZED);
}

void WebOSShellSurfaceWrapper::UnSetMaximized() {
  wl_webos_shell_surface_set_state(webos_shell_surface_.get(),
                                   WL_WEBOS_SHELL_SURFACE_STATE_DEFAULT);
}

void WebOSShellSurfaceWrapper::SetFullscreen() {
  wl_webos_shell_surface_set_state(webos_shell_surface_.get(),
                                   WL_WEBOS_SHELL_SURFACE_STATE_FULLSCREEN);
}

void WebOSShellSurfaceWrapper::UnSetFullscreen() {
  wl_webos_shell_surface_set_state(webos_shell_surface_.get(),
                                   WL_WEBOS_SHELL_SURFACE_STATE_DEFAULT);
}

void WebOSShellSurfaceWrapper::SetMinimized() {
  wl_webos_shell_surface_set_state(webos_shell_surface_.get(),
                                   WL_WEBOS_SHELL_SURFACE_STATE_MINIMIZED);
}

void WebOSShellSurfaceWrapper::SetInputRegion(
    const std::vector<gfx::Rect>& region) {
  NOTIMPLEMENTED();
}

void WebOSShellSurfaceWrapper::SetWindowProperty(const std::string& name,
                                                 const std::string& value) {
  wl_webos_shell_surface_set_property(webos_shell_surface_.get(), name.c_str(),
                                      value.c_str());
}

// static
void WebOSShellSurfaceWrapper::StateChanged(
    void* data,
    wl_webos_shell_surface* webos_shell_surface,
    uint32_t state) {
  WebOSShellSurfaceWrapper* shell_surface_wrapper =
      static_cast<WebOSShellSurfaceWrapper*>(data);
  DCHECK(shell_surface_wrapper);
  DCHECK(shell_surface_wrapper->wayland_window_);

  if (shell_surface_wrapper->wayland_window_) {
    shell_surface_wrapper->wayland_window_->HandleStateChanged(
        ToPlatformWindowState(state));
    shell_surface_wrapper->wayland_window_->HandleActivationChanged(
        ToActivationState(state));
  }
}

void WebOSShellSurfaceWrapper::PositionChanged(
    void* data,
    wl_webos_shell_surface* webos_shell_surface,
    int32_t x,
    int32_t y) {
  NOTIMPLEMENTED();
}

void WebOSShellSurfaceWrapper::Close(
    void* data,
    wl_webos_shell_surface* webos_shell_surface) {
  NOTIMPLEMENTED();
}

void WebOSShellSurfaceWrapper::Exposed(
    void* data,
    wl_webos_shell_surface* webos_shell_surface,
    wl_array* rectangles) {
  WebOSShellSurfaceWrapper* shell_surface_wrapper =
      static_cast<WebOSShellSurfaceWrapper*>(data);
  DCHECK(shell_surface_wrapper);
  DCHECK(shell_surface_wrapper->wayland_window_);
  // Activate window.
  if (shell_surface_wrapper->wayland_window_)
    shell_surface_wrapper->wayland_window_->HandleActivationChanged(true);
}

void WebOSShellSurfaceWrapper::StateAboutToChange(
    void* data,
    wl_webos_shell_surface* webos_shell_surface,
    uint32_t state) {
  NOTIMPLEMENTED();
}

}  // namespace ui
