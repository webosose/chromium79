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
#include "ozone/media/foreign_video_window_provider.h"

#include <memory>
#include <string>

#include "base/threading/thread_task_runner_handle.h"
#include "base/unguessable_token.h"
#include "ozone/media/video_window_controller_impl.h"
#include "ozone/platform/messages.h"
#include "ozone/wayland/display.h"
#include "ozone/wayland/shell/shell_surface.h"
#include "ozone/wayland/window.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {

namespace {
const int kMinVideoGeometryUpdateInterval = 200;  // milliseconds
}

namespace {
bool IsSupportedType(ui::ForeignWindowType type) {
  return type == ui::ForeignWindowType::VIDEO ||
         type == ui::ForeignWindowType::SUBTITLE;
}

std::string ToString(ui::ForeignWindowType type) {
  switch (type) {
    case ui::ForeignWindowType::VIDEO:
      return "VIDEO";
    case ui::ForeignWindowType::SUBTITLE:
      return "SUBTITLE";
    default:
      return "INVALID";
  }
}

ui::ForeignWindowType ConvertToForeignWindowType(uint32_t type) {
  switch (type) {
    case 0:
      return ui::ForeignWindowType::VIDEO;
    case 1:
      return ui::ForeignWindowType::SUBTITLE;
  }
  return ui::ForeignWindowType::INVALID;
}

uint32_t ConvertToUInt32(ui::ForeignWindowType type) {
  switch (type) {
    case ui::ForeignWindowType::VIDEO:
      return 0;
    case ui::ForeignWindowType::SUBTITLE:
      return 1;
    default:
      LOG(ERROR) << __func__ << " Invalid conversion.";
      return 0;
  }
}

}  // namespace

struct ForeignVideoWindowProvider::ForeignVideoWindow : public ui::VideoWindow {
  enum class State { kNone, kCreating, kCreated, kDestroying, kDestroyed };

  ForeignVideoWindow(const base::UnguessableToken& window_id,
                     ui::ForeignWindowType type,
                     wl_surface* surface)
      : type_(type) {
    window_id_ = window_id;
    ozonewayland::WaylandDisplay* display =
        ozonewayland::WaylandDisplay::GetInstance();
    webos_exported_ = wl_webos_foreign_export_element(
        display->GetWebosForeign(), surface, ConvertToUInt32(type));
    LOG(INFO) << __func__ << " window_id=" << window_id_
              << " type=" << (int)type << " surface=" << surface
              << " webos_exported=" << webos_exported_;
  }
  ~ForeignVideoWindow() {
    LOG(INFO) << __func__;
    if (!webos_exported_)
      return;

    wl_webos_exported_destroy(webos_exported_);
  }

  struct wl_webos_exported* webos_exported_;
  ui::ForeignWindowType type_;
  VideoWindowProvider::WindowEventCb window_event_cb_;
  base::CancelableOnceCallback<void()> notify_geometry_cb_;
  gfx::Rect rect_;
  base::Time last_updated_ = base::Time::Now();
  State state_ = State::kNone;
};

std::unique_ptr<VideoWindowProvider> VideoWindowProvider::Create(
    VideoWindowSupport* support) {
  return std::make_unique<ForeignVideoWindowProvider>(support);
}

ForeignVideoWindowProvider::ForeignVideoWindowProvider(
    VideoWindowSupport* support)
    : support_(support) {}

ForeignVideoWindowProvider::~ForeignVideoWindowProvider() = default;

// static
void ForeignVideoWindowProvider::HandleExportedWindowAssigned(
    void* data,
    struct wl_webos_exported* webos_exported,
    const char* native_window_id,
    uint32_t exported_type) {
  ForeignVideoWindowProvider* window =
      static_cast<ForeignVideoWindowProvider*>(data);
  if (!window)
    return;

  window->OnCreatedForeignWindow(webos_exported, native_window_id,
                                 ConvertToForeignWindowType(exported_type));
}

void ForeignVideoWindowProvider::OnCreatedForeignWindow(
    struct wl_webos_exported* webos_exported,
    const char* native_window_id,
    ui::ForeignWindowType type) {
  LOG(INFO) << __func__ << " native_window_id=" << native_window_id;
  ForeignVideoWindow* window = FindWindow(webos_exported);
  if (!window) {
    LOG(ERROR) << __func__
               << " failed to find window for exported=" << webos_exported
               << " native_id=" << native_window_id;
    return;
  }
  window->state_ = ForeignVideoWindow::State::kCreated;
  window->native_window_name_ = native_window_id;
  window->type_ = type;
  // TODO(neva) check the running thread and is it thread-safe ?
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(window->window_event_cb_, window->window_id_,
                                ui::VideoWindowProvider::Event::kCreated));
}

