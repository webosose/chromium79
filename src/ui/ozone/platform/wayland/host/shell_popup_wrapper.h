// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_SHELL_POPUP_WRAPPER_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_SHELL_POPUP_WRAPPER_H_

#include "ui/gfx/geometry/rect.h"
#include "ui/ozone/platform/wayland/common/wayland_object.h"

namespace ui {

class WaylandConnection;
class WaylandWindow;

// A wrapper around different shell popups.
class ShellPopupWrapper {
 public:
  virtual ~ShellPopupWrapper() {}

  // Creates actual shell popup object and sets a listener to it.
  virtual bool Initialize(WaylandConnection* connection,
                          wl_surface* surface,
                          WaylandWindow* parent_window,
                          const gfx::Rect& bounds) = 0;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_SHELL_POPUP_WRAPPER_H_
