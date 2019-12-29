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

#include "ui/ozone/platform/wayland/extensions/webos/host/wayland_webos_extension.h"

#include "base/logging.h"
#include "ui/ozone/platform/wayland/extensions/webos/common/wayland_webos_object.h"
#include "ui/ozone/platform/wayland/extensions/webos/host/webos_shell_surface_wrapper.h"
#include "ui/ozone/platform/wayland/host/shell_popup_wrapper.h"

namespace ui {

constexpr uint32_t kMaxWlShellVersion = 1;
constexpr uint32_t kMaxWebOSShellVersion = 1;

bool WaylandWebosExtension::Bind(wl_registry* registry,
                                 uint32_t name,
                                 const char* interface,
                                 uint32_t version) {
  if (!wl_shell_ && strcmp(interface, "wl_shell") == 0) {
    wl_shell_ = wl::Bind<wl_shell>(registry, name,
                                   std::min(version, kMaxWlShellVersion));

    if (!wl_shell_) {
      LOG(ERROR) << "Failed to bind to wl_shell global";
      return false;
    }
    return true;
  } else if (!webos_shell_ && strcmp(interface, "wl_webos_shell") == 0) {
    webos_shell_ = wl::Bind<wl_webos_shell>(
        registry, name, std::min(version, kMaxWebOSShellVersion));

    if (!webos_shell_) {
      LOG(ERROR) << "Failed to bind to wl_webos_shell global";
      return false;
    }
    return true;
  }

  return false;
}

bool WaylandWebosExtension::HasShellObject() {
  if (wl_shell_ && webos_shell_)
    return true;
  else
    return false;
}

std::unique_ptr<ShellSurfaceWrapper> WaylandWebosExtension::CreateShellSurface(
    WaylandWindow* wayland_window) {
  if (wl_shell_ && webos_shell_)
    return std::make_unique<WebOSShellSurfaceWrapper>(wayland_window);
  else
    return nullptr;
}

std::unique_ptr<ShellPopupWrapper> WaylandWebosExtension::CreateShellPopup(
    WaylandWindow* wayland_window) {
  return nullptr;
}

std::unique_ptr<WaylandExtension> CreateWaylandExtension() {
  return std::make_unique<WaylandWebosExtension>();
}

}  // namespace ui
