// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/screen_ozone.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"

#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
#include "base/command_line.h"
#include "ui/gfx/switches.h"
#include "ui/views/widget/desktop_aura/desktop_screen_headless.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#include "ui/views/widget/desktop_aura/desktop_factory_ozone.h"
#endif

namespace views {

#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
display::Screen* CreateDesktopScreen() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kHeadless))
    return new views::DesktopScreenHeadless;

  return DesktopFactoryOzone::GetInstance()->CreateDesktopScreen();
}
#else
display::Screen* CreateDesktopScreen() {
  return new aura::ScreenOzone();
}
#endif

}  // namespace views