void ForeignVideoWindowProvider::CreateNativeVideoWindow(
    gfx::AcceleratedWidget w,
    const base::UnguessableToken& id,
    ui::VideoWindowProvider::WindowEventCb cb) {
  ozonewayland::WaylandDisplay* display =
      ozonewayland::WaylandDisplay::GetInstance();
  struct wl_surface* surface = display->GetWindow(static_cast<unsigned>(w))
                                   ->ShellSurface()
                                   ->GetWLSurface();

  native_id_to_window_id_[id.ToString()] = id;
  foreign_windows_.emplace(id, std::make_unique<ForeignVideoWindow>(
                                   id, ForeignWindowType::VIDEO, surface));
  ForeignVideoWindow* window = FindWindow(id);
  window->state_ = ForeignVideoWindow::State::kCreating;
  window->window_id_ = id;
  window->window_event_cb_ = cb;
  static const struct wl_webos_exported_listener exported_listener = {
      ForeignVideoWindowProvider::HandleExportedWindowAssigned};

  wl_webos_exported_add_listener(window->webos_exported_, &exported_listener,
                                 this);

  wl_display_dispatch(display->display());
}

void ForeignVideoWindowProvider::UpdateNativeVideoWindowGeometry(
    const base::UnguessableToken& window_id) {
  LOG(INFO) << __func__ << " window_id=" << window_id;
  ForeignVideoWindow* w = FindWindow(window_id);
  if (!w) {
    LOG(ERROR) << __func__ << " failed to find foreign window for "
               << window_id;
    return;
  }

  if (!w->notify_geometry_cb_.IsCancelled())
    w->notify_geometry_cb_.Cancel();

  gfx::Rect source;
  const gfx::Rect& dest = w->rect_;
  wl_compositor* wlcompositor =
      ozonewayland::WaylandDisplay::GetInstance()->GetCompositor();
  wl_region* source_region = wl_compositor_create_region(wlcompositor);
  wl_region_add(source_region, source.x(), source.y(), source.width(),
                source.height());

  wl_region* dest_region = wl_compositor_create_region(wlcompositor);
  wl_region_add(dest_region, dest.x(), dest.y(), dest.width(), dest.height());

  wl_webos_exported_set_exported_window(w->webos_exported_, source_region,
                                        dest_region);
  wl_region_destroy(dest_region);
  wl_region_destroy(source_region);
  w->last_updated_ = base::Time::Now();
}

void ForeignVideoWindowProvider::NativeVideoWindowGeometryChanged(
    const base::UnguessableToken& window_id,
    const gfx::Rect& rect) {
  ForeignVideoWindow* win = FindWindow(window_id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find windows for id=" << window_id;
    return;
  }

  if (win->rect_ == rect)
    return;
  win->rect_ = rect;

  // callback is already scheduled!
  if (!win->notify_geometry_cb_.IsCancelled()) {
    return;
  }

  if (base::Time::Now() - win->last_updated_ <
      base::TimeDelta::FromMilliseconds(kMinVideoGeometryUpdateInterval)) {
    win->notify_geometry_cb_.Reset(base::BindOnce(
        &ForeignVideoWindowProvider::UpdateNativeVideoWindowGeometry,
        base::Unretained(this), window_id));
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, win->notify_geometry_cb_.callback(),
        base::TimeDelta::FromMilliseconds(kMinVideoGeometryUpdateInterval));
    return;
  }
  UpdateNativeVideoWindowGeometry(window_id);
}

void ForeignVideoWindowProvider::NativeVideoWindowVisibilityChanged(
    const base::UnguessableToken& window_id,
    bool visibility) {
  VLOG(1) << __func__ << " window_id=" << window_id
          << " visibility=" << visibility;

  ForeignVideoWindow* win = FindWindow(window_id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find windows for id=" << window_id;
    return;
  }
  if (!win->notify_geometry_cb_.IsCancelled()) {
    UpdateNativeVideoWindowGeometry(window_id);
  }

  support_->SendVideoWindowMessage(
      new WaylandDisplay_VideoWindowVisibilityChanged(window_id, visibility));
}

void ForeignVideoWindowProvider::DestroyNativeVideoWindow(
    const base::UnguessableToken& id) {
  ForeignVideoWindow* w = FindWindow(id);
  if (w) {
    w->state_ = ForeignVideoWindow::State::kDestroying;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(w->window_event_cb_, id,
                                  ui::VideoWindowProvider::Event::kDestroyed));
    foreign_windows_.erase(id);
  } else {
    LOG(WARNING) << __func__ << " failed to find video window id=" << id;
  }
}

std::string ForeignVideoWindowProvider::GetNativeVideoWindowName(
    const base::UnguessableToken& id) {
  ForeignVideoWindow* win = FindWindow(id);
  if (!win)
    return "";
  return win->native_window_name_;
}

ForeignVideoWindowProvider::ForeignVideoWindow*
ForeignVideoWindowProvider::FindWindow(
    struct wl_webos_exported* webos_exported) {
  for (auto it = foreign_windows_.begin(); it != foreign_windows_.end(); ++it) {
    if (it->second->webos_exported_ == webos_exported) {
      return it->second.get();
    }
  }
  return nullptr;
}

ForeignVideoWindowProvider::ForeignVideoWindow*
ForeignVideoWindowProvider::FindWindow(const base::UnguessableToken& id) {
  auto it = foreign_windows_.find(id);
  if (it == foreign_windows_.end())
    return nullptr;
  return it->second.get();
}
}  // namespace ui
