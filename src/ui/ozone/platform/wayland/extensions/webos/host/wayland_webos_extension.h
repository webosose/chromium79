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

#ifndef UI_OZONE_PLATFORM_WAYLAND_EXTENSIONS_WEBOS_HOST_WAYLAND_WEBOS_EXTENSION_H_
#define UI_OZONE_PLATFORM_WAYLAND_EXTENSIONS_WEBOS_HOST_WAYLAND_WEBOS_EXTENSION_H_

#include <memory>

#include "base/macros.h"
#include "ui/ozone/platform/wayland/extensions/webos/common/wayland_webos_object.h"
#include "ui/ozone/platform/wayland/host/wayland_extension.h"

namespace ui {

class WaylandWebosExtension : public WaylandExtension {
 public:
  WaylandWebosExtension(){};
  ~WaylandWebosExtension() override{};

  bool Bind(wl_registry* registry,
            uint32_t name,
            const char* interface,
            uint32_t version) override;

  bool HasShellObject() override;

  std::unique_ptr<ShellSurfaceWrapper> CreateShellSurface(
      WaylandWindow* wayland_window) override;

  std::unique_ptr<ShellPopupWrapper> CreateShellPopup(
      WaylandWindow* wayland_window) override;

  wl_shell* shell() const { return wl_shell_.get(); }
  wl_webos_shell* webos_shell() const { return webos_shell_.get(); }

 private:
  wl::Object<wl_shell> wl_shell_;
  wl::Object<wl_webos_shell> webos_shell_;

  DISALLOW_COPY_AND_ASSIGN(WaylandWebosExtension);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_EXTENSIONS_WEBOS_HOST_WAYLAND_WEBOS_EXTENSION_H_
