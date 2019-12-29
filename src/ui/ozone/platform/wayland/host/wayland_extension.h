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

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_EXTENSION_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_EXTENSION_H_

#include <memory>

#include "base/macros.h"
#include "ui/ozone/platform/wayland/common/wayland_object.h"

namespace ui {

class ShellSurfaceWrapper;
class ShellPopupWrapper;
class WaylandWindow;

class WaylandExtension {
 public:
  WaylandExtension() {}
  virtual ~WaylandExtension() {}

  virtual bool Bind(wl_registry* registry,
                    uint32_t name,
                    const char* interface,
                    uint32_t version) = 0;

  virtual bool HasShellObject() = 0;

  virtual std::unique_ptr<ShellSurfaceWrapper> CreateShellSurface(
      WaylandWindow* wayland_window) = 0;

  virtual std::unique_ptr<ShellPopupWrapper> CreateShellPopup(
      WaylandWindow* wayland_window) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(WaylandExtension);
};

// Create an wayland extension.
std::unique_ptr<WaylandExtension> CreateWaylandExtension();

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_EXTENSION_H_
